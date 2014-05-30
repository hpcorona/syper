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

#define SYPERAPI extern "C" __declspec(dllexport)

SYPERAPI void* syper_init();

SYPERAPI void syper_destroy(void*);

SYPERAPI void syper_start(void*);

SYPERAPI void syper_stop(void*);

SYPERAPI void syper_setmaxthreads(void*, int);

SYPERAPI void syper_setzmqcontext(void*, void*);

SYPERAPI void syper_setfcgisocket(void*, int);

SYPERAPI void syper_addzmqbackend(void*, const char*);

SYPERAPI void syper_setzmqlog(void*, const char*);

#endif /* defined(__syper__syper__) */
