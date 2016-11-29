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


/**
* @defgroup trm
* @{
* @defgroup common
* @{
**/


#ifndef TRM_TRM_H_
#define TRM_TRM_H_

#include <cstdio>
#include <stdexcept>
#include <stdint.h>

#define LOCAL_BEGIN_NAMESPACE namespace {
#define LOCAL_END_NAMESPACE }
#define TRM_BEGIN_NAMESPACE namespace TRM {
#define TRM_END_NAMESPACE }

#define UNUSED_VARIABLE(x) (void(x))

TRM_BEGIN_NAMESPACE

class Exception
{

};

class IllegalArgumentException : public Exception
{

};
class InvalidOperationException : public Exception
{

};
class ItemNotFoundException : public Exception
{

};
class ConnectionNotFoundException : public Exception
{

};
class AssertionFailureException : public Exception
{

};
class InvalidStateException : public Exception
{

};

class Empty
{
};

#define Assert(expr) do{\
    if (!(expr)) {\
		printf("[%s] failed at [%s][%d]\r\n", "Assert ", __FILE__, __LINE__);\
		throw AssertionFailureException();\
    }\
} while (0)

#define SafeAssert(expr) do{\
	try {\
		Assert(expr);\
	}\
	catch(...) {\
	}\
}while(0)

#define MAKE_PAIR(v) v, #v

TRM_END_NAMESPACE

#include <functional>
#include <string>
namespace std {
    template<> class less<const char*>
    {
    public:
        bool operator()(const char* lhs, const char *rhs) const
        {
            return (std::string(lhs).compare(rhs) < 0);
        }
    };
}


#endif


/** @} */
/** @} */
