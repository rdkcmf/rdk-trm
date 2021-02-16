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

#ifndef TRMMGR_H_
#define TRMMGR_H_


#include <stdint.h>
#include <errno.h>
#include <stdlib.h>
#include <string>


using namespace std;


#define IARM_BUS_TRMMGR_NAME                        "TRMMgr"  /*!< TRM  manager IARM bus name */

/*
 * Declare RPC API names and their arguments
 */
#define IARM_BUS_TRMMGR_API_GetTRMDiagInfo               "GetTRMDiagInfo" /*!< Get TRM Message */


#define MAX_PAYLOAD_LEN         4096 // need to re-visit the size . TBD


typedef enum {
    TRMMgr_ERR_NONE = 0,                   /**< No Error       */
    TRMMgr_ERR_FAILED,                     /**< API failure    */
    TRMMgr_ERR_INVALID_PARAM,              /**< Invalid input parameter.          */
    TRMMgr_ERR_OPERATION_NOT_SUPPORTED,    /**< Operation not supported.          */
    TRMMgr_ERR_UNKNOWN                    /**< Unknown error.                    */
}TRMMgr_Error_t;


typedef enum _TRMMessageType_t {
    TRMMgr_MSG_TYPE_GET_TRM_VERSION = 0,
    TRMMgr_MSG_TYPE_GET_NUM_IN_BAND_TUNERS,
    TRMMgr_MSG_TYPE_GET_CONN_DEVICE_IDS,
    TRMMgr_MSG_TYPE_GET_TUNER_RESERVATION,
    TRMMgr_MSG_TYPE_GET_CONNECTION_ERRORS,
    TRMMgr_MSG_TYPE_GET_TUNER_CONFLICTS,
    TRMMgr_MSG_TYPE_GET_NUM_TRM_ERRORS,
    TRMMgr_MSG_TYPE_MAX,
}TRMMessageType_t;


typedef struct _TRMDiagInfo_t {
    TRMMgr_Error_t  retCode;            // return code of the API.
    TRMMessageType_t msgType;           // Requested Message Type.
    uint8_t numOfTuner;                    // Num of supported Tuners.
    uint8_t numOfTRMError;                 // Count of TRM Errors
    uint32_t bufLen;                      // Length of the data buffer;
    char   buf[MAX_PAYLOAD_LEN];        // Buffer containing the TRM Message.
}TRMDiagInfo_t;


#endif

