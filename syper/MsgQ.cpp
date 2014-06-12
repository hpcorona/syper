//
//  MsgQ.cpp
//  syper
//
//  Created by Hilario Perez Corona on 5/21/14.
//  Copyright (c) 2014 Hilario Pérez Corona. All rights reserved.
//

#include "MsgQ.h"
#include <stdlib.h>
#include <sstream>
#include <assert.h>
#include <zmq.h>
#include "util.h"
#include "stdafx.h"
#ifndef WIN32
#include <string.h>
#endif

MsgQ::MsgQ(void* ctx, int type) {
	this->context = ctx;
	this->type = type;

	socket = nullptr;
	sendTimeout = 1000;
	receiveTimeout = 1000;

	message = nullptr;
	messageSize = 0;

	pollEvents = 0;
}

MsgQ::~MsgQ() {
	close();

	if (message != nullptr) {
		free(message);
		message = nullptr;
	}
}

void MsgQ::addAddress(const std::string &address) {
	addresses.push_back(address);
}

void MsgQ::addFilter(const std::string &filter) {
	filters.push_back(filter);
}

void MsgQ::bind() {
	if (addresses.size() == 0) {
		throw MsgQException("No bind address defined");
	}

	socket = zmq_socket(context, type);
	if (socket == nullptr) {
		exceptionErr();
	}


	ok(zmq_setsockopt(socket, ZMQ_RCVTIMEO, &receiveTimeout, sizeof(receiveTimeout)));
	ok(zmq_setsockopt(socket, ZMQ_SNDTIMEO, &sendTimeout, sizeof(sendTimeout)));

	for (std::vector<std::string>::iterator i = addresses.begin(); i != addresses.end(); i++) {
		ok(zmq_bind(socket, i->c_str()));
	}
}

void MsgQ::connect() {
	if (addresses.size() == 0) {
		throw MsgQException("No connect address defined");
	}

	socket = zmq_socket(context, type);
	if (socket == nullptr) {
		exceptionErr();
	}


	ok(zmq_setsockopt(socket, ZMQ_RCVTIMEO, &receiveTimeout, sizeof(receiveTimeout)));
	ok(zmq_setsockopt(socket, ZMQ_SNDTIMEO, &sendTimeout, sizeof(sendTimeout)));

	for (std::vector<std::string>::iterator i = addresses.begin(); i != addresses.end(); i++) {
		ok(zmq_connect(socket, i->c_str()));
	}

	for (std::vector<std::string>::iterator i = filters.begin(); i != filters.end(); i++) {
		ok(zmq_setsockopt(socket, ZMQ_SUBSCRIBE, i->c_str(), i->length()));
	}
}

void MsgQ::exceptionErr() {
	throw MsgQException();
}

void MsgQ::ok(int e) {
	if (e == 0) return;

	exceptionErr();
}

void MsgQ::close() {
	if (socket == nullptr) return;

	int linger = 1000;
	ok(zmq_setsockopt(socket, ZMQ_LINGER, &linger, sizeof(linger)));
	ok(zmq_close(socket));
	socket = nullptr;
}

int MsgQ::receive(int flags) {
	if (message != nullptr) {
		free(message);
		message = nullptr;
		messageSize = 0;
	}

	int size = zmq_msg_s_recv(socket, &message, flags);
	if (size < 0) {
		int err = zmq_errno();
		switch (err) {
		case ETIMEDOUT:
		case EAGAIN:
			return MSGQ_TIMEOUT;
		case ETERM:
		case 128:
			return MSGQ_CLOSED;
		default:
			return MSGQ_ERROR;
		}
	}

	messageSize = size;
	return MSGQ_OK;
}

int MsgQ::send(const char* data, size_t size, int flags) {
	int ret = zmq_msg_b_send(socket, data, size, flags);
	if (ret < 0) {
		int err = zmq_errno();
		switch (err) {
		case ETIMEDOUT:
		case EAGAIN:
			return MSGQ_TIMEOUT;
		case ETERM:
		case 128:
			return MSGQ_CLOSED;
		default:
			return MSGQ_ERROR;
		}
	}

	return MSGQ_OK;
}

int MsgQ::sends(const char* data, int flags) {
	size_t size = strlen(data);
	return send(data, size, flags);
}

bool MsgQ::nextMessage(int flags, bool& sharedVar, bool runWhile) {
	int ret;
	while (sharedVar == runWhile) {
		ret = receive(flags);
		if (ret == MSGQ_OK) {
			return true;
		}
		else if (ret == MSGQ_TIMEOUT) {
			continue;
		}
		else if (ret == MSGQ_CLOSED) {
			return false;
		}
		else {
			throw MsgQException();
		}
	}

	return false;
}

bool MsgQ::nextMessage(int flags, int retryCount) {
	int ret;
	while (retryCount-- >= 0) {
		ret = receive(flags);
		if (ret == MSGQ_OK) {
			return true;
		}
		else if (ret == MSGQ_TIMEOUT) {
			continue;
		}
		else if (ret == MSGQ_CLOSED) {
			return false;
		}
		else {
			throw MsgQException();
		}
	}

	return false;
}

bool MsgQ::nextMessage(int flags, int retryCount, bool& sharedVar, bool runWhile) {
	int ret;
	while (sharedVar == runWhile && retryCount-- >= 0) {
		ret = receive(flags);
		if (ret == MSGQ_OK) {
			return true;
		}
		else if (ret == MSGQ_TIMEOUT) {
			continue;
		}
		else if (ret == MSGQ_CLOSED) {
			return false;
		}
		else {
			throw MsgQException();
		}
	}

	return false;
}

bool MsgQ::hasMore() {
	int more;
	size_t moreSize;
	moreSize = sizeof(more);

	ok(zmq_getsockopt(socket, ZMQ_RCVMORE, &more, &moreSize));

	return more != 0;
}

MsgQException::MsgQException() {
	err = zmq_errno();
	errstr = nullptr;
}

MsgQException::MsgQException(int e) {
	err = e;
	errstr = nullptr;
}

MsgQException::MsgQException(const char* str) {
	err = 0;
	size_t size = strlen(str);
	errstr = new char[size + 1];
	memcpy(errstr, str, size);
	errstr[size] = 0;
}

MsgQException::~MsgQException() throw() {
	if (errstr != nullptr) {
		free(errstr);
	}
}

const char* MsgQException::what() const throw() {
	if (errstr != nullptr) {
		return errstr;
	}

	std::stringstream buff;
	buff << "zmq:error #";
	buff << err << " - " << zmq_strerror(err);

	return buff.str().c_str();
}

PollQ::PollQ() {
	pollitems = nullptr;
	pollTimeout = 1000;
}


PollQ::~PollQ() {
	if (pollitems != nullptr) {
		free(pollitems);
	}
}

void PollQ::addSocket(MsgQ* msg) {
	sockets.push_back(msg);
}

void PollQ::setup() {
	if (pollitems != nullptr) {
		free(pollitems);
		pollitems = nullptr;
	}

	size_t totalbytes = sizeof(zmq_pollitem_t)* sockets.size();
	pollitems = malloc(totalbytes);
	pollItemCount = (int)sockets.size();

	zmq_pollitem_t *polls = (zmq_pollitem_t*)pollitems;

	for (int i = 0; i < pollItemCount; i++) {
		polls[i].socket = sockets[i]->socket;
		polls[i].fd = 0;
		polls[i].events = sockets[i]->pollEvents;
		polls[i].revents = 0;
	}
}

int PollQ::poll() {
	zmq_pollitem_t *polls = (zmq_pollitem_t*)pollitems;

	int total = zmq_poll(polls, pollItemCount, pollTimeout);
	if (total < 0) {
		throw MsgQException(total);
	}

	for (int i = 0; i < pollItemCount; i++) {
		sockets[i]->pollEvents = polls[i].revents;
	}

	return total;
}

void PollQ::proxy() {
	if (pollItemCount != 2) {
		throw MsgQException("Proxy only works with two sockets");
	}

	int total = poll();

	if (total < 1) return;

	MsgQ* socket1 = sockets[0];
	MsgQ* socket2 = sockets[1];
	bool more;
	int ret;

	if (socket1->pollEvents & ZMQ_POLLIN != 0) {
		more = true;

		while (more) {
			ret = socket1->receive(ZMQ_DONTWAIT);
			assert(ret == MSGQ_OK);

			more = socket1->hasMore();

			ret = socket2->send(socket1->message, socket1->messageSize, more ? ZMQ_SNDMORE : 0);
		}
	}
	if (socket2->pollEvents & ZMQ_POLLIN != 0) {
		more = true;

		while (more) {
			ret = socket2->receive(ZMQ_DONTWAIT);
			assert(ret == MSGQ_OK);

			more = socket2->hasMore();

			ret = socket1->send(socket2->message, socket2->messageSize, more ? ZMQ_SNDMORE : 0);
		}
	}
}
