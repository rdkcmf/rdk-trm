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


#include "trm/Klass.h"
#include "trm/JsonDecoder.h"
#include "Util.h"	

TRM_BEGIN_NAMESPACE


void JsonDecoder::decode(std::vector<uint8_t> &in)
{
	Log() << "Decoding " << in.size() << " bytes" << std::endl;
	Enum<Klass> klass(Klass::kUnknown);

	//@TODO: JsonDecode(in, klass) may err for malformed json message.
	int handle = JsonDecode(in, klass);

	if (handle == 0) return;

	if (klass == Klass::kReserveTuner) {
		JsonDecode<ReserveTuner>(handle, processor);
	}
	else
	if (klass == Klass::kReserveTunerResponse) {
		JsonDecode<ReserveTunerResponse>(handle, processor);
	}
	else
	if (klass == Klass::kReleaseTunerReservation) {
		JsonDecode<ReleaseTunerReservation>(handle, processor);
	}
	else
	if (klass == Klass::kReleaseTunerReservationResponse) {
		JsonDecode<ReleaseTunerReservationResponse>(handle, processor);
	}
	else
	if (klass == Klass::kValidateTunerReservation) {
		JsonDecode<ValidateTunerReservation>(handle, processor);
	}
	else
	if (klass == Klass::kValidateTunerReservationResponse) {
		JsonDecode<ValidateTunerReservationResponse>(handle, processor);
	}
	else
	if (klass == Klass::kCancelRecording) {
		JsonDecode<CancelRecording>(handle, processor);
	}
	else
	if (klass == Klass::kCancelRecordingResponse) {
		JsonDecode<CancelRecordingResponse>(handle, processor);
	}
	else
	if (klass == Klass::kCancelLive) {
		JsonDecode<CancelLive>(handle, processor);
	}
	else
	if (klass == Klass::kCancelLiveResponse) {
		JsonDecode<CancelLiveResponse>(handle, processor);
	}
	else
	if (klass == Klass::kGetAllTunerIds) {
		JsonDecode<GetAllTunerIds>(handle, processor);
	}
	else
	if (klass == Klass::kGetAllTunerIdsResponse) {
		JsonDecode<GetAllTunerIdsResponse>(handle, processor);
	}
	else
	if (klass == Klass::kGetAllTunerStates) {
		JsonDecode<GetAllTunerStates>(handle, processor);
	}
	else
	if (klass == Klass::kGetAllTunerStatesResponse) {
		JsonDecode<GetAllTunerStatesResponse>(handle, processor);
	}
	else
	if (klass == Klass::kGetAllReservations) {
		JsonDecode<GetAllReservations>(handle, processor);
	}
	else
	if (klass == Klass::kGetAllReservationsResponse) {
		JsonDecode<GetAllReservationsResponse>(handle, processor);
	}
	else
	if (klass == Klass::kGetAllConnectedDeviceIdsResponse) {
		JsonDecode<GetAllConnectedDeviceIdsResponse>(handle, processor);
	}
	else
	if (klass == Klass::kGetVersion) {
		JsonDecode<GetVersion>(handle, processor);
	}
	else
	if (klass == Klass::kGetVersionResponse) {
		JsonDecode<GetVersionResponse>(handle, processor);
	}
	else
	if (klass == Klass::kNotifyTunerReservationUpdate) {
		JsonDecode<NotifyTunerReservationUpdate>(handle, processor);
	}
	else
	if (klass == Klass::kNotifyTunerReservationRelease) {
		JsonDecode<NotifyTunerReservationRelease>(handle, processor);
	}
	else
	if (klass == Klass::kNotifyTunerReservationConflicts) {
		JsonDecode<NotifyTunerReservationConflicts>(handle, processor);
	}
	else
	if (klass == Klass::kNotifyTunerStatesUpdate) {
		JsonDecode<NotifyTunerStatesUpdate>(handle, processor);
	}
	else
	if (klass == Klass::kNotifyTunerPretune) {
		JsonDecode<NotifyTunerPretune>(handle, processor);
	}
	else
	if (klass == Klass::kNotifyClientConnectionEvent) {
		JsonDecode<NotifyClientConnectionEvent>(handle, processor);
	}
	else if (klass == Klass::kGetTRMConnectionEvents) {
		JsonDecode<GetTRMConnectionEvents>(handle, processor);
	}
        else if (klass == Klass::kUpdateTunerActivityStatus) {
                JsonDecode<UpdateTunerActivityStatus>(handle, processor);
        }

	else
	{
		SafeAssert(0);
	}

}

TRM_END_NAMESPACE


/** @} */
/** @} */
