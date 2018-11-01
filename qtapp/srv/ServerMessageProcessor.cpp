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
#include "trm/Header.h"

#include "ServerMessageProcessor.h"
#include "Executors.h"

TRM_BEGIN_NAMESPACE

//@TODO: Templatize

void ServerMessageProcessor::operator() (const ReserveTuner &msg)
{
	process(msg);
}

void ServerMessageProcessor::operator() (const ReleaseTunerReservation &msg)
{
	process(msg);
}

void ServerMessageProcessor::operator() (const ValidateTunerReservation &msg)
{
	process(msg);

}

void ServerMessageProcessor::operator() (const CancelRecordingResponse &msg)
{
	process(msg);
}

void ServerMessageProcessor::operator() (const CancelRecording &msg)
{
	process(msg);
}

void ServerMessageProcessor::operator() (const GetAllTunerIds &msg)
{
	process(msg);
}

void ServerMessageProcessor::operator() (const GetAllTunerStates &msg)
{
	process(msg);
}

void ServerMessageProcessor::operator() (const GetAllReservations &msg)
{
	process(msg);
}

void ServerMessageProcessor::operator() (const GetVersion &msg)
{
	process(msg);
}

void ServerMessageProcessor::operator() (const CancelLive &msg)
{
	process(msg);
}

void ServerMessageProcessor::operator() (const CancelLiveResponse &msg)
{
	process(msg);
}

TRM_END_NAMESPACE


/** @} */
/** @} */
