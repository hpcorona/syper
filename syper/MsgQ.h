//
//  MsgQ.h
//  syper
//
//  Created by Hilario Perez Corona on 5/21/14.
//  Copyright (c) 2014 Hilario Pérez Corona. All rights reserved.
//

#ifndef __syper__MsgQ__
#define __syper__MsgQ__

#include <iostream>
#include <string>
#include <vector>
#include <exception>

#define MSGQ_OK         1
#define MSGQ_TIMEOUT    0
#define MSGQ_ERROR      -1
#define MSGQ_CLOSED     -2

class MsgQException : public std::exception {
public:
	int err;
	char* errstr;

	MsgQException();
	MsgQException(int);
	MsgQException(const char*);
	virtual ~MsgQException() throw();

	virtual const char* what() const throw();
};

class MsgQ {
public:
	void* context;
	int type;
	void* socket;

	int sendTimeout;
	int receiveTimeout;

	std::vector<std::string> addresses;
	std::vector<std::string> filters;

	char* message;
	size_t messageSize;

	short pollEvents;

	MsgQ(void*, int);
	virtual ~MsgQ();

	void addAddress(const std::string&);
	void bind();

	void exceptionErr();
	void ok(int);
	void close();
	void addFilter(const std::string&);
	void connect();

	int receive(int);
	int send(const char*, size_t, int);
	int sends(const char*, int);

	bool nextMessage(int, bool&, bool);
	bool nextMessage(int, int);
	bool nextMessage(int, int, bool&, bool);
	bool hasMore();
};

class PollQ {
public:
	PollQ();
	virtual ~PollQ();

	int pollTimeout;
	std::vector<MsgQ*> sockets;
	void* pollitems;
	int pollItemCount;

	void addSocket(MsgQ*);
	void setup();
	int poll();
	void proxy();
};

#endif /* defined(__syper__MsgQ__) */
