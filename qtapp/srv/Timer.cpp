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
* @defgroup qtapp
* @{
**/


#include <QObject>
#include <QTimer>
#include <QTime>

#include "trm/Timer.h"
#include "QtTimer.h"

namespace TRM {

#define PVOID(p) 	((void *)(p))
#define PQTIMER_(p) ((QtTimer *)(p))
#define PQTIMER() 	(PQTIMER_(data))
#define QTIMER() 	(*(PQTIMER()))

Timer::Timer(const std::string &token)
: data(PVOID(new TRM::QtTimer())), token(token)
{
};

void Timer::schedule(TimerTask &task, int64_t milliSecs, bool absolute)
{
    int64_t future = 0;
    qint64 now = QDateTime::currentMSecsSinceEpoch(); 
	if (!absolute) {
        future = now + milliSecs;
	}
	else {
        future = milliSecs;
		milliSecs = (milliSecs - now);
		if (milliSecs < 0) milliSecs = 0;
	}
    std::cout << "Scheduling Timer " << getToken() << std::endl;
	QTIMER().schedule(task, future, milliSecs);

}

void Timer::cancel()
{
	QTIMER().cancel();
}

Timer::~Timer(void)
{
     delete PQTIMER();
}

}


/** @} */
/** @} */
