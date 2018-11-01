/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2016 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

#ifndef _DIAG_DEBUG_H_
#define _DIAG_DEBUG_H_

#include <stdint.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>

//#define ENABLE_DIAG_DEBUG


/* Time stamp information */
#define __TIMESTAMP() do { /*YYMMDD-HH:MM:SS:usec*/               \
    struct tm __tm;                                             \
    struct timeval __tv;                                        \
    gettimeofday(&__tv, NULL);                                  \
    localtime_r(&__tv.tv_sec, &__tm);                           \
    printf("[TRMMgr=%ld %02d%02d%02d-%02d:%02d:%02d:%06d ",                 \
    syscall(SYS_gettid), \
    __tm.tm_year+1900-2000,                             \
    __tm.tm_mon+1,                                      \
    __tm.tm_mday,                                       \
    __tm.tm_hour,                                       \
    __tm.tm_min,                                        \
    __tm.tm_sec,                                        \
    (int)__tv.tv_usec);                                      \
} while(0)



#define DIAG_WARN( m ) __TIMESTAMP(); printf m
#ifdef ENABLE_DIAG_DEBUG
    #define DIAG_TRACE( m ) __TIMESTAMP(); printf m
    #define DIAG_DEBUG( m ) __TIMESTAMP(); printf m
#else
    #define DIAG_TRACE( m ) 
    #define DIAG_DEBUG( m ) 
#endif




#endif

