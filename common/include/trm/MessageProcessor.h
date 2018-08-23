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


#ifndef TRM_MESSAGE_PROCESSOR_H_
#define TRM_MESSAGE_PROCESSOR_H_

#include <iostream>

#include "TRM.h"
#include "Messages.h"

TRM_BEGIN_NAMESPACE

class MessageProcessor
{
public:
	MessageProcessor(void) {}

	virtual void operator() (const ReserveTuner &msg)  					 {process(msg);}
	virtual void operator() (const ReserveTunerResponse &msg) 			 {process(msg);}
	virtual void operator() (const ReleaseTunerReservation &msg) 		 {process(msg);}
	virtual void operator() (const ReleaseTunerReservationResponse &msg) {process(msg);}
	virtual void operator() (const ValidateTunerReservation &msg) 		 {process(msg);}
	virtual void operator() (const ValidateTunerReservationResponse &msg){process(msg);}
	virtual void operator() (const CancelRecording &msg) 				 {process(msg);}
	virtual void operator() (const CancelRecordingResponse &msg) 		 {process(msg);}
	virtual void operator() (const CancelLive &msg) 				     {process(msg);}
	virtual void operator() (const CancelLiveResponse &msg) 		 	 {process(msg);}
	virtual void operator() (const GetAllTunerIds &msg) 				 {process(msg);}
	virtual void operator() (const GetAllTunerIdsResponse &msg) 		 {process(msg);}
	virtual void operator() (const GetAllTunerStates &msg) 				 {process(msg);}
	virtual void operator() (const GetAllTunerStatesResponse &msg) 		 {process(msg);}
	virtual void operator() (const GetAllReservations &msg) 			 {process(msg);}
	virtual void operator() (const GetAllReservationsResponse &msg) 	 {process(msg);}
	virtual void operator() (const GetAllConnectedDeviceIdsResponse &msg){process(msg);}
	virtual void operator() (const GetVersion &msg) 	                 {process(msg);}
	virtual void operator() (const GetVersionResponse &msg) 	         {process(msg);}
	virtual void operator() (const NotifyTunerReservationUpdate &msg) 	 {process(msg);}
	virtual void operator() (const NotifyTunerReservationRelease &msg) 	 {process(msg);}
	virtual void operator() (const NotifyTunerReservationConflicts &msg) {process(msg);}
	virtual void operator() (const NotifyTunerStatesUpdate &msg) 	     {process(msg);}
	virtual void operator() (const NotifyTunerPretune &msg) 	         {process(msg);}
	virtual void operator() (const NotifyClientConnectionEvent &msg) 	 {process(msg);}
	virtual void operator() (const GetTRMConnectionEvents &msg) 	 	 {process(msg);}
        virtual void operator() (const UpdateTunerActivityStatus &msg)              {process(msg);}


	virtual ~MessageProcessor(void) {}

private:

	template<class MsgT>
	void process(const MsgT &msg) {
		/* Default Implementation */
		msg.print();
	}

};

TRM_END_NAMESPACE
#endif


/** @} */
/** @} */
