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


#include <map>

#include <iostream>
#include "trm/TRM.h"
#include "trm/Klass.h"

LOCAL_BEGIN_NAMESPACE

enum  {
	unknown = 0,
	details,
	activity,
	tunerReservation,
	tunerState,
	responseStatus,

	messageBase,
	noResponse,
	requestBase,
	responseBase,
	notificationBase,
	simpleTRMRequest,
	simpleTRMResponse,

	reserveTuner,
	reserveTunerResponse,

	releaseTunerReservation,
	releaseTunerReservationResponse,

	validateTunerReservation,
	validateTunerReservationResponse,

	cancelRecording,
	cancelRecordingResponse,

	cancelLive,
	cancelLiveResponse,

	getAllTunerIds,
	getAllTunerIdsResponse,
	getAllTunerStates,
	getAllTunerStatesResponse,
	getAllReservations,
	getAllReservationsResponse,
	getVersion,
	getVersionResponse,
	getAllConnectedDeviceIdsResponse,

	notifyTunerReservationUpdate,
	notifyTunerReservationRelease,
	notifyTunerReservationConflict,
	notifyTunerStatesUpdate,
	notifyTunerPretune,
	notifyClientConnectionEvent,

	getTRMConnectionEvents,
	updateTunerActivityStatus,
	updateTunerActivityStatusResponse,

	MAX_ENUM_NUMBER
};

LOCAL_END_NAMESPACE

TRM_BEGIN_NAMESPACE

const Enum<Klass> Klass::kUnknown								(MAKE_PAIR(unknown));
const Enum<Klass> Klass::kDetails								(MAKE_PAIR(details));
const Enum<Klass> Klass::kActivity								(MAKE_PAIR(activity));
const Enum<Klass> Klass::kTunerReservation						(MAKE_PAIR(tunerReservation));
const Enum<Klass> Klass::kTunerState							(MAKE_PAIR(tunerState));
const Enum<Klass> Klass::kResponseStatus						(MAKE_PAIR(responseStatus));

const Enum<Klass> Klass::kMessageBase 							(MAKE_PAIR(messageBase));
const Enum<Klass> Klass::kNoResponse 							(MAKE_PAIR(noResponse));
const Enum<Klass> Klass::kRequestBase 							(MAKE_PAIR(requestBase));
const Enum<Klass> Klass::kResponseBase 							(MAKE_PAIR(responseBase));
const Enum<Klass> Klass::kNotificationBase 						(MAKE_PAIR(notificationBase));
const Enum<Klass> Klass::kSimpleTRMRequest 						(MAKE_PAIR(simpleTRMRequest));
const Enum<Klass> Klass::kSimpleTRMResponse 					(MAKE_PAIR(simpleTRMResponse));

const Enum<Klass> Klass::kReserveTuner							(MAKE_PAIR(reserveTuner));
const Enum<Klass> Klass::kReserveTunerResponse					(MAKE_PAIR(reserveTunerResponse));

const Enum<Klass> Klass::kReleaseTunerReservation				(MAKE_PAIR(releaseTunerReservation));
const Enum<Klass> Klass::kReleaseTunerReservationResponse		(MAKE_PAIR(releaseTunerReservationResponse));

const Enum<Klass> Klass::kValidateTunerReservation				(MAKE_PAIR(validateTunerReservation));
const Enum<Klass> Klass::kValidateTunerReservationResponse		(MAKE_PAIR(validateTunerReservationResponse));

const Enum<Klass> Klass::kCancelRecording						(MAKE_PAIR(cancelRecording));
const Enum<Klass> Klass::kCancelRecordingResponse				(MAKE_PAIR(cancelRecordingResponse));

const Enum<Klass> Klass::kCancelLive							(MAKE_PAIR(cancelLive));
const Enum<Klass> Klass::kCancelLiveResponse					(MAKE_PAIR(cancelLiveResponse));

const Enum<Klass> Klass::kGetAllTunerIds						(MAKE_PAIR(getAllTunerIds));
const Enum<Klass> Klass::kGetAllTunerIdsResponse				(MAKE_PAIR(getAllTunerIdsResponse));
const Enum<Klass> Klass::kGetAllTunerStates						(MAKE_PAIR(getAllTunerStates));
const Enum<Klass> Klass::kGetAllTunerStatesResponse				(MAKE_PAIR(getAllTunerStatesResponse));
const Enum<Klass> Klass::kGetAllReservations					(MAKE_PAIR(getAllReservations));
const Enum<Klass> Klass::kGetAllReservationsResponse			(MAKE_PAIR(getAllReservationsResponse));
const Enum<Klass> Klass::kGetVersion					        (MAKE_PAIR(getVersion));
const Enum<Klass> Klass::kGetVersionResponse        			(MAKE_PAIR(getVersionResponse));

const Enum<Klass> Klass::kGetAllConnectedDeviceIdsResponse      (MAKE_PAIR(getAllConnectedDeviceIdsResponse));


const Enum<Klass> Klass::kNotifyTunerReservationUpdate			(MAKE_PAIR(notifyTunerReservationUpdate));
const Enum<Klass> Klass::kNotifyTunerReservationRelease			(MAKE_PAIR(notifyTunerReservationRelease));
const Enum<Klass> Klass::kNotifyTunerReservationConflicts		(MAKE_PAIR(notifyTunerReservationConflict));
const Enum<Klass> Klass::kNotifyTunerStatesUpdate				(MAKE_PAIR(notifyTunerStatesUpdate));
const Enum<Klass> Klass::kNotifyTunerPretune 					(MAKE_PAIR(notifyTunerPretune));
const Enum<Klass> Klass::kNotifyClientConnectionEvent			(MAKE_PAIR(notifyClientConnectionEvent));

const Enum<Klass> Klass::kGetTRMConnectionEvents      			(MAKE_PAIR(getTRMConnectionEvents));
const Enum<Klass> Klass::kUpdateTunerActivityStatus                        (MAKE_PAIR(updateTunerActivityStatus));
const Enum<Klass> Klass::kUpdateTunerActivityStatusResponse                        (MAKE_PAIR(updateTunerActivityStatusResponse));


const std::vector<const Enum<Klass> * > & Klass::getEnums(void)
{
	static std::vector<const Enum<Klass> * >  enums_;
	if (enums_.size() == 0) {
		enums_.push_back(&Klass::kUnknown);
		enums_.push_back(&Klass::kDetails);
		enums_.push_back(&Klass::kActivity);
		enums_.push_back(&Klass::kTunerReservation);
		enums_.push_back(&Klass::kTunerState);
		enums_.push_back(&Klass::kResponseStatus);

		enums_.push_back(&Klass::kMessageBase);
		enums_.push_back(&Klass::kNoResponse);
		enums_.push_back(&Klass::kRequestBase);
		enums_.push_back(&Klass::kResponseBase);
		enums_.push_back(&Klass::kNotificationBase);
		enums_.push_back(&Klass::kSimpleTRMRequest);
		enums_.push_back(&Klass::kSimpleTRMResponse);

		enums_.push_back(&Klass::kReserveTuner);
		enums_.push_back(&Klass::kReserveTunerResponse);

		enums_.push_back(&Klass::kReleaseTunerReservation);
		enums_.push_back(&Klass::kReleaseTunerReservationResponse);

		enums_.push_back(&Klass::kValidateTunerReservation);
		enums_.push_back(&Klass::kValidateTunerReservationResponse);

		enums_.push_back(&Klass::kCancelRecording);
		enums_.push_back(&Klass::kCancelRecordingResponse);

		enums_.push_back(&Klass::kCancelLive);
		enums_.push_back(&Klass::kCancelLiveResponse);

		enums_.push_back(&Klass::kGetAllTunerIds);
		enums_.push_back(&Klass::kGetAllTunerIdsResponse);

		enums_.push_back(&Klass::kGetAllTunerStates);
		enums_.push_back(&Klass::kGetAllTunerStatesResponse);

		enums_.push_back(&Klass::kGetAllReservations);
		enums_.push_back(&Klass::kGetAllReservationsResponse);

		enums_.push_back(&Klass::kGetAllConnectedDeviceIdsResponse);


		enums_.push_back(&Klass::kGetVersion);
		enums_.push_back(&Klass::kGetVersionResponse);

		enums_.push_back(&Klass::kNotifyTunerReservationUpdate);
		enums_.push_back(&Klass::kNotifyTunerReservationRelease);
		enums_.push_back(&Klass::kNotifyTunerReservationConflicts);
		enums_.push_back(&Klass::kNotifyTunerStatesUpdate);
		enums_.push_back(&Klass::kNotifyTunerPretune);
		enums_.push_back(&Klass::kNotifyClientConnectionEvent);

		enums_.push_back(&Klass::kGetTRMConnectionEvents);

		enums_.push_back(&Klass::kUpdateTunerActivityStatus);
		enums_.push_back(&Klass::kUpdateTunerActivityStatusResponse);

		Assert(enums_.size() == MAX_ENUM_NUMBER);
	};

	return enums_;
}

Klass::Klass(const Enum<Klass> &klass)
: klass(klass)
{
}

Klass::Klass(const char *name)
: klass(Enum<Klass>::at(name))
{
}

Klass::~Klass(void)
{
}

TRM_END_NAMESPACE



/** @} */
/** @} */
