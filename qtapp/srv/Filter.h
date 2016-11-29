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


#ifndef _TRM_RESERVATION_FILTER_H
#define _TRM_RESERVATION_FILTER_H

#include <list>
#include <string>

#include "trm/TRM.h"
#include "trm/TunerState.h"
#include "trm/TunerReservation.h"
#include "Tuner.h"

LOCAL_BEGIN_NAMESPACE
enum {
	ByNone,
	ByTunerId,
	ByTunerState,
	ByTunerLocator,
	ByReservationLocator,

	ByStartAfter,
	ByTunerEndBefore,
	ByReservationEndBefore,
	ByReservationState,

	ByActivity,
	ByToken,

	ByDevice,
	ByInConflict,
};

LOCAL_END_NAMESPACE

TRM_BEGIN_NAMESPACE

extern Tuner::IdList & Filter_ByTunerId(const std::string &tunerId, Tuner::IdList &tuners);
extern Tuner::IdList & Filter_ByTunerState(const Enum<TunerState> &state, Tuner::IdList &tuners);
extern Tuner::IdList & Filter_ByTunerLocator(const std::string &serviceLocator, Tuner::IdList &tuners);
extern Tuner::IdList & Filter_ByTunerEndBefore(const uint64_t &endTime, Tuner::IdList &tuners, const Activity &activity);

extern TunerReservation::TokenList & Filter_ByActivity(const Enum<Activity> &activity, TunerReservation::TokenList &reservations);
extern TunerReservation::TokenList & Filter_ByStartAfter(const uint64_t &startTime, TunerReservation::TokenList &reservations);
extern TunerReservation::TokenList & Filter_ReservationByToken(const std::string &token, TunerReservation::TokenList &reservations);
extern TunerReservation::TokenList & Filter_ReservationByDevice(const std::string &device, TunerReservation::TokenList &reservations);
extern TunerReservation::TokenList & Filter_ReservationByAttribute(const bool &inConflict, TunerReservation::TokenList &reservations);
extern TunerReservation::TokenList & Filter_ByReservationLocator(const std::string &serviceLocator, TunerReservation::TokenList &reservations);
extern TunerReservation::TokenList & Filter_ByReservationEndBefore(const uint64_t &endTime, TunerReservation::TokenList &reservations);
extern TunerReservation::TokenList & Filter_ByReservationState(int state, TunerReservation::TokenList &reservations);

template<int i, typename T1, class T2>
T2 & Filter(const T1 &, T2 &t2) {
	Assert(0);
	return t2;
}

template<>
Tuner::IdList & Filter<ByTunerId>(const std::string &tunerId, Tuner::IdList &tuners);
template<>
Tuner::IdList & Filter<ByTunerState>(const Enum<TunerState> &state, Tuner::IdList &tuners);
template<>
Tuner::IdList & Filter<ByTunerLocator>(const std::string &serviceLocator, Tuner::IdList &tuners);
template<>
Tuner::IdList & Filter<ByTunerEndBefore>(const uint64_t &endTime, Tuner::IdList &tuners);

template<>
TunerReservation::TokenList & Filter<ByActivity>(const Enum<Activity> &activity, TunerReservation::TokenList &reservations);
template<>
TunerReservation::TokenList & Filter<ByStartAfter>(const uint64_t &startTime, TunerReservation::TokenList &reservations);
template<>
TunerReservation::TokenList & Filter<ByReservationEndBefore>(const uint64_t &endTime, TunerReservation::TokenList &reservations);
template<>
TunerReservation::TokenList & Filter<ByToken>(const std::string &token, TunerReservation::TokenList &reservations);
template<>
TunerReservation::TokenList & Filter<ByDevice>(const std::string &device, TunerReservation::TokenList &reservations);
template<>
TunerReservation::TokenList & Filter<ByInConflict>(const bool &inConflict, TunerReservation::TokenList &reservations);
template<>
TunerReservation::TokenList & Filter<ByReservationLocator>(const std::string &serviceLocator, TunerReservation::TokenList &reservations);
template<>
TunerReservation::TokenList & Filter<ByReservationState>(const int &state, TunerReservation::TokenList &reservations);

TRM_END_NAMESPACE

#endif


/** @} */
/** @} */
