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


#include "trm/TRM.h"
#include "trm/JsonEncoder.h"

#include "Manager.h"
#include "Server.h"
#include "Util.h"
#include "ReservationTimerTask.h"
#include "Connection.h"

extern void printThread(const char *func);
extern TRM::Server *serverInstance;

TRM_BEGIN_NAMESPACE

ReservationExpirationTimerTask::ReservationExpirationTimerTask(Timer &timer, const std::string &reservationToken)
: timer(timer), reservationToken(reservationToken) {}

ReservationExpirationTimerTask::~ReservationExpirationTimerTask(void) {}

void ReservationExpirationTimerTask::run(void)
{
	Log() << "Reservation " << reservationToken << "  Timer "
			<< timer.getToken()
			<< " Expired at " << time(0) << std::endl;

	TunerReservation &expired = Manager::getInstance().getReservation(reservationToken);
	try
	{
		Assert(expired.state == TunerReservation::ACTIVE);
		const ReservationAttributes &attrs = Manager::getInstance().getReservationAttributes(reservationToken);
		try {
			const Connection &connection = ::serverInstance->getConnection(attrs.clientId);
			NotifyTunerReservationRelease notification(GenerateUUID(), reservationToken, "expired");
			{
				std::vector<uint8_t> out;
				SerializeMessage(notification, attrs.clientId, out);
				connection.sendAsync(out);
			}
		}
		catch(...) {

		}
	}
	catch(...) {
		Log() << "Reservation released failed: " << std::endl;
	}
	Log() << "Reservation Expired : " << std::endl;
	expired.print();
	expired.state = TunerReservation::EXPIRED;
	Manager::getInstance().releaseReservation(reservationToken);
};

Timer & ReservationExpirationTimerTask::getTimer(void)
{
	return timer;
}


ReservationStartTimerTask::ReservationStartTimerTask(Timer &timer, const std::string &reservationToken)
: timer(timer), reservationToken(reservationToken) {}

ReservationStartTimerTask::~ReservationStartTimerTask(void) {}

void ReservationStartTimerTask::run(void)
{
	Log() << "Reservation " << reservationToken << "  Start Timer "
			<< timer.getToken()
			<< " Started at " << time(0) << std::endl;

	TunerReservation &started = Manager::getInstance().getReservation(reservationToken);
	try
	{
		Assert(started.state == TunerReservation::IDLE);
		printf("Starting Reservation %p\r\n", &started);

		/* Update the state of the tuner */
		Tuner & tuner = Manager::getInstance().getTuner(Manager::getInstance().getParent(reservationToken));
#if 0
		TunerState state = tuner.getState();
		state = state + started.getActivity().getActivity();
		tuner.setState(state);
#else
		/*Tuner state is calculated dynamically based on the currentn token held at the time of getState() call. */
#endif

		started.state = TunerReservation::ACTIVE;
		Log() << "New Tuner State is " << (const char *) tuner.getState().getState() << std::endl;

		/* If tuner state is hybrid, make sure the sourceIds of LIVE and RECORD are same */
		if (tuner.getState().getState() == TunerState::kHybrid) {
			Manager::getInstance().syncHybrid(tuner.getId(), reservationToken);
		}

		Manager::getInstance().startReservation(reservationToken);
		Manager::getInstance().adjustExpireTimer(reservationToken);
	}
	catch(...) {
		Log() << "Reservation start failed : " << std::endl;
	}
	Log() << "Reservation started : " << std::endl;
	started.print();
};

Timer & ReservationStartTimerTask::getTimer(void)
{
	return timer;
}


ReservationPreStartTimerTask::ReservationPreStartTimerTask(Timer &timer, const std::string &reservationToken)
: timer(timer), reservationToken(reservationToken) {}

ReservationPreStartTimerTask::~ReservationPreStartTimerTask(void) {}

void ReservationPreStartTimerTask::run(void)
{
	Log() << "Reservation " << reservationToken << "  PreStart Timer "
			<< timer.getToken()
			<< " Started at " << time(0) << std::endl;

	TunerReservation &started = Manager::getInstance().getReservation(reservationToken);
	try
	{
		Assert(started.state == TunerReservation::IDLE);
		printf("About to start Reservation %p\r\n", &started);

		/* Update the state of the tuner */
		Tuner & tuner = Manager::getInstance().getTuner(Manager::getInstance().getParent(reservationToken));
		Log() << "Current Tuner State is " << (const char *) tuner.getState().getState() << std::endl;

		/* If tuner state is hybrid, make sure the sourceIds of LIVE and RECORD are same */
		if ( (started.getActivity().getActivity() == Activity::kRecord)  && (tuner.getState().getState() == TunerState::kHybrid)) {
			Manager::getInstance().prepareRecordHandover(tuner.getId(), reservationToken);
		}
	}
	catch(...) {
		Log() << "Reservation start failed : " << std::endl;
	}
	Log() << "Reservation is about to start: " << std::endl;
	started.print();
};

Timer & ReservationPreStartTimerTask::getTimer(void)
{
	return timer;
}

PendingRequestTimeoutTimerTask::PendingRequestTimeoutTimerTask(Timer &timer, const std::string &uuid)
: timer(timer), uuid(uuid) {}

PendingRequestTimeoutTimerTask::~PendingRequestTimeoutTimerTask(void) {}

void PendingRequestTimeoutTimerTask::run(void)
{
	/* If PendingReservation is record, grant reservation */
	try
	{

		PendingRequestProcessor &pending = Manager::getInstance().getPendingRequest(timer.getToken());

		Log() << "PendingRequest [type=" << pending.getType() << "] " << "uuid=" << pending.getUUID() << " "
				<< "Timer " << timer.getToken() << " timeout" << std::endl;

		pending.timeout();
		Manager::getInstance().removePendingRequest(pending.getUUID());
		delete &pending;
	}
	catch(...) {
	}
};

Timer & PendingRequestTimeoutTimerTask::getTimer(void)
{
	return timer;
}
TRM_END_NAMESPACE


/** @} */
/** @} */
