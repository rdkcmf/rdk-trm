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




/*C++ include*/
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
#include <stdint.h>
#include <vector>
#include <memory>
#include <limits>
#include <iostream>
#include <map>
#include <string>

/*IARM  include*/
#include "libIBus.h"


/*TRM Manager  include*/
#include "TRMMgr.h"



/* TRM Include*/
#include "trm/Messages.h"
#include "trm/MessageProcessor.h"
#include "trm/Activity.h"
#include "trm/JsonEncoder.h"
#include "trm/JsonDecoder.h"
#include "trm/TRM.h"
#include "trm/Klass.h"
#include "trm/TunerReservation.h"
#include "trm/TunerReservation.h"

using namespace TRM;

static void getDiagParameter(int type);
static void processBuffer( const char* buf, int len);

/* Time stamp information */
#define __TIMESTAMP() do { /*YYMMDD-HH:MM:SS:usec*/               \
    struct tm __tm;                                             \
    struct timeval __tv;                                        \
    gettimeofday(&__tv, NULL);                                  \
    localtime_r(&__tv.tv_sec, &__tm);                           \
    printf("\r\n[tid=%ld]:TestDiag %02d%02d%02d-%02d:%02d:%02d:%06d ",                 \
    syscall(SYS_gettid), \
    __tm.tm_year+1900-2000,                             \
    __tm.tm_mon+1,                                      \
    __tm.tm_mday,                                       \
    __tm.tm_hour,                                       \
    __tm.tm_min,                                        \
    __tm.tm_sec,                                        \
    (int)__tv.tv_usec);                                      \
} while(0);


int main(int argc, char *argv[]) 
{
   
   if (argc != 2) {
         printf("%s : <Type - 0 - TRM Version | 1 - IN BAND TUNER | 2 - Device ID | 3  TUNER_RESERVATION | \r\n  4-CONNECTION_ERRORS   | 5-TUNER_CONFLICTS   \r\n", argv[0]);
        return 0;
    }

    int type = atoi((const char *)argv[1]);

    if (type >= TRMMgr_MSG_TYPE_MAX) 
    {
        printf("%s : <Type - 0 - TRM Version | 1 - IN BAND TUNER | 2 - Device ID | 3  TUNER_RESERVATION | \r\n  4-CONNECTION_ERRORS   | 5-TUNER_CONFLICTS   \r\n", argv[0]);

        return 0;
    }

	printf("Enter %s():%d \n" , __FUNCTION__, __LINE__);

    /* Initialization with IARM and Register RPC for the clients */
    IARM_Bus_Init("getTRMDiagInfo");
    IARM_Bus_Connect();
    
    getDiagParameter(type);

    IARM_Bus_Disconnect();
    IARM_Bus_Term();

	return 0;
}



static void getDiagParameter(int type)
{
    TRMDiagInfo_t infoParam;
   
    __TIMESTAMP();printf("Enter %s():%d \n" , __FUNCTION__, __LINE__);
    
    memset(&infoParam, 0, sizeof(infoParam));

    switch (type)
    {
        
        case TRMMgr_MSG_TYPE_GET_TRM_VERSION:
        {
             __TIMESTAMP();printf("%s : Get Diag Info  for GET_TRM_VERSION  \r\n",__FUNCTION__);

            infoParam.retCode = TRMMgr_ERR_UNKNOWN;
            infoParam.bufLen  = 0;
            infoParam.msgType = TRMMgr_MSG_TYPE_GET_TRM_VERSION;    

            IARM_Bus_Call(IARM_BUS_TRMMGR_NAME,
                    (char *)IARM_BUS_TRMMGR_API_GetTRMDiagInfo,
                    (void *)&infoParam,
                    sizeof(infoParam));

            if (infoParam.retCode == TRMMgr_ERR_NONE)
            {
                __TIMESTAMP();printf("%s() Get Diag Info  Return Sucess for GET_TRM_VERSION \r\n",__FUNCTION__);
                processBuffer(&(infoParam.buf[0]),infoParam.bufLen);
            }

        }                             
        break;


        case TRMMgr_MSG_TYPE_GET_NUM_IN_BAND_TUNERS:
        {
             __TIMESTAMP();printf("%s : Get Diag Info  for GET_NUM_IN_BAND_TUNERS  \r\n",__FUNCTION__);

            infoParam.retCode = TRMMgr_ERR_UNKNOWN;
            infoParam.bufLen  = 0;
            infoParam.numOfTuner = 0;
            infoParam.msgType = TRMMgr_MSG_TYPE_GET_NUM_IN_BAND_TUNERS;    

            IARM_Bus_Call(IARM_BUS_TRMMGR_NAME,
                    (char *)IARM_BUS_TRMMGR_API_GetTRMDiagInfo,
                    (void *)&infoParam,
                    sizeof(infoParam));

            if (infoParam.retCode == TRMMgr_ERR_NONE)
            {
                __TIMESTAMP();printf("%s() Get Diag Info  Return Sucess for GET_NUM_IN_BAND_TUNERS \r\n",__FUNCTION__);
            }
             __TIMESTAMP();printf("%s : Num of Tuner Supported = %d  \r\n",__FUNCTION__,infoParam.numOfTuner);     

        }                             
        break;


        case TRMMgr_MSG_TYPE_GET_CONN_DEVICE_IDS:
        {
            __TIMESTAMP();printf("%s() Get Diag Info  for GET_CONN_DEVICE_IDS \r\n",__FUNCTION__);
    
            infoParam.retCode = TRMMgr_ERR_UNKNOWN;
            infoParam.bufLen  = 0;
            infoParam.msgType = TRMMgr_MSG_TYPE_GET_CONN_DEVICE_IDS;    

            IARM_Bus_Call(IARM_BUS_TRMMGR_NAME,
                    (char *)IARM_BUS_TRMMGR_API_GetTRMDiagInfo,
                    (void *)&infoParam,
                    sizeof(infoParam));

            if (infoParam.retCode == TRMMgr_ERR_NONE)
            {
                __TIMESTAMP();printf("%s() Get Diag Info  Return Sucess for GET_CONN_DEVICE_IDS \r\n",__FUNCTION__);
                processBuffer(&(infoParam.buf[0]),infoParam.bufLen);
            }
        }                             
        break;
   
        case TRMMgr_MSG_TYPE_GET_TUNER_RESERVATION:
        {
            __TIMESTAMP();printf("%s : Get Diag Info  for GET_TUNER_RESERVATION \r\n",__FUNCTION__);

            infoParam.retCode = TRMMgr_ERR_UNKNOWN;
            infoParam.bufLen  = 0;
            infoParam.msgType = TRMMgr_MSG_TYPE_GET_TUNER_RESERVATION;    

            IARM_Bus_Call(IARM_BUS_TRMMGR_NAME,
                    (char *)IARM_BUS_TRMMGR_API_GetTRMDiagInfo,
                    (void *)&infoParam,
                    sizeof(infoParam));

            if (infoParam.retCode == TRMMgr_ERR_NONE)
            {
                __TIMESTAMP();printf("%s() Get Diag Info  Return Sucess for GET_TUNER_RESERVATION \r\n",__FUNCTION__);
                processBuffer(&(infoParam.buf[0]),infoParam.bufLen);
            }

              
        }                             
        break;

        case TRMMgr_MSG_TYPE_GET_NUM_TRM_ERRORS:
        {
            __TIMESTAMP();printf("%s : Get Diag Info  for GET_NUM_TRM_ERRORS \r\n",__FUNCTION__);

            infoParam.retCode = TRMMgr_ERR_UNKNOWN;
            infoParam.bufLen  = 0;
            infoParam.numOfTRMError = 0;
            infoParam.msgType = TRMMgr_MSG_TYPE_GET_NUM_TRM_ERRORS;    

            IARM_Bus_Call(IARM_BUS_TRMMGR_NAME,
                    (char *)IARM_BUS_TRMMGR_API_GetTRMDiagInfo,
                    (void *)&infoParam,
                    sizeof(infoParam));

            if (infoParam.retCode == TRMMgr_ERR_NONE)
            {
                __TIMESTAMP();printf("%s() Get Diag Info  Return Sucess for GET_NUM_TRM_ERRORS \r\n",__FUNCTION__);
                __TIMESTAMP();printf("%s : Num of TRM Errors = %d  \r\n",__FUNCTION__,infoParam.numOfTRMError);     
            }
           
        }                             
        break;

      
        case TRMMgr_MSG_TYPE_GET_CONNECTION_ERRORS:
        {
           __TIMESTAMP();printf("%s : Get Diag Info  for GET_CONNECTION_ERRORS \r\n",__FUNCTION__);

            infoParam.retCode = TRMMgr_ERR_UNKNOWN;
            infoParam.bufLen  = 0;
            infoParam.msgType = TRMMgr_MSG_TYPE_GET_CONNECTION_ERRORS;    

            IARM_Bus_Call(IARM_BUS_TRMMGR_NAME,
                    (char *)IARM_BUS_TRMMGR_API_GetTRMDiagInfo,
                    (void *)&infoParam,
                    sizeof(infoParam));

            if (infoParam.retCode == TRMMgr_ERR_NONE)
            {
                __TIMESTAMP();printf("%s() Get Diag Info  Return Sucess for GET_CONNECTION_ERRORS \r\n",__FUNCTION__);
                processBuffer(&(infoParam.buf[0]),infoParam.bufLen);
            }
           
        }                             
        break;
   
        case TRMMgr_MSG_TYPE_GET_TUNER_CONFLICTS:
        {
             __TIMESTAMP();printf("%s : Get Diag Info  for GET_TUNER_CONFLICTS  \r\n",__FUNCTION__);
    
        }                             
        break;

        default:
        {
            __TIMESTAMP();printf("%s Error: Diag Info not supported   \r\n",__FUNCTION__);
        }
        break;
    }

   __TIMESTAMP();printf("Exit  %s():%d \n" ,__FUNCTION__,__LINE__);
}


/**
 * @brief This function is used to Deocode the parse messafe from TRM Srv
 */
class MsgProcessor : public TRM::MessageProcessor
{

public :
    void operator() (const TRM::GetVersionResponse &msg);
    void operator() (const TRM::GetAllReservationsResponse &msg);
    void operator() (const TRM::GetAllConnectedDeviceIdsResponse &msg);
    void operator() (const TRM::GetTRMConnectionEvents &msg);
};




/**
 * @brief Version Response Message
 */
void MsgProcessor::operator() (const TRM::GetAllConnectedDeviceIdsResponse &msg)
{
   
    int statusCode = msg.getStatus().getStatusCode();
    if (TRM::ResponseStatus::kOk == statusCode)
    {
        __TIMESTAMP();printf("(GetAllConnectedDeviceIdsResponse) Sucess = %d\r\n",statusCode);

        const std::list<std::string> & DeviceIds = msg.getDeviceIds();
        std::list<std::string>::const_iterator it;
        
        for (it = DeviceIds.begin(); it != DeviceIds.end(); it++) {
            __TIMESTAMP();printf("conDeviceIDs = %s \r\n",(*it).c_str());
        }
    }
    else
    {
        __TIMESTAMP();printf("(GetAllConnectedDeviceIdsResponse) StatusCode = %d \r\n",statusCode);
    }
    
}


/**
 * @brief Version Response Message
 */
void MsgProcessor::operator() (const TRM::GetVersionResponse &msg)
{
    int statusCode = msg.getStatus().getStatusCode();
    if (TRM::ResponseStatus::kOk == statusCode)
    {
        __TIMESTAMP();printf("(GetVersionResponse) Sucess = %d \r\n",statusCode);
        __TIMESTAMP();printf("(TRM Version is ) = %s\r\n",msg.getVersion().c_str());
    }
    else
    {
        __TIMESTAMP();printf("(GetVersionResponse) StatusCode = %d \r\n",statusCode);
    }
}




/**
 * @brief TRM GetAllReservationsResponse Message 
 */
void MsgProcessor::operator() (const TRM::GetAllReservationsResponse &msg)
{

    int statusCode = msg.getStatus().getStatusCode();
    
    if (TRM::ResponseStatus::kOk == statusCode)
    {
        __TIMESTAMP();printf("(GetAllReservationsResponse) Sucess = %d\r\n",statusCode);
       
        const std::map<std::string, std::list<TunerReservation> > & allReservations = msg.getAllReservations();
        std::map<std::string, std::list<TunerReservation> >::const_iterator it;
        int tuner = 0;
        for (it = allReservations.begin(); it != allReservations.end(); it++)
        {
            const std::list<TunerReservation> & tunerReservations = it->second;
            std::list<TunerReservation>::const_iterator it1;
            
            __TIMESTAMP();printf("Information  for Tuner Reservation %d   \r\n",tuner);   
            for (it1 = tunerReservations.begin(); it1 != tunerReservations.end(); it1++) 
            {
               
               __TIMESTAMP();printf("Info for Tuner %d   \r\n",tuner);   
               __TIMESTAMP();printf("----------------------------------------------------------- \r\n");    
               __TIMESTAMP();printf("reservationToken  - %s \r\n",(*it1).getReservationToken().c_str());
               __TIMESTAMP();printf("device   - %s  \r\n",(*it1).getDevice().c_str());
               __TIMESTAMP();printf("serviceLocator -  %s \r\n",(*it1).getServiceLocator().c_str());
               __TIMESTAMP();printf("startTime - %llu \r\n",(*it1).getStartTime());
               __TIMESTAMP();printf("duration - %llu \r\n",(*it1).getDuration());
              
               if ((*it1).getActivity() == Activity::kLive) {
                __TIMESTAMP();printf("Tuner is reserved for Live  \r\n");
               }
               else if ((*it1).getActivity() == Activity::kRecord){
                __TIMESTAMP();printf("Tuner is reserved for Record  \r\n");

               }
               else if ((*it1).getActivity() == Activity::kEAS){
                __TIMESTAMP();printf("Tuner is reserved for EAS  \r\n");

               }
               else if ((*it1).getActivity() == Activity::kNone){

                __TIMESTAMP();printf("Tuner is reserved for None and Free  \r\n");
               }
               __TIMESTAMP();printf("----------------------------------------------------------- \r\n");    
               tuner++;          
            }
        }
    }
    else
    {
        __TIMESTAMP();printf("(GetAllReservationsResponse) StatusCode = %d \r\n",statusCode);
    }
    
}



/**
 * @brief Version Response Message
 */
void MsgProcessor::operator() (const TRM::GetTRMConnectionEvents &msg)
{
   
    int statusCode = msg.getStatus().getStatusCode();
    if (TRM::ResponseStatus::kOk == statusCode)
    {
        __TIMESTAMP();printf("(GetTRMConnectionEvents) Sucess = %d\r\n",statusCode);
       
    }
    else
    {
        __TIMESTAMP();printf("(GetTRMConnectionEvents) StatusCode = %d \r\n",statusCode);
    }
}


/**
 * @brief This function is used to Deocode the parse messafe from TRM Srv
 */
static void processBuffer( const char* buf, int len)
{
    if (buf != NULL)
    {
        __TIMESTAMP();printf("Response %s \r\n", buf);
        __TIMESTAMP();printf("Response Length  %d - %d\r\n", strlen(buf),len);

        std::vector<uint8_t> response;
        response.insert( response.begin(), buf, buf+len);

        MsgProcessor msgProc;
        TRM::JsonDecoder jdecoder( msgProc);
        jdecoder.decode( response);
    }
}

