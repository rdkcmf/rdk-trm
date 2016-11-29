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
 * @file Header.h
 *
 * @defgroup TRM_MAIN Tuner Reservation Manager (TRM)
 * Tuner Reservation Manager (TRM) coordinates the usage of tuners on the device for the connected clients on Gateway device and
 * it is specific to Gateway devices only.
 * TRM uses messages for communicating between Gateway and Client device to reserve, cancel, status retrieval, etc.
 *
 * @image html trm_main.png
 *
 * - The Gateway box contains multiple tuners (example, Xg1v3 broadcom box having 6 tuner connected to it)
 * - The Gateway box connected with the incoming RF signal and it is also connected to client boxes
 * through the MOCA/Router devices.
 * - TRM application runs on a gateway device and coordinates the usage of tuners on the device for
 * its connecting clients based on the types of the request.
 * - Reserve a tuner for specified usage (Live playback, Record, Hybrid, or EAS), start time, end time and content (locator).
 * - Detect conflicts with existing tuner reservations and resolve.
 *
 * @par How TRM works
 * Client requests a tuner through URL(http, dvr, live), TRM server receives the request and checks for the valid
 * reservation and reserves the tuner so that the client is provided with the service requested.
 * Client also can extend or delete the reservation. It is also possible for a client to request
 * a list of the active reservations.
 *
 * Usually Xg1 boxes have multiple tuners (in Xg1v3 box has 6 tuners (configurable)) & each tuners can have following states:
 * State       | Meaning
 * ------------| -----------
 * Free        | The Tuner is available for allocation.
 * Live        | Currently tuned to a service and in use for viewing the live TV channel.
 * Record      | Currently tuned to a service and recording a program.
 * Hybrid      | Live view of recording in progress.
 * EAS         | Emergency Alert Service.
 *
 * @par TRM Consumer
 * This diagram depicts the different element interact with TRM.
 * @image html trm_consumer.png
 * - The Guide server represent the primary user interface for end user.
 * - The user agent and the browser agents represents the local application acts as a client to the guide server.
 * - In case of html 5 application, the Browser/Agent would be web browser.
 * - The Recorder is an RDK component, it maintain a list of Active, Completed and Schedule of recording.
 * - The Scheduler is responsible for communicating with TRM for scheduling the recording (start, stop, pause).
 *
 * @par TRM Communication and WebSockets
 * TRM uses WebSockets to communicate with other components. This diagram represent the running state of
 * the Tuner Reservation Manager(TRM)
 * @image html trm_websocket1.png
 * The above diagram depicts that how IP client device sends the request to gateway device for TRM service.
 * - TRM Server is a process, which starts and listen on a perticular port.
 * - TRM Server receives the Tuner reservation request from client device through websocket.
 *
 * @par TRM Sequence Diagram
 * @image html trm_sequence.png
 */

/**
* @defgroup trm
* @{
* @defgroup common
* @{
**/


#ifndef TRM_TRANSPORT_HEADER_H_
#define TRM_TRANSPORT_HEADER_H_

#include <stdint.h>
#include <vector>

#include "TRM.h"
#include "Klass.h"
#include "Messages.h"

TRM_BEGIN_NAMESPACE

class Header
{
public:
	static const size_t kHeaderLength = 16;

	Header(void) {}
	Header(MessageType type, int connectionId, size_t payloadLength);
	void serialize(std::vector<uint8_t> &out) const;
	void deserialize(const std::vector<uint8_t> &in);
	void setPayloadLength(size_t payloadLength);
	size_t getPayloadLength(void) const;
	void setClientId(uint32_t connectionId);
	uint32_t  getClientId(void) const;

	void print(void) const {
		printf("payloadLength =0x%0X\r\n", payloadLength);
	}
private:
	static uint32_t messageIndex;
	/* Header Fields:
	 * |'T'R'M'S|'t'y'p'e|-'i'd'-|'p'l'e'n|
	 */
	static const char 	kProtocol[4];
	MessageType 		type;
	uint32_t 			clientId;
	uint32_t 			payloadLength;

public:
};

TRM_END_NAMESPACE
#endif


/** @} */
/** @} */
