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


#ifndef TRM_JSON_DECODER_H_
#define TRM_JSON_DECODER_H_

#include <stdint.h>
#include <vector>

#include "TRM.h"
#include "MessageProcessor.h"

TRM_BEGIN_NAMESPACE

void JsonDecode(int handle, ReserveTuner & message);
void JsonDecode(int handle, ReserveTunerResponse & message);
void JsonDecode(int handle, ReleaseTunerReservation & message);
void JsonDecode(int handle, ValidateTunerReservation & message);
void JsonDecode(int handle, CancelRecording & message);
void JsonDecode(int handle, CancelLive & message);
void JsonDecode(int handle, ReleaseTunerReservationResponse & message);
void JsonDecode(int handle, ValidateTunerReservationResponse & message);
void JsonDecode(int handle, CancelRecordingResponse & message);
void JsonDecode(int handle, CancelLiveResponse & message);
void JsonDecode(int handle, GetAllTunerIds & message);
void JsonDecode(int handle, GetAllTunerIdsResponse & message);
void JsonDecode(int handle, GetAllTunerStates & message);
void JsonDecode(int handle, GetAllTunerStatesResponse & message);
void JsonDecode(int handle, GetAllReservations & message);
void JsonDecode(int handle, GetAllReservationsResponse & message);
void JsonDecode(int handle, GetAllConnectedDeviceIdsResponse & message);
void JsonDecode(int handle, GetVersion & message);
void JsonDecode(int handle, GetVersionResponse & message);
void JsonDecode(int handle, NotifyTunerReservationUpdate & message);
void JsonDecode(int handle, NotifyTunerReservationRelease & message);
void JsonDecode(int handle, NotifyTunerReservationConflicts & message);
void JsonDecode(int handle, NotifyTunerStatesUpdate & message);
void JsonDecode(int handle, NotifyTunerPretune & message);
void JsonDecode(int handle, NotifyClientConnectionEvent & message);
void JsonDecode(int handle, GetTRMConnectionEvents & message);
void JsonDecode(int handle, UpdateTunerActivityStatus & message);



extern int JsonDecode(const std::vector<uint8_t> &in, Enum<Klass> &meta);


class JsonDecoder
{
public:
	JsonDecoder(MessageProcessor & processor)
	: processor(processor){};
	void decode(std::vector<uint8_t> &in);
private:
	MessageProcessor &processor;
};


template<class MsgT>
void JsonDecode(int handle, MessageProcessor &processor)
{
	MsgT msg;
	JsonDecode(handle, msg);
	processor(msg);
}

TRM_END_NAMESPACE
#endif


/** @} */
/** @} */
