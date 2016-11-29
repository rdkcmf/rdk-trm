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


#include "trm/TunerState.h"
#include "trm/Activity.h"

LOCAL_BEGIN_NAMESPACE
enum {
		Free,
		Live,
		Record,
		Hybrid,
		EAS,

		MAX_ENUM_NUMBER
};

LOCAL_END_NAMESPACE

TRM_BEGIN_NAMESPACE

const Enum<TunerState> TunerState::kFree  (MAKE_PAIR(Free));
const Enum<TunerState> TunerState::kLive  (MAKE_PAIR(Live));
const Enum<TunerState> TunerState::kRecord(MAKE_PAIR(Record));
const Enum<TunerState> TunerState::kHybrid(MAKE_PAIR(Hybrid));
const Enum<TunerState> TunerState::kEAS   (MAKE_PAIR(EAS));

const std::vector<const Enum<TunerState> * > & TunerState::getEnums(void)
{
	static std::vector<const Enum<TunerState> * >  enums_;
	if (enums_.size() == 0) {
		enums_.push_back(&TunerState::kFree);
		enums_.push_back(&TunerState::kLive);
		enums_.push_back(&TunerState::kRecord);
		enums_.push_back(&TunerState::kHybrid);
		enums_.push_back(&TunerState::kEAS);

		Assert(enums_.size() == MAX_ENUM_NUMBER);

	};

	return enums_;
}

TunerState::TunerState(const Enum<TunerState> &state)
: state(state)
{
}

TunerState::TunerState(const char *name)
: state((Enum<TunerState>::at(name)))
{
}

TunerState::~TunerState(void)
{
}

const Enum<TunerState> & TunerState::getState(void) const
{
	return state;
}

TunerState TunerState::operator +  (const Enum<Activity> & activity)
{
	const Enum<TunerState>  origState = state;

	if (state ==  TunerState::kFree) {
		if (activity == Activity::kLive) {
			state = TunerState::kLive;
		}
		else if(activity == Activity::kRecord){
			state = TunerState::kRecord;
		}
		else if(activity == Activity::kEAS){
			state = TunerState::kEAS;
		}
	}
	else
	if (state ==  TunerState::kLive) {
		if(activity == Activity::kRecord){
			state = TunerState::kHybrid;
		}
	}
	else
	if (state ==  TunerState::kRecord) {
		if(activity == Activity::kLive){
			state = TunerState::kHybrid;
		}
	}

	if (state == origState) {
	}

	return *this;
}

#if 0
TunerState TunerState::operator -  (const Enum<Activity> &activity)
{
	const Enum<TunerState>  origState = state;
	std::cout << "State(" << (const char *) state << ") - Activity ("
			  << (const char *)activity << ")" << std::endl;

	if (state ==  TunerState::kHybrid) {
		if (activity == Activity::kLive) {
			state = TunerState::kRecord;
		}
		else if(activity == Activity::kRecord){
			state = TunerState::kLive;
		}
	}
	else
	if (state ==  TunerState::kLive) {
		if(activity == Activity::kLive){
			state = TunerState::kFree;
		}
	}
	else
	if (state ==  TunerState::kRecord) {
		if(activity == Activity::kRecord){
			state = TunerState::kFree;
		}
	}

	if (state == origState) {
		SafeAssert(0);
	}

	return *this;
}
#endif

bool TunerState::operator == (const TunerState &that) const
{
	return ((this->state)) == ((that.state));
}

bool TunerState::operator != (const TunerState &that) const
{
	return (!(*this == that));
}

void TunerState::print(void) const
{
	(state).print();
}

TRM_END_NAMESPACE


/** @} */
/** @} */
