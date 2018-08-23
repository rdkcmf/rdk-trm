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


#include <vector>
#include <iostream>

#include "trm/TRM.h"
#include "trm/Messages.h"
#include "trm/JsonEncoder.h"
#include "trm/Header.h"

#include "Connection.h"

#ifndef TRM_EXECUTORS_H_
#define TRM_EXECUTORS_H_

TRM_BEGIN_NAMESPACE
template<class InT>
class Executor
{
	friend void Execute(Executor<InT> &);
	friend void Execute(Executor<ReserveTuner> &, const std::string &);

public:
	typedef typename InT::ResponseType OutT;
	Executor(const InT & messageIn, uint32_t clientId) : messageIn(messageIn), messageOut(messageIn.getUUID()), clientId(clientId) {}
	const InT &  getRequest  (void) const {return messageIn;}
	const OutT & getResponse(void) const  {
		if (messageOut.getClassName().compare(NoResponse::klassName())) {
			throw InvalidOperationException();
		}
		return messageOut;
	}
	void operator() (void) {
		std::cout << "[EXEC]-" << clientId << "|" << messageIn.getClassName() << std::endl;
		Execute(*this);
	}

	uint32_t getClientId(void) const {return clientId;}
private:
	const InT &messageIn;
	OutT messageOut;
	uint32_t clientId;
};

void Execute(Executor<ReserveTuner> &);
void Execute(Executor<ReleaseTunerReservation> &);
void Execute(Executor<ValidateTunerReservation> &);
void Execute(Executor<CancelRecording> &);
void Execute(Executor<CancelRecordingResponse> &);
void Execute(Executor<GetAllTunerIds> &);
void Execute(Executor<GetAllTunerStates> &);
void Execute(Executor<GetAllReservations> &);
void Execute(Executor<ReserveTuner> &, const std::string &);
void Execute(Executor<GetVersion> &exec);
void Execute(Executor<CancelLive> &exec);
void Execute(Executor<CancelLiveResponse> &exec);
void Execute(Executor<UpdateTunerActivityStatus> &exec);

TRM_END_NAMESPACE

#endif


/** @} */
/** @} */
