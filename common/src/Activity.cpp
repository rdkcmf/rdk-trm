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
#include <iostream>
#include <vector>
#include "trm/Activity.h"

LOCAL_BEGIN_NAMESPACE
enum  {
		None,
		Live,
		Record,
		EAS,

		MAX_ENUM_NUMBER,
	};

LOCAL_END_NAMESPACE

TRM_BEGIN_NAMESPACE

const Enum<Activity> Activity::kNone   (MAKE_PAIR(None));
const Enum<Activity> Activity::kLive   (MAKE_PAIR(Live));
const Enum<Activity> Activity::kRecord (MAKE_PAIR(Record));
const Enum<Activity> Activity::kEAS    (MAKE_PAIR(EAS));

const std::vector<const Enum<Activity> * > & Activity::getEnums(void)
{
	static std::vector<const Enum<Activity> * >  enums_;
	if (enums_.size() == 0) {
		enums_.push_back(&Activity::kNone);
		enums_.push_back(&Activity::kLive);
		enums_.push_back(&Activity::kRecord);
		enums_.push_back(&Activity::kEAS);

		Assert(enums_.size() == MAX_ENUM_NUMBER);

	};

	return enums_;
}

Activity::Activity(const Enum<Activity> &activity)
: activity(activity)
{
}

Activity::Activity(const char *name)
: activity(Enum<Activity>::at(name))
{
}

Activity::~Activity(void)
{
}

/**
 * @brief This function is used to return the request or granted usage of tuner. The Activity
 * field represents the intended use of the tuner.
 *
 * @return Supported tuner activity name are Live, Recording, or EAS.
 */
const Enum<Activity> & Activity::getActivity(void) const
{
	return activity;
}


/**
 * @brief This function is used to return true if the activity has the detailed field describing the
 * activity, otherwise the function will return false.
 *
 * @return Returns true if the tuner activity has a detailed field else will return false.
 */
bool Activity::hasDetails(void) const
{
	return details.size() != 0;
}


/**
 * @brief This function will get the details of the recording.
 * Each activity may be associated with a set of details describing the tuner activity.
 *
 * @param[in] key recording Id for which details to be extracted from the list.
 * @return
 */
const std::string & Activity::getDetail(const std::string &key) const
{
	static const std::string emptyDetail = "";
	std::map<KeyT,ValT>::const_iterator it = details.find(key);
	if (it != details.end()) {
		return it->second;
	}
	else {
		return emptyDetail;
	}
}

const std::map<Activity::KeyT,Activity::ValT> & Activity::getDetails(void) const
{
	return details;
}


/**
 * @brief This API is used to add the details describing the activity in to a list.
 *
 * The field specified here are required for the associated activity. The requestor is
 * allowed to insert unspecified fields in the details. These unspecified fields are ignored by TRM,
 * and echoed back in response message that have the activity field.
 * @n The defined fields are:
 * - recordingId : required when requesting Record activity for a tuner.
 * - hot : flag (true or false) indicating of the recording is scheduled or hot.
 *
 * @param[in] key it could be "recordingId" or "hot" when requesting Record activity for a tuner.
 * @param[in] value value associated for recording.
 * @return None.
 * @ingroup TRM_API
 */
void Activity::addDetail(const std::string &key, const std::string &value)
{
	details.insert(std::pair<KeyT,ValT>(key,value));
}

bool Activity::operator == (const Activity &that) const
{
	return (this->activity == that.activity);
			//&& (this->details == that.details);
}


/**
 * @brief Print the list of recording(s) details which are being scheduled. The defined fields are,
 * - recordingId : required when requesting Record activity for a tuner.
 * - hot : flag (true or false) indicating of the recording is scheduled or hot.
 * This function is used for debugging.
 * @return None.
 */
void Activity::print(void) const
{
	activity.print();
	  for (std::map<KeyT,ValT>::const_iterator it=details.begin(); it!=details.end(); ++it) {
		std::cout << "[OBJ][" << klassName() << "][Details] " << it->first << " => " << it->second << '\n';
	  }
}

TRM_END_NAMESPACE


/** @} */
/** @} */
