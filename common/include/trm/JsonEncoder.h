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


#ifndef TRM_JSON_ENCODER_H_
#define TRM_JSON_ENCODER_H_

#include <stdint.h>
#include <string.h>

#include <vector>

#include "TRM.h"
#include "Messages.h"
#include "Header.h"

TRM_BEGIN_NAMESPACE
void JsonEncode(const ReserveTuner &r, std::vector<uint8_t> &out);
void JsonEncode(const ReserveTunerResponse &r, std::vector<uint8_t> &out);
void JsonEncode(const ReleaseTunerReservation & r, std::vector<uint8_t> &out);
void JsonEncode(const ValidateTunerReservation & r, std::vector<uint8_t> &out);
void JsonEncode(const CancelRecording & r, std::vector<uint8_t> &out);
void JsonEncode(const CancelLive & r, std::vector<uint8_t> &out);
void JsonEncode(const ReleaseTunerReservationResponse &r, std::vector<uint8_t> &out);
void JsonEncode(const ValidateTunerReservationResponse &r, std::vector<uint8_t> &out);
void JsonEncode(const CancelRecordingResponse &r, std::vector<uint8_t> &out);
void JsonEncode(const CancelLiveResponse &r, std::vector<uint8_t> &out);
void JsonEncode(const GetAllTunerIds &r, std::vector<uint8_t> &out);
void JsonEncode(const GetAllTunerIdsResponse &r, std::vector<uint8_t> &out);
void JsonEncode(const GetAllTunerStates &r, std::vector<uint8_t> &out);
void JsonEncode(const GetAllTunerStatesResponse &r, std::vector<uint8_t> &out);
void JsonEncode(const GetAllReservations &r, std::vector<uint8_t> &out);
void JsonEncode(const GetAllReservationsResponse &r, std::vector<uint8_t> &out);
void JsonEncode(const GetAllConnectedDeviceIdsResponse &r, std::vector<uint8_t> &out);
void JsonEncode(const GetVersion &r, std::vector<uint8_t> &out);
void JsonEncode(const GetVersionResponse &r, std::vector<uint8_t> &out);
void JsonEncode(const NotifyTunerReservationUpdate &r, std::vector<uint8_t> &out);
void JsonEncode(const NotifyTunerReservationRelease &r, std::vector<uint8_t> &out);
void JsonEncode(const NotifyTunerReservationConflicts &r, std::vector<uint8_t> &out);
void JsonEncode(const NotifyTunerStatesUpdate &r, std::vector<uint8_t> &out);
void JsonEncode(const NotifyTunerPretune &r, std::vector<uint8_t> &out);
void JsonEncode(const NotifyClientConnectionEvent &r, std::vector<uint8_t> &out);
void JsonEncode(const GetTRMConnectionEvents &r, std::vector<uint8_t> &out);
void JsonEncode(const UpdateTunerActivityStatus &r, std::vector<uint8_t> &out);
void JsonEncode(const UpdateTunerActivityStatusResponse &r, std::vector<uint8_t> &out);


class JsonEncoder {

public:
	JsonEncoder(void) {};
	template<class MsgT>
	void encode(const MsgT &r, std::vector<uint8_t> &out)
	{
		JsonEncode(r, out);
	}
};

template<class MsgT>
void SerializeMessage(const MsgT&msg, uint32_t clientId, std::vector<uint8_t> &out)
{
	out.clear();
	out.resize(Header::kHeaderLength, 0);
	JsonEncoder().encode(msg, out);
	out.push_back('\0');
	std::cout << (const char *)(&out[Header::kHeaderLength]) << "\r\n";
	out.pop_back(); // Some json parser had issue with extra '\0' byte;
	//Now send response bytes out to connection.
	if (1) {
		std::vector<uint8_t> headerBytes;
		Header header(msg.getType(), clientId, out.size() - Header::kHeaderLength);
		header.serialize(headerBytes);
		::memcpy(&out[0], &headerBytes[0], Header::kHeaderLength);
	}
}


TRM_END_NAMESPACE
#endif


/** @} */
/** @} */
