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


#ifndef TRM_UTIL_H_
#define TRM_UTIL_H_

#include <iostream>
#include <map>
#include "trm/TRM.h"

TRM_BEGIN_NAMESPACE

const std::string IntToString(int i);
const std::string IntToString(int i, int j);

class SpecVersion {

public:
	SpecVersion(int major_, int minor_) : major_(major_), minor_(minor_) {
		Assert(major_ < 1000);
		Assert(minor_ < 1000);
	}
	const std::string toString() const {
		return IntToString(major_, minor_);
	}

	bool operator > (const SpecVersion &that) const {
		return this->toInt() > that.toInt();
	}

private:
	int toInt(void) const {
		return ((major_ << 16) | minor_);
	}
	int major_;
	int minor_;
};

const std::string GenerateUUID(void);
void HexDump(const std::vector<uint8_t> &in);
uint64_t GetCurrentEpoch(void);
std::ostream & Timestamp (std::ostream &os);
std::ostream & Log (void);
std::string GetAuthToken(const char *generateTokenRequest);
const SpecVersion &GetSpecVersion(void);
TRM_END_NAMESPACE

#endif


/** @} */
/** @} */
