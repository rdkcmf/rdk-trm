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

#ifndef _HELPER_H_
#define _HELPER_H_

#include <stdint.h>
#include <errno.h>
#include <stdlib.h>
#include <map>
#include <string>
#include <string.h>
#include <iostream>
#include <list>
#include <string>

class TRMMgrHelperImpl
{

	TRMMgrHelperImpl();
   ~TRMMgrHelperImpl();

public:
	static void init();
    static TRMMgrHelperImpl &getInstance();
    bool getVersion(uint32_t *length,char *responseMsg);
    bool getTunerReservationsList(uint32_t *length,char *responseMsg);
    bool getConnectedDeviceIdList(uint32_t *length,char *responseMsg);
    bool getTRMConnectionEventList(uint32_t *length,char *responseMsg);
    bool getNumofTRMErrors(uint32_t *errCount);

private:
	static TRMMgrHelperImpl* TRMMgrHelperInstance;
    static bool inited;
};

#endif

