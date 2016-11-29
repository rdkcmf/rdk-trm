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
 * @defgroup TRM_ACTIVITY Tuner Activity
 * @ingroup TRM_MAIN
 * This represents the request or granted usage of a tuner.
 * @n @b Activity object represents the request or granted usage of a tuner.
 * @n
 * @code
 * Activity := {
 *     "name" : [String] name,
 *     "details"(optional): <Details>,
 * }
 * @endcode
 * @n
 * The activity field represents the intended use of the tuner. Supported tuner activity names are:
 * - @b Live: the tuner is used for Live (Live Streaming or Local Live).
 * - @b Record: the tuner is used for Recording.
 * - @b Hybrid: the tuner is reserved for Live and Record activity.
 * - @b EAS: the tuner is used for EAS.
 * @n
 * When tuner sharing is allowed among these activities, a tuner may be reserved for multiple activities at a time.
 * This is indicated by the state of the tuner. However, a single Tuner Reservation Request message can contain at most one activity.
 * The details of activity is optional.
 *
 * @par Tuner Details
 * Each activity may be associated with a set of details describing the activity.
 * @n
 * @code
 * Details :=
 * {
 *     "recordingId" : [String]
 *     "hot" : [String]
 * }
 * @endcode
 *
 * @defgroup TRM_ACTIVITY_CLASSES Tuner Activity Interface Classes
 * @ingroup TRM_ACTIVITY
 */

/**
* @defgroup trm
* @{
* @defgroup common
* @{
**/


#ifndef TRM_ACTIVITY_H_
#define TRM_ACTIVITY_H_

#include <string>
#include <iostream>
#include <map>

#include "TRM.h"
#include "Enum.h"

TRM_BEGIN_NAMESPACE

/**
 * @brief The Activity class represents the request or granted usage of a tuner.
 * The activity field in the class represents the intended use of the tuner.
 * Supported tuner activity names are:
 * - @b Live: the tuner is used for Live (Live Streaming or Local Live)
 * - @b Record: The tuner is used for Recording.
 * - @b Hybrid: the tuner is reserved for Live and Record activity.
 * - @b EAS: The tuner is used for EAS
 * When tuner sharing is allowed among these activities, a tuner may be reserved for multiple
 * activities at a time. This is indicated by the state of the tuner. However, a single Tuner
 * Reservation message can contain at most one activity. The details filed which contains in the
 * class is optional.
 * @ingroup TRM_ACTIVITY_CLASSES
 */
class Activity
{
public:
	static const char *klassName(void) { return "Activity"; }

	typedef  const std::string KeyT;
	typedef  const std::string ValT;
	typedef  int   EnumType;

	static const Enum<Activity> kNone;
	static const Enum<Activity> kLive;
	static const Enum<Activity> kRecord;
	static const Enum<Activity> kEAS;

	static const std::vector<const Enum<Activity> * > & getEnums(void);

	Activity(const Enum<Activity> &activity = kNone);
	Activity(const char *);
	~Activity(void);

	const Enum<Activity> & getActivity(void) const;
	bool hasDetails(void) const;
	const std::string & getDetail(const std::string &key) const;
	const std::map<KeyT,ValT> & getDetails(void) const;
	void  addDetail(const std::string &key, const std::string &value);
	bool  operator == (const Activity &that) const ;
	void  print(void) const;

private:
	Enum<Activity> activity;
	std::map<KeyT,ValT> details;
};

TRM_END_NAMESPACE

#endif


/** @} */
/** @} */
