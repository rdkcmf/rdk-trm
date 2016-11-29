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


#ifndef TRM_RESPONSE_STATUS_H_
#define TRM_RESPONSE_STATUS_H_

#include <stdio.h>
#include <string>

#include "TRM.h"
#include "Enum.h"
#include "Klass.h"

TRM_BEGIN_NAMESPACE


/**
 * @brief This class is responsible for handling response message for tuner reservation.
 * All response messages from TRM will provide information regarding the status of the response.
 * Responses to recognized requests may contain additional information, as described in later sections
 * of this document. Responses to unrecognized requests will contain only this status data, consisting
 * of a status code and message signifying the request was unrecognized.
 * The Tuner response status could be the following types.
 * - @b Ok : Request was successful
 * - @b GeneralError : Request was unsuccessful
 * - @b MalFormedRequest : Unexpected/Invalid request data
 * - @b UnRecognizedRequest : Unrecognized request
 * - @b InsufficientResource : there is no tuner available
 * - @b UserCancellation: Token is released as result of user cancellation.
 * - @b InvalidToken: Token included in the message is invalid.
 * - @b InvalidState: Token is in invalid state. 
 * - @b statusMessage: is a string containing additional information about the status.
 *
 * @ingroup TRM_RESERVATION_CLASSES
 */
class ResponseStatus
{
public:
	static const char *klassName(void) { return Klass::kResponseStatus; }

	typedef  int   EnumType;

	static const Enum<ResponseStatus> kOk;
	static const Enum<ResponseStatus> kGeneralError;
	static const Enum<ResponseStatus> kMalFormedRequest;
	static const Enum<ResponseStatus> kUnRecognizedRequest;
	static const Enum<ResponseStatus> kInvalidToken;
	static const Enum<ResponseStatus> kInvalidState;
	static const Enum<ResponseStatus> kUserCancellation;
	static const Enum<ResponseStatus> kInsufficientResource;

	static const std::vector<const Enum<ResponseStatus> * > & getEnums(void);

	ResponseStatus(const Enum<ResponseStatus> &statusCode, const std::string &details="");
	ResponseStatus(const char *name, const std::string &details="");
	~ResponseStatus(void);

	const Enum<ResponseStatus> & getStatusCode(void) const;

	bool operator==(const ResponseStatus &that) const;
	ResponseStatus & operator += (const char *message);
	ResponseStatus & operator = (const Enum<ResponseStatus> &status);

	const std::string & getDetails(void) const;
	std::string &getDetails(void);
	void print(void) const;

private:
	Enum<ResponseStatus> statusCode;
	std::string details;
};


TRM_END_NAMESPACE

#endif


/** @} */
/** @} */
