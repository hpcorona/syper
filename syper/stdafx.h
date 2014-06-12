// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#ifdef WIN32
#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
#include <zmq.h>
#include <stdio.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <fcgiapp.h>
#include <zmq_utils.h>
#include <signal.h>
#include <assert.h>

#include "util.h"
#include "MsgQ.h"
#include "syper.h"
#endif
