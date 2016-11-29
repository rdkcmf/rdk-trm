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


#include <list>
#include <string>

#include "trm/TRM.h"
#include "trm/TunerState.h"
#include "trm/TunerReservation.h"
#include "Tuner.h"
#include "Filter.h"
#include "Manager.h"


TRM_BEGIN_NAMESPACE
template<>

Tuner::IdList & Filter<ByTunerId>(const std::string &tunerId, Tuner::IdList &tuners)
{
	return Filter_ByTunerId(tunerId, tuners);
}

template<>
Tuner::IdList & Filter<ByTunerState>(const Enum<TunerState> &state, Tuner::IdList &tuners)
{
	return Filter_ByTunerState(state, tuners);
}

template<>
Tuner::IdList & Filter<ByTunerLocator>(const std::string &serviceLocator, Tuner::IdList &tuners)
{
	return Filter_ByTunerLocator(serviceLocator, tuners);
}

template<>
Tuner::IdList & Filter<ByTunerEndBefore>(const uint64_t &endTime, Tuner::IdList &tuners)
{
	return Filter_ByTunerEndBefore(endTime, tuners, Activity::kRecord);
}

template<>
TunerReservation::TokenList & Filter<ByActivity>(const Enum<Activity> &activity, TunerReservation::TokenList &reservations)
{
	return Filter_ByActivity(activity, reservations);

}

template<>
TunerReservation::TokenList & Filter<ByStartAfter>(const uint64_t &startTime, TunerReservation::TokenList &reservations)
{
	return Filter_ByStartAfter(startTime, reservations);

}

template<>
TunerReservation::TokenList & Filter<ByReservationEndBefore>(const uint64_t &endTime, TunerReservation::TokenList &reservations)
{
	return Filter_ByReservationEndBefore(endTime, reservations);
}

template<>
TunerReservation::TokenList & Filter<ByToken>(const std::string &token, TunerReservation::TokenList &reservations)
{
	return Filter_ReservationByToken(token, reservations);
}

template<>
TunerReservation::TokenList & Filter<ByDevice>(const std::string &device, TunerReservation::TokenList &reservations)
{
	return Filter_ReservationByDevice(device, reservations);
}

template<>
TunerReservation::TokenList & Filter<ByInConflict>(const bool &inConflict, TunerReservation::TokenList &reservations)
{
	return Filter_ReservationByAttribute(inConflict, reservations);
}

template<>
TunerReservation::TokenList & Filter<ByReservationLocator>(const std::string &serviceLocator, TunerReservation::TokenList &reservations)
{
	return Filter_ByReservationLocator(serviceLocator, reservations);
}

template<>
TunerReservation::TokenList & Filter<ByReservationState>(const int &state, TunerReservation::TokenList &reservations)
{
	std::cout << "Filter<ByReservationState>"  << std::endl;

	return Filter_ByReservationState(state, reservations);
}

Tuner::IdList & Filter_ByTunerId(const std::string &tunerId, Tuner::IdList &tuners)
{
	std::cout << "Filter Reservations ByTunerId"  << std::endl;
	Tuner::IdList::iterator it = tuners.begin();
	while(it != tuners.end()) {
		const Tuner &tuner = Manager::getInstance().getTuner(*it);
		if (tuner.getId().compare(tunerId) != 0) {
			it = tuners.erase(it);
		}
		else {
			it++;
		}
	}

	Assert(tuners.size() <= 1);
	return tuners;
}

Tuner::IdList & Filter_ByTunerState(const Enum<TunerState> &state, Tuner::IdList &tuners)
{
	std::cout << "Filter Tuners ByTunerState "  << tuners.size() << std::endl;
	Tuner::IdList::iterator it = tuners.begin();
	while(it != tuners.end()) {
		//std::cout << "checking tuner id " << *it << std::endl;
		const Tuner &tuner = Manager::getInstance().getTuner(*it);
		if (!(tuner.getState().getState() == state)) {
			it = tuners.erase(it);
		}
		else {
			it++;
		}
	}
	//std::cout << "After Filter has " << tuners.size() << " left " << std::endl;
	return tuners;
}

Tuner::IdList & Filter_ByTunerLocator(const std::string &serviceLocator, Tuner::IdList &tuners)
{
	std::cout << "Filter Tuners ByServiceLocator"  << std::endl;
	Tuner::IdList::iterator it = tuners.begin();
	while (it != tuners.end()) {
		const Tuner &tuner = Manager::getInstance().getTuner(*it);
		if ((!tuner.getServiceLocator().empty()) && (tuner.getServiceLocator().compare(serviceLocator) != 0)) {
			it = tuners.erase(it);
		}
		else {
			it++;
		}
	}
	return tuners;
}

Tuner::IdList & Filter_ByTunerEndBefore(const uint64_t &endTime, Tuner::IdList &tuners, const Activity &activity)
{
	/*
	 * Find out the latest expiration time on a tuner is prior to the endTime.
	 */
	std::cout << "Filter Reservations By ByEndBefore on Tuners"  << std::endl;

	Tuner::IdList::iterator it = tuners.begin();
	while (it != tuners.end()) {
		bool beyondEndTime = false;
		TunerReservation::TokenList reservationTokens;
		Manager::getInstance().getReservationTokens(reservationTokens, *it);
		TunerReservation::TokenList::iterator itt = reservationTokens.begin();
		while (itt != reservationTokens.end()) {
			TunerReservation &reservation = Manager::getInstance().getReservation(*itt);
			if ((reservation.getActivity() == activity) &&
			    ((reservation.getStartTime() + reservation.getDuration()) > endTime)) {
				 beyondEndTime = true;
			}

			itt++;
		}

		if (beyondEndTime) {
			std::cout << "Tuner " << Manager::getInstance().getTuner(*it).getId() << " goes beyond endTime " << endTime << std::endl;
			it = tuners.erase(it);
		}
		else {
			it++;
		}
	}
	return tuners;
}

TunerReservation::TokenList & Filter_ByActivity(const Enum<Activity> &activity, TunerReservation::TokenList &reservations)
{
	std::cout << "Filter Reservations By Activity start"  << std::endl;
	TunerReservation::TokenList::iterator it = reservations.begin();
	while (it != reservations.end()) {
		const TunerReservation &reservation = Manager::getInstance().getReservation(*it);
		if (!(reservation.getActivity().getActivity() == activity)) {
			it = reservations.erase(it);
		}
		else {
			it++;
		}
	}

	std::cout << "Filter Reservations By DONE"  << std::endl;

	return reservations;
}

TunerReservation::TokenList & Filter_ByStartAfter(const uint64_t &startTime, TunerReservation::TokenList &reservations)
{
	std::cout << "Filter Reservations By ByStartAfter"  << std::endl;
	TunerReservation::TokenList::iterator it = reservations.begin();
	while (it != reservations.end()) {
		const TunerReservation &reservation = Manager::getInstance().getReservation(*it);
		if (reservation.getStartTime() <= startTime) {
			it = reservations.erase(it);
		}
		else {
			it++;
		}
	}
	return reservations;
}

TunerReservation::TokenList & Filter_ByReservationEndBefore(const uint64_t &endTime, TunerReservation::TokenList &reservations)
{
	std::cout << "Filter Reservations By ByEndBefore on Reservations"  << std::endl;
	TunerReservation::TokenList::iterator it = reservations.begin();
	while (it != reservations.end()) {
		const TunerReservation &reservation = Manager::getInstance().getReservation(*it);
		if ((reservation.getStartTime() + reservation.getDuration()) > endTime) {
			it = reservations.erase(it);
		}
		else {
			it++;
		}
	}
	return reservations;
}

TunerReservation::TokenList & Filter_ReservationByToken(const std::string &token, TunerReservation::TokenList &reservations)
{
	std::cout << "Filter Reservations By ByToken"  << std::endl;
	TunerReservation::TokenList::iterator it = reservations.begin();
	while (it != reservations.end()) {
		const TunerReservation &reservation = Manager::getInstance().getReservation(*it);
		if (reservation.getReservationToken().compare(token) != 0) {
			it = reservations.erase(it);
		}
		else {
			it++;
		}
	}
	return reservations;
}

TunerReservation::TokenList & Filter_ReservationByDevice(const std::string &device, TunerReservation::TokenList &reservations)
{
	{
		TunerReservation::TokenList::iterator it;
		for (it = reservations.begin(); it != reservations.end(); it++) {
			std::cout << "Start with reservation " << (*it) << std::endl;
		}
	}
	std::cout << "Filter Reservations By ByDevice "  <<  reservations.size() << std::endl;
	TunerReservation::TokenList::iterator it = reservations.begin();
	while (it != reservations.end()) {
		const TunerReservation &reservation = Manager::getInstance().getReservation(*it);

		if (reservation.getDevice().compare(device) != 0) {
			it = reservations.erase(it);
		}
		else {
			it++;
		}
	}
	return reservations;
}

TunerReservation::TokenList & Filter_ReservationByAttribute(const bool &inConflict, TunerReservation::TokenList &reservations)
{
	{
		TunerReservation::TokenList::iterator it;
		for (it = reservations.begin(); it != reservations.end(); it++) {
			std::cout << "Start with reservation " << (*it) << std::endl;
		}
	}
	std::cout << "Filter Reservations By ByInConflict "  <<  reservations.size() << std::endl;
	TunerReservation::TokenList::iterator it = reservations.begin();
	while (it != reservations.end()) {

		if ((inConflict) && (!Manager::getInstance().isPendingConflict((*it)))) {
			it = reservations.erase(it);
		}
		else if((!inConflict) && (Manager::getInstance().isPendingConflict((*it)))) {
			it = reservations.erase(it);
		}
		else {
			it++;
		}
	}
	return reservations;
}

TunerReservation::TokenList & Filter_ByReservationLocator(const std::string &serviceLocator, TunerReservation::TokenList &reservations)
{
	std::cout << "Filter Reservations By ByServiceLocator"  << std::endl;
	TunerReservation::TokenList::iterator it = reservations.begin();
	while (it != reservations.end()) {
		const TunerReservation &reservation = Manager::getInstance().getReservation(*it);
		if (reservation.getServiceLocator().compare(serviceLocator) != 0) {
			it = reservations.erase(it);
		}
		else {
			it++;
		}
	}
	return reservations;
}

TunerReservation::TokenList & Filter_ByReservationState(int state, TunerReservation::TokenList &reservations)
{
	std::cout << "Filter Reservations " << reservations.size() << " By State "  << state << std::endl;
	TunerReservation::TokenList::iterator it = reservations.begin();
	while (it != reservations.end()) {
		const TunerReservation &reservation = Manager::getInstance().getReservation(*it);
		if (reservation.state != state) {
			std::cout << "Removing reservation " << reservation.getReservationToken() << " for being in state " << reservation.state << std::endl;
			it = reservations.erase(it);
		}
		else {
			it++;
		}
	}
	return reservations;
}

TRM_END_NAMESPACE



/** @} */
/** @} */
