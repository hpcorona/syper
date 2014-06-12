//
//  syper.h
//  syper
//
//  Created by Hilario Perez Corona on 5/22/14.
//  Copyright (c) 2014 Hilario Pérez Corona. All rights reserved.
//

#ifndef __syper__syper__
#define __syper__syper__

#include "stdafx.h"

#ifdef WIN32
#define SYPERAPI extern "C" __declspec(dllexport)
#define SYPERDECL
#else
#define SYPERAPI extern "C" __attribute__ ((visibility ("default")))
#define SYPERDECL
#endif

SYPERAPI void* syper_init();

SYPERAPI void syper_destroy(void*);

SYPERAPI void syper_start(void*);

SYPERAPI void syper_stop(void*);

SYPERAPI void syper_setmaxthreads(void*, int);

SYPERAPI void syper_setzmqcontext(void*, void*);

SYPERAPI void syper_setfcgisocket(void*, int);

SYPERAPI void syper_addzmqbackend(void*, const char*);

SYPERAPI void syper_setzmqlog(void*, const char*);

SYPERAPI void syper_criticalstart(const char*);

SYPERAPI void syper_criticalend(const char*);

SYPERAPI int syper_threadstart(const char*, int, void (*)(void*), void*);

SYPERAPI const char* syper_threadname();

SYPERAPI int syper_threadalive(const char*);

SYPERAPI void syper_threadjoin(const char*);

#endif /* defined(__syper__syper__) */
