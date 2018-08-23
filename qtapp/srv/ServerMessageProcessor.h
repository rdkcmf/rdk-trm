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


#ifndef TRM_SERVER_MESSAGE_PROCESSOR_H_
#define TRM_SERVER_MESSAGE_PROCESSOR_H_

#include <limits>

///#include <QObject>
#include <map>

#include "trm/TRM.h"
#include "trm/TunerReservation.h"
#include "trm/MessageProcessor.h"
#include "trm/AsyncNotification.h"

#include "Tuner.h"
#include "Executors.h"
#include "Connection.h"

TRM_BEGIN_NAMESPACE

class ServerMessageProcessor : public MessageProcessor
{
public:

	ServerMessageProcessor(const Connection *connection = 0, int clientId = 0) :
		connection(connection), clientId(clientId) {};

	template<class MsgT>
	void process(const MsgT &msg) {
		Executor<MsgT> exec(msg, clientId);
		exec();
		{
#if 0
			std::vector<uint8_t> out(Header::kHeaderLength, 0);
			JsonEncoder().encode(exec.getResponse(), out);
			out.push_back('\0');
			std::cout << (const char *)(&out[Header::kHeaderLength]) << "\r\n";
			out.pop_back(); // Some json parser had issue with extra '\0' byte;
			//Now send response bytes out to connection.
			if (connection) {
				std::vector<uint8_t> headerBytes;
				Header header(Response, clientId, out.size() - Header::kHeaderLength);
				header.serialize(headerBytes);
				memcpy(&out[0], &headerBytes[0], Header::kHeaderLength);
				connection->send(out);
			}
#endif
		}
	}

	void operator() (const ReserveTuner &msg);
	void operator() (const ReleaseTunerReservation &msg);
	void operator() (const ValidateTunerReservation &msg);
	void operator() (const CancelRecording &msg);
	void operator() (const CancelRecordingResponse &msg);
	void operator() (const GetAllTunerIds &msg);
	void operator() (const GetAllTunerStates &msg);
	void operator() (const GetAllReservations &msg);
	void operator() (const GetVersion &msg);
	void operator() (const CancelLive &msg);
	void operator() (const CancelLiveResponse &msg);
	void operator() (const UpdateTunerActivityStatus &msg);

private:


private:
	const Connection * connection;
	int clientId;

private:
};

TRM_END_NAMESPACE

#endif


/** @} */
/** @} */
