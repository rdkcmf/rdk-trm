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


#ifndef TRM_KLASS_H_
#define TRM_KLASS_H_

#include <string>
#include <memory>

#include "TRM.h"
#include "Enum.h"

TRM_BEGIN_NAMESPACE

class Klass
{
public:
public:
	typedef  int   EnumType;

	static const Enum<Klass> kUnknown;
	static const Enum<Klass> kDetails;
	static const Enum<Klass> kActivity;
	static const Enum<Klass> kTunerReservation;
	static const Enum<Klass> kTunerState;
	static const Enum<Klass> kResponseStatus;

	static const Enum<Klass> kMessageBase;
	static const Enum<Klass> kNoResponse;
	static const Enum<Klass> kRequestBase;
	static const Enum<Klass> kResponseBase;
	static const Enum<Klass> kNotificationBase;
	static const Enum<Klass> kSimpleTRMRequest;
	static const Enum<Klass> kSimpleTRMResponse;

	static const Enum<Klass> kReserveTuner;
	static const Enum<Klass> kReserveTunerResponse;

	static const Enum<Klass> kReleaseTunerReservation;
	static const Enum<Klass> kReleaseTunerReservationResponse;

	static const Enum<Klass> kValidateTunerReservation;
	static const Enum<Klass> kValidateTunerReservationResponse;

	static const Enum<Klass> kCancelRecording;
	static const Enum<Klass> kCancelRecordingResponse;

	static const Enum<Klass> kCancelLive;
	static const Enum<Klass> kCancelLiveResponse;

	static const Enum<Klass> kGetAllTunerIds;
	static const Enum<Klass> kGetAllTunerIdsResponse;
	static const Enum<Klass> kGetAllTunerStates;
	static const Enum<Klass> kGetAllTunerStatesResponse;
	static const Enum<Klass> kGetAllReservations;
	static const Enum<Klass> kGetAllReservationsResponse;
	static const Enum<Klass> kGetVersion;
	static const Enum<Klass> kGetVersionResponse;
	static const Enum<Klass> kGetAllConnectedDeviceIdsResponse;

	static const Enum<Klass> kNotifyTunerReservationUpdate;
	static const Enum<Klass> kNotifyTunerReservationRelease;
	static const Enum<Klass> kNotifyTunerReservationConflicts;
	static const Enum<Klass> kNotifyTunerStatesUpdate;
	static const Enum<Klass> kNotifyTunerPretune;
	static const Enum<Klass> kNotifyClientConnectionEvent;
		
	static const Enum<Klass> kGetTRMConnectionEvents;	

	static const std::vector<const Enum<Klass> * > & getEnums(void);

	const Enum<Klass> & getClass(void) const {return klass;}

	Klass(const Enum<Klass> &klass = kUnknown);
	Klass(const char *);
	~Klass(void);

private:
	Enum<Klass> klass;
};

TRM_END_NAMESPACE
#endif


/** @} */
/** @} */
