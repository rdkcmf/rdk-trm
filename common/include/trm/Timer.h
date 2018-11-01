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


#ifndef TRM_TIMER_H_
#define TRM_TIMER_H_

#include <stdint.h>

#include "TRM.h"

TRM_BEGIN_NAMESPACE
class TimerTask
{
public:
	virtual void run(void) {};
	virtual ~TimerTask() {};
    virtual TimerTask* clone() = 0;
};

class Timer
{
public:
	Timer(const std::string &token);
	void schedule(TimerTask &task, int64_t milliSecs, bool absolute = false);
	const std::string& getToken(void) const {return token;}
	void cancel(void);
	~Timer(void);
private:
	void *data;
	std::string token;
};


TRM_END_NAMESPACE
#endif


/** @} */
/** @} */
