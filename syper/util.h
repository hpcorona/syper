//
//  util.h
//  syper
//
//  Created by Hilario Perez Corona on 5/21/14.
//  Copyright (c) 2014 Hilario Pérez Corona. All rights reserved.
//

#ifndef syper_util_h
#define syper_util_h

int zmq_msg_s_recv(void*, char**, int);

int zmq_msg_s_send(void*, const char*, int);

int zmq_msg_b_send(void*, const char*, size_t, int);

const char* json_string(const char* original);

#endif
