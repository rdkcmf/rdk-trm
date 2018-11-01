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

using namespace std;


#include "TRMMgr.h"

/*Helper header*/
#include "Helper.h"
#include "Debug.h"

/*C++ include */
#include <stdint.h>
#include <errno.h>
#include <stdlib.h>
#include <map>
#include <string>
#include <string.h>
#include <uuid/uuid.h>
#include <list>
#include <vector>
#include <time.h>
#include <sys/time.h>
#include <stddef.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <iostream>
#include <cstdio>


/* IARM include */
#include "libIBus.h"
static IARM_Result_t _GetTRMDiagInfo(void *arg);

/* TRM include */
#include "trm/Messages.h"
#include "trm/MessageProcessor.h"
#include "trm/Activity.h"
#include "trm/JsonEncoder.h"
#include "trm/JsonDecoder.h"

/* For connection sync */
static char responseMsg[MAX_PAYLOAD_LEN];
static pthread_mutex_t trmMgr_mutex;

class TRMMgrLock_
{
public:
    TRMMgrLock_(void) {
        pthread_mutex_lock(&trmMgr_mutex);
    }
    ~TRMMgrLock_(void) {
        pthread_mutex_unlock(&trmMgr_mutex);
    }
};

#define TRMMgrLock() TRMMgrLock_ t()



/**************************************************************************
Function name : Get TRMMessage 

Arguments     : TRMMessageData_t 

Description   : Frame the Requested TRM Diag Message and send it to Client.
**************************************************************************/
static IARM_Result_t _GetTRMDiagInfo(void *arg)
{
    TRMDiagInfo_t *infoParam = (TRMDiagInfo_t *)arg;
    

    DIAG_TRACE(("Enter %s():%d \r\n" , __FUNCTION__, __LINE__));
    
    TRMMgrLock();

    uint32_t length = 0;
    uint32_t errCount = 0;
    bool ret= false;
   
    memset(responseMsg,0,sizeof(responseMsg));
    
    if (NULL == infoParam)
    {
        DIAG_WARN(("Error in  %s : NULL Arguments Passed \r\n",__FUNCTION__));
        return IARM_RESULT_INVALID_PARAM;
    }
    
    /* Initialize the parameters */
    infoParam->numOfTuner = 0;
    infoParam->numOfTRMError = 0;
    infoParam->bufLen  = 0;
    infoParam->retCode = TRMMgr_ERR_NONE;
    

    switch (infoParam->msgType)
    {
        case TRMMgr_MSG_TYPE_GET_NUM_IN_BAND_TUNERS:
        {
            DIAG_WARN(("%s : GET_NUM_IN_BAND_TUNERS  \r\n",__FUNCTION__));
            #ifdef NUM_OF_TUNERS
                    infoParam->numOfTuner = NUM_OF_TUNERS;
            #else
                     infoParam->numOfTRMError = 6;
                     infoParam->retCode = TRMMgr_ERR_FAILED;
            #endif
            DIAG_TRACE(("%s : Num of Tuner Supported = %d  \r\n",__FUNCTION__,infoParam->numOfTuner));     
        }
        break;

        case TRMMgr_MSG_TYPE_GET_CONN_DEVICE_IDS:
        {
            DIAG_WARN(("%s : GET_CONN_DEVICE_IDS  \r\n",__FUNCTION__));
            
            ret = TRMMgrHelperImpl::getInstance().getConnectedDeviceIdList(&length,&responseMsg[0]);
            if(ret)
            {
                infoParam->bufLen  = length;
                memcpy(infoParam->buf,responseMsg,length);
                DIAG_TRACE(("%s() Length of TRM Message is %d \r\n",__FUNCTION__,length));
            }
            else
            {
                infoParam->retCode = TRMMgr_ERR_FAILED;
                DIAG_WARN(("Failed to Get Diag Info  for GET_CONN_DEVICE_IDS \r\n"));
            }
        }                             
        break;
   
        case TRMMgr_MSG_TYPE_GET_TUNER_RESERVATION:
        {
            DIAG_WARN(("%s : GET_TUNER_RESERVATION  \r\n",__FUNCTION__));

            ret = TRMMgrHelperImpl::getInstance().getTunerReservationsList(&length,&responseMsg[0]);
            if(ret)
            {
                infoParam->bufLen  = length;
                memcpy(infoParam->buf,responseMsg,length);
                DIAG_TRACE(("%s() Length of TRM Message is %d \r\n",__FUNCTION__,length));
            }
            else
            {
                infoParam->retCode = TRMMgr_ERR_FAILED;
                DIAG_WARN(("Failed to Get Diag Info  for GET_TUNER_RESERVATION \r\n"));
            }  
        }                             
        break;
        
        case TRMMgr_MSG_TYPE_GET_NUM_TRM_ERRORS:
        {
            DIAG_WARN(("%s : GET_NUM_TRM_ERRORS  \r\n",__FUNCTION__));
            ret = TRMMgrHelperImpl::getInstance().getNumofTRMErrors(&errCount);
            if(ret)
            {
               infoParam->numOfTRMError = errCount;
               DIAG_TRACE(("%s() Num of TRM Errors are %d \r\n",__FUNCTION__,errCount));
            }
            else
            {
                infoParam->retCode = TRMMgr_ERR_FAILED;
                DIAG_WARN(("Failed to Get Diag Info  for GET_CONNECTION_ERRORS \r\n"));
            }  
        }                             
        break;

        case TRMMgr_MSG_TYPE_GET_CONNECTION_ERRORS:
        {
            DIAG_WARN(("%s : GET_CONNECTION_ERRORS  \r\n",__FUNCTION__));
            ret = TRMMgrHelperImpl::getInstance().getTRMConnectionEventList(&length,&responseMsg[0]);
            if(ret)
            {
                infoParam->bufLen  = length;
                memcpy(infoParam->buf,responseMsg,length);
                DIAG_TRACE(("%s() Length of TRM Message is %d \r\n",__FUNCTION__,length));
            }
            else
            {
                infoParam->retCode = TRMMgr_ERR_FAILED;
                DIAG_WARN(("Failed to Get Diag Info  for GET_CONNECTION_ERRORS \r\n"));
            }  

        }                             
        break;
   
        case TRMMgr_MSG_TYPE_GET_TUNER_CONFLICTS:
        {
            DIAG_WARN(("%s : GET_TUNER_CONFLICTS  \r\n",__FUNCTION__));
            
            if(ret)
            {
                infoParam->bufLen  = length;
                memcpy(infoParam->buf,responseMsg,length);
                DIAG_TRACE(("%s() Length of TRM Message is %d \r\n",__FUNCTION__,length));
                
            }
            else
            {
                infoParam->retCode = TRMMgr_ERR_FAILED;
                DIAG_WARN(("Failed to Get Diag Info  for GET_TUNER_CONFLICTS \r\n"));
            }  
        }                             
        break;

        case TRMMgr_MSG_TYPE_GET_TRM_VERSION:
        {
            DIAG_WARN(("%s : GET_TRM_VERSION  \r\n",__FUNCTION__));
         
            bool ret = TRMMgrHelperImpl::getInstance().getVersion(&length,&responseMsg[0]);
            
            if(ret)
            {
                infoParam->bufLen  = length;
                memcpy(infoParam->buf,responseMsg,length);
                DIAG_TRACE(("%s() Length of TRM Message is %d \r\n",__FUNCTION__,length));
            }
            else
            {
                DIAG_WARN(("%s() Failed to Get Diag Info  for GET_TRM_VERSION \r\n",__FUNCTION__));
            }  
        }                             
        break;
      
        default:
        {
            DIAG_WARN(("%s Error: Diag Info not supported   \r\n",__FUNCTION__));
            infoParam->retCode = TRMMgr_ERR_INVALID_PARAM;
        }
        break;
    }

   DIAG_TRACE(("Exit %s():%d with %s \r\n" , __FUNCTION__, __LINE__,ret?"sucess":"failure"));

   return IARM_RESULT_SUCCESS;
}


/**************************************************************************
Function name : TRM Manager Main routine

Description   : Initialize with IARM  and start the helper routine to get the 
TRM Message. 
**************************************************************************/
int main(int argc, char *argv[]) 
{

    uint32_t length = 0;

	 /* Line Buffering*/
    setvbuf(stdout, NULL, _IOLBF, 0);
   

   DIAG_TRACE(("TRM Manager Enter %s():%d \r\n" , __FUNCTION__, __LINE__));


    /* Initialization with IARM and Register RPC for the clients */
    IARM_Bus_Init(IARM_BUS_TRMMGR_NAME);
    IARM_Bus_Connect();
    IARM_Bus_RegisterCall(IARM_BUS_TRMMGR_API_GetTRMDiagInfo,_GetTRMDiagInfo);

    /*Mutex Init*/
	pthread_mutex_init(&trmMgr_mutex, NULL);

    /* Init with TRMMgr helper Implementation */
    TRMMgrHelperImpl::getInstance().init();

    /* Get TRM Version */
    memset(responseMsg,0,sizeof(responseMsg));
   bool ret = TRMMgrHelperImpl::getInstance().getVersion(&length,&responseMsg[0]);
            
	while(1)
	{
        DIAG_WARN(("HeartBeat : TRM Manager !!\r\n"));
        sleep(300);
	}

    IARM_Bus_Disconnect();
    IARM_Bus_Term();
	
    DIAG_TRACE(("TRM Manager Exit %s():%d \r\n" , __FUNCTION__, __LINE__));
	
    return 0;
}

