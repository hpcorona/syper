//
//  syper.cpp
//  syper
//
//  Created by Hilario Perez Corona on 5/22/14.
//  Copyright (c) 2014 Hilario P�rez Corona. All rights reserved.
//

#include "syper.h"

#include <stdlib.h>
#include <fcgiapp.h>
#include <zmq.h>
#include <zmq_utils.h>
#include "MsgQ.h"

#include <signal.h>
#include <sstream>
#include "stdafx.h"

#ifdef WIN32
#define	SYPER_MUTEX	CRITICAL_SECTION
typedef void SYPER_THREAD_FUNC(void*);
#define SYPER_FUNC_END return;
#define SYPER_THREAD void
#else
#define SYPER_MUTEX pthread_mutex_t
typedef void* SYPER_THREAD_FUNC(void*);
#define SYPER_FUNC_END return nullptr;
#define SYPER_THREAD void*
#endif

static int releaseFcgx = 0;

class SyperContext {
public:
	void* zmq_context;
	int fastcgi_socket;
	std::vector<std::string> zmq_backends;
	char* zmq_log;
	int maxthreads;
	SYPER_MUTEX threadmux;
	int threadcount;

	bool running;
	void* fastcgi_thread;
	void* zmq_thread;
	void* log_thread;

	SyperContext();
	virtual ~SyperContext();
};

struct SyperRequest {
	SyperContext* syp;
	FCGX_Request* req;
	unsigned long long reqId;
};

#define SYPER_FRONTEND      "inproc://syper_frontend"
#define SYPER_BACKEND       "inproc://syper_backend"
#define SYPER_LOGINT		"inproc://syper_logint"
#define SYPER_LOGPUB		"inproc://syper_log"

void syper_log(SyperContext* syp, const char* msg, ...) {
	if (syp->running == false) return;
	MsgQ mq(syp->zmq_context, ZMQ_PUSH);
	mq.addAddress(SYPER_LOGINT);
	mq.connect();
	int ret;
	char msgf[200];
	msgf[0] = 0;
	va_list args;
	va_start(args, msg);
	vsprintf_s(msgf, 200, msg, args);
	va_end(args);
	while (syp->running) {
		ret = mq.sends(msgf, 0);
		if (ret == MSGQ_OK || ret == MSGQ_ERROR || ret == MSGQ_CLOSED) {
			break;
		}
	}
	mq.close();
}

void syper_threadstart(SYPER_THREAD_FUNC func, void* param) {
#ifdef WIN32
	zmq_threadstart(func, param);
#else
	pthread_t worker;
	pthread_create(&worker, nullptr, func, param);
#endif
}

void syper_threadinc(SyperContext* syp) {
#ifdef WIN32
	EnterCriticalSection(&syp->threadmux);
#else
	pthread_mutex_lock(&syp->threadmux);
#endif
	syp->threadcount += 1;
#ifdef WIN32
	LeaveCriticalSection(&syp->threadmux);
#else
	pthread_mutex_unlock(&syp->threadmux);
#endif
}

void syper_threaddec(SyperContext* syp) {
#ifdef WIN32
	EnterCriticalSection(&syp->threadmux);
#else
	pthread_mutex_lock(&syp->threadmux);
#endif
	syp->threadcount -= 1;
#ifdef WIN32
	LeaveCriticalSection(&syp->threadmux);
#else
	pthread_mutex_unlock(&syp->threadmux);
#endif
}

void syper_threadwait(SyperContext* syp) {
	// Just wait 1 millisecond if there are a lot of threads...
	if (syp->threadcount >= syp->maxthreads) {
		syper_log(syp, "[fcgi] Max requests reached");
	} else {
		return;
	}
	while (syp->threadcount >= syp->maxthreads) {
		Sleep(1);
	}
	syper_log(syp, "[fcgi] Ok, we're back into business");
}

// This procedure is the key to immortality... too much code in here
void syper_handlerequest(void* vreq) {
	char measure[35];
	SyperRequest* sreq = (SyperRequest*)vreq;
	SyperContext* syp = sreq->syp;
	FCGX_Request* req = sreq->req;
	unsigned long long reqId = sreq->reqId;

	syper_log(syp, "[%llu] handling a new request", reqId);

	size_t headerSize = 0, contentSize = 0;

	char* contSizeStr = nullptr;
	contSizeStr = FCGX_GetParam("CONTENT_LENGTH", req->envp);

	if (contSizeStr != nullptr) {
		syper_log(syp, "[%llu] content length: %s", reqId, contSizeStr);
		contentSize = atol(contSizeStr);
		if (contentSize <= 0) {
			contentSize = 0;
		}
	}
	else {
		int zero = 0;
		int zero1 = (int)contentSize;
		syper_log(syp, "[%llu] zero content length", reqId);
	}
	contentSize += 2; //Tipo (C = contenido, F = archivo, E = error)

	// Message Request ID
	sprintf_s(measure, 35, "SYPER_REQ_ID=%llu", reqId);
	headerSize += strlen(measure) + 1;

	// All parameters
	int idx = 0;
	while (req->envp[idx] != nullptr) {
		headerSize += strlen(req->envp[idx]) + 1;
		idx++;
	}

	// Ok, now build up the buffer
	char* membuff = (char*)malloc(headerSize > contentSize ? headerSize : contentSize);

	char* memptr = membuff;
	sprintf(memptr, "SYPER_REQ_ID=%llu", reqId);
	memptr += strlen(measure) + 1;

	idx = 0;
	size_t rowsize;
	while (req->envp[idx] != nullptr) {
		rowsize = strlen(req->envp[idx]) + 1;
		strcpy(memptr, req->envp[idx]);
		memptr += rowsize;
		idx++;
	}

	memptr = nullptr;

	syper_log(syp, "[%llu] connecting to frontend", reqId);

	MsgQ mq(syp->zmq_context, ZMQ_REQ);
	mq.addAddress(SYPER_FRONTEND);
	mq.connect();

	int ret = 0;
	while (syp->running) {
		ret = mq.send(membuff, headerSize, ZMQ_SNDMORE);
		if (ret == MSGQ_TIMEOUT) {
			syper_log(syp, "[%llu] frontend timeout", reqId);
			continue;
		}
		else {
			break;
		}
	}

	syper_log(syp, "[%llu] header sent to frontend", reqId);

	// Now send the content in case there is any
	if (ret == MSGQ_OK) {
		membuff[0] = 'C';
		membuff[1] = 0;
		memptr = membuff + 2;
		if (contentSize > 2) {
			syper_log(syp, "[%llu] reading content (length: %Iu)", reqId, contentSize - 2);
			int readed = FCGX_GetStr(memptr, (int)contentSize - 2, req->in);
			if (readed != (int)contentSize - 2) {
				syper_log(syp, "[%llu] can't read all the content, only readed: %d", reqId, readed);
				FCGX_FPrintF(req->err, "Invalid content received, received %d bytes, expecting %Iu", readed, contentSize);
				contentSize = readed + 2;
				membuff[0] = 'E';
			}
		}

		// TODO: if content is too big, maybe put it as a file... maybe? nah, no big posts coming from here, i can assure you that, no one will use this library for big uploads

		syper_log(syp, "[%llu] sending message to frontend", reqId);
		while (syp->running) {
			ret = mq.send(membuff, contentSize, 0);
			if (ret == MSGQ_TIMEOUT) {
				continue;
			}
			else {
				break;
			}
		}

		syper_log(syp, "[%llu] waiting for response", reqId);
		// Read the response and pass it back to fukn' fastcgi... ASAP!!
		if (ret == MSGQ_OK) {
			bool more = true;
			while (syp->running && more) {
				ret = mq.receive(0);
				if (ret == MSGQ_TIMEOUT) {
					syper_log(syp, "[%llu] frontend timedout, too much work? retrying...", reqId);
					continue;
				}
				else if (ret != MSGQ_OK) {
					syper_log(syp, "[%llu] ERROR response from server NOT ok: %s", reqId, zmq_strerror(zmq_errno()));
					break;
				}

				more = mq.hasMore();

				size_t remaining = mq.messageSize;
				syper_log(syp, "[%llu] received %Iu bytes from frontend", reqId, remaining);
				char* msgPtr = mq.message;
				while (remaining > 0) {
					ret = FCGX_PutStr(msgPtr, (int)remaining, req->out);
					if (ret < 0) {
						syper_log(syp, "[%llu] ERROR can't send more bytes to fastcgi", reqId);
						FCGX_FPrintF(req->err, "Cannot write response to FastCGI");
						break;
					}
					FCGX_FFlush(req->out);
					syper_log(syp, "[%llu] sent %d bytes to fcgi", reqId, ret);

					remaining -= ret;
					msgPtr += ret;
				}
			}
		}
	}

	syper_threaddec(syp);

	FCGX_Finish_r(req);
	syper_log(syp, "[%llu] finished the request... a new request may born now", reqId);

	free(req);
	req = nullptr;
	sreq->req = nullptr;
	delete sreq;
	sreq = nullptr;

	free(membuff);
	membuff = nullptr;
	syper_log(syp, "[%llu] free'd request memory", reqId);

	if (ret == MSGQ_ERROR) {
		syper_log(syp, "[%llu] an error was detected in the request... i'm exceptioning", reqId);
		mq.exceptionErr();
	}

	mq.close();
	syper_log(syp, "[%llu] closing zmq port... now the request is over for good", reqId);
}

void* syper_init() {
	if (releaseFcgx == 0) {
		FCGX_Init();
	}

	releaseFcgx += 1;

	SyperContext* syp = new SyperContext();

	return syp;
}

void syper_destroy(void* syp) {
	releaseFcgx -= 1;

	if (releaseFcgx == 0) {
		FCGX_ShutdownPending();
		FCGX_Finish();

	}

	delete (SyperContext*)syp;
}

void syper_setzmqcontext(void* vsyp, void* zmqc) {
	SyperContext* syp = (SyperContext*)vsyp;
	syp->zmq_context = zmqc;
}

void syper_setmaxthreads(void* vsyp, int max) {
	SyperContext* syp = (SyperContext*)vsyp;
	syp->maxthreads = max;
}

void syper_setfcgisocket(void* vsyp, int socket) {
	SyperContext* syp = (SyperContext*)vsyp;
	syp->fastcgi_socket = socket;
}

void syper_addzmqbackend(void* vsyp, const char* backend) {
	SyperContext* syp = (SyperContext*)vsyp;
	syp->zmq_backends.push_back(backend);
}

void syper_setzmqlog(void* vsyp, const char* zmqlog) {
	SyperContext* syp = (SyperContext*)vsyp;
	if (syp->zmq_log != nullptr) {
		free(syp->zmq_log);
	}
	syp->zmq_log = (char*)malloc(strlen(zmqlog) + 1);
	strcpy(syp->zmq_log, zmqlog);
}

void syper_zmqserver(void* vsyp) {
	SyperContext* syp = (SyperContext*)vsyp;
	syper_log(syp, "[zmq] started zmq frontend server on %s", SYPER_FRONTEND);

	MsgQ fmq(syp->zmq_context, ZMQ_ROUTER);
	fmq.addAddress(SYPER_FRONTEND);
	fmq.bind();

	MsgQ bmq(syp->zmq_context, ZMQ_DEALER);
	for (std::vector<std::string>::iterator i = syp->zmq_backends.begin(); i != syp->zmq_backends.end(); i++) {
		bmq.addAddress(i->c_str());
		syper_log(syp, "[zmq] binding backend to %s", i->c_str());
	}
	if (syp->zmq_backends.size() == 0) {
		bmq.addAddress(SYPER_BACKEND);
		syper_log(syp, "[zmq] binding backend to %s", SYPER_BACKEND);
	}
	bmq.bind();

	PollQ poll;
	fmq.pollEvents = ZMQ_POLLIN;
	poll.addSocket(&fmq);
	bmq.pollEvents = ZMQ_POLLIN;
	poll.addSocket(&bmq);
	poll.setup();

	syper_log(syp, "[zmq] proxying forever");
	while (syp->running) {
		poll.proxy(); // just do the hard work for me, would ya?
	}

	fmq.close();
	bmq.close();

	syper_log(syp, "[zmq] closing zmq server");
}

void syper_fastcgiserver(void* vsyp) {
	SyperContext* syp = (SyperContext*)vsyp;
	unsigned long long reqId = 0;

	syper_log(syp, "[fcgi] started fcgi server");

	FCGX_Request* req = nullptr;

	while (syp->running) {
		if (req == nullptr) {
			syper_log(syp, "[fcgi] creating a new request");
			req = new FCGX_Request;
			FCGX_InitRequest(req, syp->fastcgi_socket, FCGI_FAIL_ACCEPT_ON_INTR);
		}

		syper_log(syp, "[fcgi] waiting for a request to arrive (blocking forever)");
		if (FCGX_Accept_r(req) == 0) {
			syper_threadinc(syp);
			reqId += 1;
			SyperRequest* sreq = new SyperRequest;
			sreq->syp = syp;
			sreq->req = req;
			sreq->reqId = reqId;
			syper_log(syp, "[fcgi] request received, launching a new thread for req id: %llu", reqId);
			zmq_threadstart(syper_handlerequest, sreq);
			req = nullptr;
		}

		syper_threadwait(syp);
	}

	syper_log(syp, "[fcgi] finishing fcgi server");
	if (req != nullptr) {
		FCGX_Finish_r(req);
	}
	syper_log(syp, "[fcgi] server stopped for good");
}

void syper_logserver(void* vsyp) {
	SyperContext* syp = (SyperContext*)vsyp;

	MsgQ pull(syp->zmq_context, ZMQ_PULL);
	pull.addAddress(SYPER_LOGINT);
	pull.bind();

	MsgQ pub(syp->zmq_context, ZMQ_PUB);
	pub.addAddress(syp->zmq_log);
	pub.bind();

	int ret;
	bool hasMore;
	while (syp->running) {
		if (pull.nextMessage(0, syp->running, true) == false) {
			pub.sends("closing server", 0);
			break;
		}


		hasMore = true;
		while (hasMore) {
			pub.send(pull.message, pull.messageSize, 0);

			hasMore = pull.hasMore();
			if (hasMore) {
				ret = pull.receive(ZMQ_DONTWAIT);
				assert(ret == MSGQ_OK);
			}
		}
	}

	pull.close();
	pub.close();
}

void syper_start(void* vsyp) {
	SyperContext* syp = (SyperContext*)vsyp;
	syp->running = true;

	syp->log_thread = zmq_threadstart(syper_logserver, vsyp);
	syp->zmq_thread = zmq_threadstart(syper_zmqserver, vsyp);
	syp->fastcgi_thread = zmq_threadstart(syper_fastcgiserver, vsyp);
}

void syper_stop(void* vsyp) {
	SyperContext* syp = (SyperContext*)vsyp;
	syp->running = false;

	raise(SIGINT);

	zmq_threadclose(syp->zmq_thread);
	syp->zmq_thread = nullptr;
	zmq_threadclose(syp->fastcgi_thread);
	syp->fastcgi_thread = nullptr;
	zmq_threadclose(syp->log_thread);
	syp->log_thread = nullptr;
}

SyperContext::SyperContext() {
	fastcgi_socket = 0;
	running = false;
	fastcgi_thread = nullptr;
	zmq_context = nullptr;
	zmq_thread = nullptr;
	this->zmq_log = (char*)malloc(strlen(SYPER_LOGPUB) + 1);
	strcpy(this->zmq_log, SYPER_LOGPUB);

	maxthreads = 5;
	threadcount = 0;

#ifdef WIN32
	InitializeCriticalSection(&threadmux);
#else
	pthread_mutex_init(&threadmux, nullptr);
#endif
}

SyperContext::~SyperContext() {
	if (fastcgi_thread != nullptr || zmq_thread != nullptr) {
		syper_stop(this);
	}

	if (zmq_log != nullptr) {
		free(zmq_log);
	}

#ifdef WIN32
	DeleteCriticalSection(&threadmux);
#else
	pthread_mutex_destroy(&threadmux);
#endif
}
