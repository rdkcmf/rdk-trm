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


#include "trm/TRM.h"
#include "trm/Header.h"
#include "Util.h"

TRM_BEGIN_NAMESPACE
const char Header::kProtocol[4] = {'T','R', 'M', 'S'};

Header::Header(MessageType type, int clientId, size_t payloadLength)
: type(type), clientId(clientId), payloadLength(payloadLength)
{
}

void Header::serialize(std::vector<uint8_t> &out) const
{
	out.resize(Header::kHeaderLength, 0);

	/* First 4 bytes are 'T'R'M'S */
	out[0]  = kProtocol[0];
	out[1]  = kProtocol[1];
	out[2]  = kProtocol[2];
	out[3]  = kProtocol[3];
	/* Next 4 bytes are Message Type */
	out[4]  = (type & 0xFF000000) >> 24;
	out[5]  = (type & 0x00FF0000) >> 16;
	out[6]  = (type & 0x0000FF00) >>  8;
	out[7]  = (type & 0x000000FF) >>  0;
	/* Next 4 bytes are transportId () */
	out[8]  = (clientId & 0xFF000000)  >> 24;
	out[9]  = (clientId & 0x00FF0000)  >> 16;
	out[10] = (clientId & 0x0000FF00) >>  8;
	out[11] = (clientId & 0x000000FF) >>  0;
	/* Last 4 bytes are Payload Length (Unused) */
	out[12]  = (payloadLength & 0xFF000000) >> 24;
	out[13]  = (payloadLength & 0x00FF0000) >> 16;
	out[14]  = (payloadLength & 0x0000FF00) >>  8;
	out[15]  = (payloadLength & 0x000000FF) >>  0;
}

void Header::deserialize(const std::vector<uint8_t> &in)
{
	HexDump(in);
	if (in.size() >= 16) {
		uint8_t *ptr = (uint8_t *)&in[0];
		/* First 4 bytes are 'T'R'M'S */
		if (ptr[0]  == kProtocol[0] &&
			ptr[1]  == kProtocol[1] &&
			ptr[2]  == kProtocol[2] &&
			ptr[3]  == kProtocol[3]) {
			/* Next 4 bytes are Message Type */
			type    = (MessageType)
					  (((ptr[4]) << 24) |
			     	   ((ptr[5]) << 16) |
			     	   ((ptr[6]) <<  8) |
			     	   ((ptr[7]) <<  0));
			/* Next 4 bytes are Message Id (Unused) */
			clientId =
					  (((ptr[8]) << 24) |
			     	   ((ptr[9]) << 16) |
			     	   ((ptr[10]) <<  8) |
			     	   ((ptr[11]) <<  0));

			/* Last 4 bytes are Payload Length (Unused) */
		payloadLength=(((ptr[12]) << 24) |
			     	   ((ptr[13]) << 16) |
			     	   ((ptr[14]) <<  8) |
			     	   ((ptr[15]) <<  0));
		}
		else {
			//@TODO: discard invalid message.
			Assert(0);
		}
	}
	else {
		Assert(0);
	}
}

void Header::setPayloadLength(size_t payloadLength)
{
	this->payloadLength = payloadLength;
}

size_t Header::getPayloadLength(void) const
{
	return payloadLength;
}

void Header::setClientId(uint32_t clientId)
{
	this->clientId = clientId;
}

uint32_t Header::getClientId(void) const
{
	return clientId;
}

TRM_END_NAMESPACE




/** @} */
/** @} */
