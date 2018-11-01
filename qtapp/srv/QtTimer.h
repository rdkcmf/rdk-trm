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


#ifndef TRM_QT_TIMER_H_
#define TRM_QT_TIMER_H_
#include <iostream>

#include <QObject>
#include <QTimer>
#include <QMutex>
#include <QDate>
#include <QTime>
#include <limits>

#include "trm/TRM.h"
#include "trm/Timer.h"

namespace TRM {

extern   std::ostream & Log(void) ;

//@TODO: Synchronization.

#define _TRIM_QT_TIMER_BYPASS_PRECISION_CHECK_ 0

class QtTimer : public QObject
{
	Q_OBJECT
public:
	QtTimer(void)
	: QObject(0)
	{
		timer.setSingleShot(true);
		task = 0;
        future = 0;
		QObject::connect(&timer, SIGNAL(timeout()), this, SLOT(onExpiration()));
	};
	~QtTimer(void) {};

	void schedule(TimerTask &task, int64_t expireAt, int64_t milliSecs)
	{

		int32_t max32 = std::numeric_limits<int32_t>::max();
		mutex.lock();
        future = expireAt; 
		this->task = &task;
		milliSecs = (milliSecs <= ((int64_t)max32)) ? milliSecs : max32;
        timer.start(milliSecs);
		mutex.unlock();
	}
	void cancel(void)
	{
		mutex.lock();
		timer.stop();
		task = 0;
        future = 0;
		mutex.unlock();
	}

private slots:
	void onExpiration(void)
	{
		qint64 now = QDateTime::currentMSecsSinceEpoch(); 
        if (_TRIM_QT_TIMER_BYPASS_PRECISION_CHECK_ || now >= future) {
            if (task) {
                TimerTask *task2 = task->clone();
                task2->run();
                delete task2;
            }
            future = 0;
        }
        else {
            int64_t milliSecs = future - now + 1;
            milliSecs = ((milliSecs > 0) ? milliSecs : 0);
            Log() << "WARN::Timer " << (void *)this << " Expired Earlyy, Reschedule " << (milliSecs)  << " with future " << future << std::endl;
            /* schedule again */
            mutex.lock();
            timer.setSingleShot(true);
            timer.start(milliSecs);
            mutex.unlock();
        }
	}

public:
	QMutex mutex;
	TimerTask *task;
	QTimer timer;
    qint64 future;
};

}
#endif


/** @} */
/** @} */
