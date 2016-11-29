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


#include <map>
#include <string>

#include "trm/TRM.h"
#include "trm/ResponseStatus.h"


LOCAL_BEGIN_NAMESPACE
enum {
	Ok,
	GeneralError,
	MalFormedRequest,
	UnRecognizedRequest,

	InvalidToken,
	InvalidState,
	UserCancellation,
	InsufficientResource,
	MAX_ENUM_NUMBER
};
LOCAL_END_NAMESPACE

TRM_BEGIN_NAMESPACE

const Enum<ResponseStatus> ResponseStatus::kOk  				(Ok, 					"Ok");
const Enum<ResponseStatus> ResponseStatus::kGeneralError  		(GeneralError, 			"GeneralError");
const Enum<ResponseStatus> ResponseStatus::kMalFormedRequest  	(MalFormedRequest, 		"MalFormedRequest");
const Enum<ResponseStatus> ResponseStatus::kUnRecognizedRequest	(UnRecognizedRequest, 	"UnRecognizedRequest");

const Enum<ResponseStatus> ResponseStatus::kInvalidToken	    (MAKE_PAIR(InvalidToken));
const Enum<ResponseStatus> ResponseStatus::kInvalidState	    (MAKE_PAIR(InvalidState));
const Enum<ResponseStatus> ResponseStatus::kUserCancellation	(MAKE_PAIR(UserCancellation));
const Enum<ResponseStatus> ResponseStatus::kInsufficientResource(MAKE_PAIR(InsufficientResource));

const std::vector<const Enum<ResponseStatus> * > & ResponseStatus::getEnums(void)
{
	static std::vector<const Enum<ResponseStatus> * >  enums_;
	if (enums_.size() == 0) {
		enums_.push_back(&ResponseStatus::kOk);
		enums_.push_back(&ResponseStatus::kGeneralError);
		enums_.push_back(&ResponseStatus::kMalFormedRequest);
		enums_.push_back(&ResponseStatus::kUnRecognizedRequest);
		enums_.push_back(&ResponseStatus::kInvalidToken);
		enums_.push_back(&ResponseStatus::kInvalidState);
		enums_.push_back(&ResponseStatus::kUserCancellation);
		enums_.push_back(&ResponseStatus::kInsufficientResource);

		Assert(enums_.size() == MAX_ENUM_NUMBER);
	};

	return enums_;
}

ResponseStatus::ResponseStatus(const Enum<ResponseStatus> &statusCode, const std::string &details)
: statusCode(statusCode),  details(details)
{
}

ResponseStatus::ResponseStatus(const char *name, const std::string &details)
: statusCode(Enum<ResponseStatus>::at(name)), details(details)
{
}

ResponseStatus::~ResponseStatus(void)
{
}

const Enum<ResponseStatus> & ResponseStatus::getStatusCode(void) const
{
	return statusCode;
}


bool ResponseStatus::operator == (const ResponseStatus &that) const
{
	return (this->statusCode == that.statusCode);
}

ResponseStatus & ResponseStatus::operator += (const char *message)
{
	details += message;
	return *this;
}
ResponseStatus & ResponseStatus::operator= (const Enum<ResponseStatus> &status)
{
	statusCode = status;
	return *this;
}

void ResponseStatus::print(void) const
{
	statusCode.print();
	std::cout << "[OBJ][" << klassName() << "][details]" << details << std::endl;
}

const std::string & ResponseStatus::getDetails(void) const
{
	return details;
}

std::string & ResponseStatus::getDetails(void)
{
	return details;
}


TRM_END_NAMESPACE


/** @} */
/** @} */
