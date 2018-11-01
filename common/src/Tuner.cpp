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


#include "trm/TRM.h"
#include "trm/TunerReservation.h"
#include "trm/Activity.h"
#include "trm/TunerState.h"

#include "ReservationCustomAttributes.h"
#include "Util.h"
#include "Tuner.h"


TRM_BEGIN_NAMESPACE

static bool isOverlap(uint64_t start1, uint64_t dur1, uint64_t start2, uint64_t dur2)
{
	bool overlap = true;
	if (((start1 + dur1) <= start2)|| ((start2 + dur2) <= start1)) {
		overlap = false;
	}

	return overlap;
}

Tuner::Tuner(const std::string & internalId)
: internalId(internalId)
{
	Assert(reservations.size() == 0);
}

TunerReservation::TokenList & Tuner::getReservationTokens(TunerReservation::TokenList & tokens) const
{
	ReservationCT::const_iterator it;
	for (it = reservations.begin(); it != reservations.end(); it++) {
		tokens.push_back(it->first);
	}
	Assert(reservations.size() <= tokens.size());

	return tokens;
}

const TunerReservation& Tuner::getReservation(const std::string &reservationToken) const
{
	ReservationCT::const_iterator it;
	it = reservations.find(reservationToken);
	if (it == reservations.end()) {
		//@TODO: Throw Exception.
		Assert(0);
	}

	return it->second;
}

TunerReservation& Tuner::getReservation(const std::string &reservationToken)
{
	ReservationCT::iterator it;
	it = reservations.find(reservationToken);
	if (it == reservations.end()) {
		//@TODO: Throw Exception.
		Assert(0);
	}

	return it->second;
}

const TunerReservation & Tuner::getReservation(const Activity &activity, int reservationState, const std::string &serviceLocator) const
{
	/*
	 * Construct state based on ACTIVE reservations.
	 */
	ReservationCT::const_iterator it = reservations.begin();
	while(it != reservations.end()) {
		if (it->second.state == reservationState) {
			if (it->second.getActivity() == activity) {
				if (serviceLocator.empty() || (serviceLocator.compare(it->second.getServiceLocator()) == 0)) {
					break;
				}
			}
		}
		it++;
	}

	if (it == reservations.end()) {
		throw ItemNotFoundException();
	}

	return it->second;
}


const TunerState Tuner::getState(void) const
{
	/*
	 * Construct state based on ACTIVE reservations.
	 */
	TunerState state = TunerState::kFree;
	ReservationCT::const_iterator it = reservations.begin();
	while(it != reservations.end()) {
		if (it->second.state == TunerReservation::ACTIVE) {
			if (it->second.getStartTime() > GetCurrentEpoch()) {
				std::cout << "WARN::" << it->second.getReservationToken()
						  << " has start Time " << it->second.getStartTime()
						  << "vs current " << GetCurrentEpoch() << std::endl;
			}

			if ((it->second.getStartTime() + it->second.getDuration()) < GetCurrentEpoch()) {
				std::cout << "WARN:: " << it->second.getReservationToken()
						  << " has end Time " << (it->second.getStartTime() + it->second.getDuration())
						  << "vs current " << GetCurrentEpoch() << std::endl;
			}

//			SafeAssert(it->second.getStartTime() <= GetCurrentEpoch());
//			SafeAssert((it->second.getStartTime() + it->second.getDuration()) >= GetCurrentEpoch());

			if (it->second.getActivity() == Activity::kLive) {
				state = state + Activity::kLive;
			}
			else if (it->second.getActivity() == Activity::kRecord) {
				state = state + Activity::kRecord;
			}
		}
		it++;
	}

	std::cout << "Tuner::getState() returning " << (const char *) (state.getState()) << std::endl;
	return state;
}

void Tuner::setState(const TunerState &state)
{
//	this->state = state;
}

const std::string Tuner::getServiceLocator(bool considerFutureToken) const
{
	/* Return the active token's locator, which are guaranteed to match,
	 * or return the nearest pending token's locator.
	 */
	std::string serviceLocator = "";

	if (serviceLocator.empty()){
		/* First look for active locator */
		ReservationCT::const_iterator it = reservations.begin();
		while(it != reservations.end()) {
			if (it->second.state == TunerReservation::ACTIVE) {
				serviceLocator = it->second.getServiceLocator();
				break;
			}
			it++;
		}
	}

	if (serviceLocator.empty() && considerFutureToken){
		Assert(getState().getState() == TunerState::kFree);
		/* Then look for nearest token's locator */
		ReservationCT::const_iterator it = reservations.begin();
		uint64_t startTime = std::numeric_limits<uint64_t>::max();
		while(it != reservations.end()) {
			if (it->second.getStartTime() < startTime) {
				startTime = it->second.getStartTime();
				serviceLocator = it->second.getServiceLocator();
			}
			it++;
		}
	}

    return serviceLocator;
}

uint64_t Tuner::getEndTime(const Activity &activity) const
{
	/* Return last token's expiration time.
	 */
	uint64_t endTime = 0;

	/* First look for active locator */
	ReservationCT::const_iterator it = reservations.begin();
	while(it != reservations.end() && it->second.getActivity() == activity ) {
		if ((it->second.getStartTime() + it->second.getDuration()) > endTime) {
			endTime = it->second.getStartTime() + it->second.getDuration();
		}
		it++;
	}


    return endTime;
}

const std::string &Tuner::getId(void) const {
	return internalId;
}

void Tuner::addReservation(const TunerReservation & reservation)
{
	std::cout << "Enter addReservation" << std::endl;

	/*
	 * TunerReservation::EXPIRED is only transicent token state when duration expires.
	 */
	SafeAssert(assertTunerState(TunerReservation::EXPIRED, 0, 0));

	/* Reservations of same tuner must have same service locator */
	if (getState() == TunerState::kFree) {
		SafeAssert(assertTunerState(TunerReservation::ACTIVE, 0, 0));
	}
	else if (getState() == TunerState::kHybrid) {
	}
	else {
		//When 1st recording expires at the same time the 2nd recording starts, the
		//Timer Start and Timer Stop may be in random order. In case that Timer start
		//Arrives before Expiration, the tuner may have two active reservations.
		SafeAssert(assertTunerState(TunerReservation::ACTIVE, 1, 2));
	}

	/* First Make sure that the reservation does not yet exist */
	ReservationCT::const_iterator it = reservations.find(reservation.getReservationToken());
	if (it != reservations.end()) {
		std::cout << "Tuner Reservation  " << reservation.getReservationToken() << " Already Exist!" << std::endl;
	}

	if (getState() != TunerState::kFree) {
		Assert(reservations.size() > 0);
		ReservationCT::const_iterator it = reservations.begin();
		/* if two reservations overlap, they should have same source id */
		while(it != reservations.end()) {
			if (isOverlap(it->second.getStartTime(), it->second.getDuration(), reservation.getStartTime(), reservation.getDuration())) {
				std::cout << "one locator is " << it->second.getServiceLocator() << std::endl;
				std::cout << "other locator is " << reservation.getServiceLocator() << std::endl;
				/* Use SafeAssert to issue warning. Different Locator is allowed between L and R
				 * L will be force tunen to R when hybrid  mode starts.
				 */
				if (it->second.getActivity() == reservation.getActivity()) {
				Assert(it->second.getServiceLocator().compare(reservation.getServiceLocator()) == 0);
				}
				else {
					/* A tuner may temporary holding tow different locator, one for record, one for live.
					 * but they will be sync'd upon record start.
					 */
				}
			}
			it++;
		}
	}

	std::cout << "Reservation to Add is new" << std::endl;

	std::cout << " Current Tuner State is " << (const char *) getState().getState()
			  << " Activity requested is "  << (const char *) reservation.getActivity().getActivity() << std::endl;

#if 1
	/* Schedule to update the tuner state at reservation start time. */
#else
	/* else do the state now... wont work if we allow future reservations */
	state = state + reservation.getActivity().getActivity();
	std::cout << "New Tuner State is " << (const char *) state.getState() << std::endl;
#endif
	reservations[reservation.getReservationToken()]  =  reservation;

	{

		int count = 0;
		ReservationCT::const_iterator it = reservations.begin();
		while(it != reservations.end()) {
			if (it->second.state == TunerReservation::ACTIVE) {
				count++;
			}
			it++;
		}
		Assert(count <= 2);
	}

	std::cout << " Reservation Added Successfully : " << reservation.getReservationToken() << std::endl;
}

void Tuner::releaseReservation(const std::string & reservationToken)
{
	std::cout << "Enter releaseReservation" << std::endl;

	/* First Make sure that the reservation does not yet exist */
	ReservationCT::iterator it = reservations.find(reservationToken);
	if (it != reservations.end()) {
		TunerReservation & removedReservation = it->second;
		printf("Releasing Reservation %p\r\n", &removedReservation);

		/* Reservations of same tuner must have same service locator */
		if (getState() == TunerState::kFree) {
			SafeAssert(assertTunerState(TunerReservation::ACTIVE, 0, 0));
			SafeAssert(assertTunerState(TunerReservation::EXPIRED, 0, 1));
		}
		else if (getState() == TunerState::kHybrid) {
		}
		else {
			//When 1st recording expires at the same time the 2nd recording starts, the
			//Timer Start and Timer Stop may be in random order. In case that Timer start
			//Arrives before Expiration, the tuner may have two active reservations.
			SafeAssert(assertTunerState(TunerReservation::ACTIVE, 1, 2));
			SafeAssert(assertTunerState(TunerReservation::EXPIRED, 0, 1));
		}

		Assert((removedReservation).getReservationToken().compare(reservationToken) == 0);
        extern void delete_ReservationCustomAttributes(void *p);
        delete_ReservationCustomAttributes((void *)removedReservation.getCustomAttributes());

#if 0
		if (removedReservation.state == TunerReservation::IDLE) {
			std::cout << "Removing Future Reservation " << std::endl;
		} else if (removedReservation.state == TunerReservation::ACTIVE) {
			std::cout << "Removing Current Reservation " << std::endl;
			state = state - removedReservation.getActivity().getActivity();
		}
		else {
			std::cout << " Removing Expired Reservation " << std::endl;
			state = state - removedReservation.getActivity().getActivity();
		}
#endif
		reservations.erase(it);

	}
}

bool Tuner::assertTunerState(int state, int lowMark, int highMark)
{
	int count = 0;
	bool ok = false;
	ReservationCT::const_iterator it = reservations.begin();
	while(it != reservations.end()) {
		if (it->second.state == state) {
			count++;
		}
		it++;
	}
	if (count >= lowMark && count <= highMark) {
		ok = true;
	}
	else {
		std::cout << "TUner has " << count  << " reservations in state " << state << " vs " << lowMark << ", " << highMark << std::endl;
		ok = false;
	}
	return(ok);
}


TRM_END_NAMESPACE


/** @} */
/** @} */
