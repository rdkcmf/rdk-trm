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


#include "trm/TunerReservation.h"
#include "trm/Timer.h"

#ifndef TRM_RESERVATION_TIMER_TASK_H_
#define TRM_RESERVATION_TIMER_TASK_H_

TRM_BEGIN_NAMESPACE

class ReservationExpirationTimerTask : public TimerTask
{
public:
	ReservationExpirationTimerTask(Timer &timer, const std::string &reservationToken);
	~ReservationExpirationTimerTask(void);
    TimerTask *clone() {return new ReservationExpirationTimerTask(*this);};

	void run(void);
	Timer & getTimer(void);

private:
	Timer &timer;
	const std::string reservationToken;
};

class ReservationStartTimerTask : public TimerTask
{
public:
	ReservationStartTimerTask(Timer &timer, const std::string &reservationToken);
	~ReservationStartTimerTask(void);
    TimerTask *clone() {return new ReservationStartTimerTask(*this);};

	void run(void);
	Timer & getTimer(void);

private:
	Timer &timer;
	const std::string reservationToken;
};

class ReservationPreStartTimerTask : public TimerTask
{
public:
	ReservationPreStartTimerTask(Timer &timer, const std::string &reservationToken);
	~ReservationPreStartTimerTask(void);
    TimerTask *clone() {return new ReservationPreStartTimerTask(*this);};

	void run(void);
	Timer & getTimer(void);

private:
	Timer &timer;
	const std::string reservationToken;
};

class PendingRequestTimeoutTimerTask : public TimerTask
{
public:
	PendingRequestTimeoutTimerTask(Timer &timer, const std::string &uuid);
	~PendingRequestTimeoutTimerTask(void);
    TimerTask *clone() {return new PendingRequestTimeoutTimerTask(*this);};

	void run(void);
	Timer & getTimer(void);

private:
	Timer &timer;
	const std::string uuid;
};


TRM_END_NAMESPACE
#endif


/** @} */
/** @} */
