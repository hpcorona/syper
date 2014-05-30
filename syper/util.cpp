//
//  util.cpp
//  syper
//
//  Created by Hilario Perez Corona on 5/21/14.
//  Copyright (c) 2014 Hilario Pérez Corona. All rights reserved.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "util.h"
#include <zmq.h>
#include <string>
#include <sstream>
#include <iomanip>
#include "stdafx.h"

int zmq_msg_s_recv(void* socket, char** data, int flags) {
	zmq_msg_t message;
	zmq_msg_init(&message);

	int size = zmq_msg_recv(&message, socket, flags);
	if (size < 0) {
		return -1;
	}

	*data = (char*)malloc(size + 1);
	memcpy(*data, zmq_msg_data(&message), size);
	zmq_msg_close(&message);

	(*data)[size] = 0;

	return size;
}

int zmq_msg_s_send(void* socket, const char* data, int flags) {
	int size = (int)strlen(data);

	return zmq_msg_b_send(socket, data, size, flags);
}

int zmq_msg_b_send(void* socket, const char* data, size_t size, int flags) {
	zmq_msg_t message;
	zmq_msg_init_size(&message, size);
	memcpy(zmq_msg_data(&message), data, size);
	int ret = zmq_msg_send(&message, socket, flags);
	zmq_msg_close(&message);

	return ret;
}

const char* json_string(const char* original) {
	std::string::size_type maxsize = strlen(original) * 6 + 1; // all unicode + NULL
	std::string result;

	result.reserve(maxsize); // to avoid lots of mallocs
	for (const char* c = original; *c != 0; c++) {
		switch (*c) {
		case '\"':
			result += "\\\"";
			break;
		case '\\':
			result += "\\\\";
			break;
		case '\b':
			result += "\\b";
			break;
		case '\f':
			result += "\\f";
			break;
		case '\n':
			result += "\\n";
			break;
		case '\r':
			result += "\\r";
			break;
		case '\t':
			result += "\\t";
			break;
		case '/':
			result += "\\/";
			break;
		default:
			if (*c > 0 && *c <= 31) {
				std::ostringstream oss;
				oss << "\\u" << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << static_cast<int>(*c);
				result += oss.str();
			}
			else {
				result += *c;
			}
			break;
		}
	}

	return result.c_str();
}
