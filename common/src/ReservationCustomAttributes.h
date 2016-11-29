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


#ifndef TRM_RESERVATION_CUSTOM_ATTRIBUTES_H_
#define TRM_RESERVATION_CUSTOM_ATTRIBUTES_H_

#include "jansson.h"

#include "trm/TRM.h"

TRM_BEGIN_NAMESPACE

class ReservationCustomAttributes
{
public:
	ReservationCustomAttributes(json_t *jsonObject) : jsonObject(jsonObject)
	{
		json_incref(jsonObject);
	}

	~ReservationCustomAttributes(void)
	{
		json_decref(jsonObject);
	}

	json_t *getObject(void) const {
		return jsonObject;
	}

private :
	json_t *jsonObject;
};
TRM_END_NAMESPACE

#endif


/** @} */
/** @} */
