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


#ifndef _TRM_TUNER_H
#define _TRM_TUNER_H

#include <string>
#include <vector>
#include <list>
#include <map>

#include "trm/TunerReservation.h"
#include "trm/Activity.h"
#include "trm/TunerState.h"

TRM_BEGIN_NAMESPACE

class Tuner
{
public:
	typedef std::list<std::string> IdList;

	typedef std::map<std::string, TunerReservation> ReservationCT;
	Tuner(const std::string & internalId);
	const std::string &getId(void) const;
	uint64_t getEndTime(const Activity &activity) const;

	TunerReservation::TokenList & getReservationTokens(TunerReservation::TokenList & tokens) const;
	const TunerReservation &getReservation(const std::string &reservationToken) const;
	TunerReservation &getReservation(const std::string &reservationToken);
	const TunerReservation &getReservation(const Activity &activity, int reservationState, const std::string &serviceLocator="") const;

	const TunerState getState(void) const;
	void setState(const TunerState &state);

	const std::string getServiceLocator(bool considerFutureToken = true) const;

	void addReservation(const TunerReservation & reservation);
	void releaseReservation(const std::string & reservationToken);

private:
	bool assertTunerState(int state, int lowMark, int highMark);
	std::string internalId;
	ReservationCT reservations;
};

TRM_END_NAMESPACE

#endif


/** @} */
/** @} */
