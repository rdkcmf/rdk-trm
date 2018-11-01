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

/* A Helper module for TRM Manager which would process all the request for TRM Manager */


/*Helper header*/
#include "Helper.h"
#include "Debug.h"

/*For Socket*/
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>

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

/* TRM Include*/
#include "trm/Messages.h"
#include "trm/MessageProcessor.h"
#include "trm/Activity.h"
#include "trm/JsonEncoder.h"
#include "trm/JsonDecoder.h"
#include "trm/TRM.h"
#include "trm/Klass.h"
#include "trm/TunerReservation.h"
#include <pthread.h>

using namespace TRM;

/*Connection and Message Processing Info*/

/* Static variables */
#define MAX_PAYLOAD_LEN 		32767
static int trm_diag_fd = -1;
static const char* ip ="127.0.0.1";
static int  port = 9987;
static int is_connected = 0;
static const uint32_t kTRMMgrClientId = 0XFFFFFF04;
static bool TrmResponseRcvd = false;
static uint32_t numofTRMErrors = 0;


/* Static functions */
static int connect_to_trm();
static void processBuffer( const char* , int );
static void * ProcessTRMMessage (void* arg);
static bool url_request_post( const char *payload, int payload_length, unsigned int clientId);
static char responseStr[MAX_PAYLOAD_LEN];
static bool waitForTRMResponse();
static bool  getAllTunerReservationsResponse();


enum Type {
	REQUEST = 0x1234,
	RESPONSE = 0x1800,
	NOTIFICATION = 0x1400,
	UNKNOWN,
};



/* list of all connected device IDs. */
std::list<std::string> conDeviceList;
/* list of all connected Events. */
std::list<NotifyClientConnectionEvent> helperConEventLists;


class MsgProcessor : public TRM::MessageProcessor
{
public :
    void operator() (const TRM::GetVersionResponse &msg);
    void operator() (const TRM::GetAllReservationsResponse &msg);
    void operator() (const TRM::NotifyClientConnectionEvent &msg);
    void operator() (const TRM::NotifyTunerReservationConflicts &msg);
    void operator() (const TRM::ReserveTunerResponse &msg);


};


/* For connection sync */
static pthread_mutex_t Helper_mutex;

class HelperLock_
{
public:
    HelperLock_(void) {
        pthread_mutex_lock(&Helper_mutex);
    }
    ~HelperLock_(void) {
        pthread_mutex_unlock(&Helper_mutex);
    }
};
#define HelperLock() HelperLock_ t()

bool  TRMMgrHelperImpl::inited = false;

TRMMgrHelperImpl::TRMMgrHelperImpl()
{
	DIAG_TRACE((" %s():%d \r\n" , __FUNCTION__, __LINE__));
}

TRMMgrHelperImpl::~TRMMgrHelperImpl()
{
	DIAG_TRACE(("%s():%d \r\n" , __FUNCTION__, __LINE__));
}


/**
 * @brief Init and connect with TRM
 * 
 */
void TRMMgrHelperImpl::init()
{
	DIAG_TRACE(("Enter %s():%d \r\n" , __FUNCTION__, __LINE__));
	
	if ( false == inited )
	{

		/*Connect To TRM */
		connect_to_trm();

		/*Mutex Init*/
		pthread_mutex_init(&Helper_mutex, NULL);
		
		/* Create a Thread to Process Message */
		pthread_t trm_process_message_thread;
		pthread_create(&trm_process_message_thread, NULL,ProcessTRMMessage, (void *)trm_diag_fd);
		
		inited = true;
	}
	DIAG_TRACE(("Exit %s():%d \r\n" , __FUNCTION__, __LINE__));
}


/**
 * @brief Instance of TRMMgrHelperImpl object.
 * 
 */
TRMMgrHelperImpl &TRMMgrHelperImpl::getInstance()
{
	static TRMMgrHelperImpl TRMMgrHelperInstance;
	return TRMMgrHelperInstance;
}


/**
 * @brief Get TRM Version Number
 */


bool  TRMMgrHelperImpl::getVersion(uint32_t *length,char *responseMsg)
{
    bool ret = false;
    std::vector<uint8_t> out;

    uuid_t value;
    char guid[64];
    uuid_generate(value);
    uuid_unparse(value, guid);

   
    DIAG_TRACE(("Enter %s():%d \r\n" , __FUNCTION__, __LINE__));
   
   	if (NULL == responseMsg)
    {
    	DIAG_WARN(("responseMsg is NULL \r\n"));
    	return false;
    }

    TRM::GetVersion msg(guid, "");
    JsonEncode(msg, out);
    out.push_back(0);

    int len = strlen((const char*)&out[0]);
    int retry_count = 5;

    do
    {
        TrmResponseRcvd = false;
        memset(responseStr,0,sizeof(responseStr));
        ret = url_request_post( (char *) &out[0], len, kTRMMgrClientId);
        retry_count --;
    } while ((ret == false) && (retry_count >0));

    if (ret == true)
    {
    	// Wait for a response
        ret = waitForTRMResponse();
    }


	// copy to response message for Requested Client
	strncpy(responseMsg,responseStr,strlen(responseStr));
	*length = strlen(responseStr);
     responseMsg[*length + 1] = '\0';
	
	DIAG_DEBUG(("Message Length of getVersion response  = %d ...\r\n",*length));
	DIAG_TRACE(("Exit %s():%d \r\n" , __FUNCTION__, __LINE__));

    return ret;

}


/**
 * @brief Send a TRM Request to get all Reservation 
 * Get the Response and send to TRM Mgr.
 */
bool  TRMMgrHelperImpl::getTunerReservationsList(uint32_t *length,char *responseMsg)
{
   
   
   	DIAG_TRACE(("Enter %s():%d \r\n" , __FUNCTION__, __LINE__));
   
    bool ret = false;

    if (NULL == responseMsg)
    {
    	DIAG_WARN(("responseMsg is NULL ...\r\n"));
    	return false;
    }
   
    /* Get the Message from the Get All Reservation Response Message  */
	ret = getAllTunerReservationsResponse();
	
	if (ret == true)
    {
	
		// copy to response message for Requested Client
        strncpy(responseMsg,responseStr,strlen(responseStr));
        *length = strlen(responseStr);
        responseMsg[*length + 1] = '\0';
        DIAG_DEBUG(("Message Length of getTunerReservationsList = %d ...\r\n",*length));
    }
    else
	{
		ret = false;
		DIAG_WARN(("getAllTunerReservationsResponse Failed ...\r\n"));
	}

	DIAG_TRACE(("Exit %s():%d \r\n" , __FUNCTION__, __LINE__));
	return ret;
}




/**
 * @brief Send a TRM Request to get all connected Device IDs 
 * Get the Response and send to TRM Mgr.
 */
bool TRMMgrHelperImpl::getConnectedDeviceIdList(uint32_t *length,char *responseMsg)
{
	
	DIAG_TRACE(("Enter %s():%d \r\n" , __FUNCTION__, __LINE__));
		
	bool ret = false;

	if (NULL == responseMsg)
    {
    	DIAG_WARN(("responseMsg is NULL ...\r\n"));
    	return false;
    }
		
	/* Get the connected devices from the Get All Reservation Response Message  */
	ret = getAllTunerReservationsResponse();

	if (ret == true)
    {
       	/* Get the UUID and Response Message */
		TRM::ResponseStatus responseStatus(ResponseStatus::kOk);
		std::vector<uint8_t> out;
	    uuid_t value;
	    char guid[64];
	    uuid_generate(value);
	    uuid_unparse(value, guid);

		/* Frame the Connected Device Message */
		TRM::GetAllConnectedDeviceIdsResponse msg(guid,responseStatus,conDeviceList);
	    
	    // Encode the message 
	    msg.print();
	    JsonEncode(msg, out);
	    out.push_back(0);
	    
	    // copy to response message for Requested Client
	    std::copy(out.begin(), out.end(),responseMsg);
		responseMsg[out.size()] = '\0';
	    *length = out.size();
	   
	    DIAG_DEBUG(("Message Length of GetAllConnectedDeviceIdsResponse = %d ...\r\n",*length));
	}
	else
	{
		ret = false;
		DIAG_WARN(("getAllTunerReservationsResponse Failed --->\r\n"));
	
	}
	
	DIAG_TRACE(("Exit %s():%d \r\n" , __FUNCTION__, __LINE__));

	return ret;
}


/**
 * @brief Send a TRM Request to get all Reservation 
 * Get the Response and send to TRM Mgr.
 */
static bool  getAllTunerReservationsResponse()
{
    bool ret = false;
    std::vector<uint8_t> out;
    uuid_t value;
    char guid[64];
    uuid_generate(value);
    uuid_unparse(value, guid);

    DIAG_TRACE(("Enter %s():%d \r\n" , __FUNCTION__, __LINE__));

    // get Tuner reservation
    TRM::GetAllReservations msg(guid,"");
    
    // Encode the message 
    JsonEncode(msg, out);
    out.push_back(0);
    int len = strlen((const char*)&out[0]);
    int retry_count = 5;

    do
    {
        //Post the message 
        TrmResponseRcvd = false;
        memset(responseStr,0,sizeof(responseStr));
        ret = url_request_post( (char *) &out[0], len, kTRMMgrClientId);
        retry_count --;
    } while ((ret == false) && (retry_count >0));

    if (ret == true)
    {
    	// Wait for a response
        ret = waitForTRMResponse();
    }

    DIAG_TRACE(("Exit %s():%d \r\n" , __FUNCTION__, __LINE__));
    return ret;
}


bool TRMMgrHelperImpl::getNumofTRMErrors(uint32_t *errCount)
{

	DIAG_TRACE(("Enter %s():%d \r\n" , __FUNCTION__, __LINE__));
		
	bool ret = true;

	if (NULL == errCount)
    {
    	DIAG_WARN(("Param errCount is NULL ...\r\n"));
    	return false;
    }
	
	*errCount = numofTRMErrors;

    DIAG_DEBUG(("Num of TRM Errors are %d ...\r\n",*errCount));

	DIAG_TRACE(("Exit %s():%d \r\n" , __FUNCTION__, __LINE__));

	return ret;
}

/**
 * @brief Send a TRM Request to get all connected Device IDs 
 * Get the Response and send to TRM Mgr.
 */
bool TRMMgrHelperImpl::getTRMConnectionEventList(uint32_t *length,char *responseMsg)
{
	
	DIAG_TRACE(("Enter %s():%d \r\n" , __FUNCTION__, __LINE__));
		
	bool ret = true;

	if (NULL == responseMsg)
    {
    	DIAG_WARN(("responseMsg is NULL ...\r\n"));
    	return false;
    }
	
	if (0 == helperConEventLists.size())
	{
		
		DIAG_WARN(("No Event to Send ...\r\n"));
		*length = 0;
    	return true;
    }
	
   	/* Get the UUID and Response Message */
	TRM::ResponseStatus responseStatus(ResponseStatus::kOk);
	std::vector<uint8_t> out;
    uuid_t value;
    char guid[64];
    uuid_generate(value);
    uuid_unparse(value, guid);

	/* Frame the Connected Device Message */
	TRM::GetTRMConnectionEvents msg(guid,responseStatus,helperConEventLists);
    
    // Encode the message 
    JsonEncode(msg, out);
    out.push_back(0);
    
    // copy to response message for Requested Client
    std::copy(out.begin(), out.end(),responseMsg);
	responseMsg[out.size()] = '\0';
    *length = out.size();
   
    DIAG_DEBUG(("Message Length of getTRMConnectionEventList = %d ...\r\n",*length));
	DIAG_TRACE(("Exit %s():%d \r\n" , __FUNCTION__, __LINE__));

	return ret;
}



/**
 * @brief Version Response Message
 */
void MsgProcessor::operator() (const TRM::GetVersionResponse &msg)
{
    HelperLock();
   
    int statusCode = msg.getStatus().getStatusCode();
    if (TRM::ResponseStatus::kOk == statusCode)
    {
        DIAG_DEBUG(("(GetVersionResponse) Sucess = %d\r\n",statusCode));
    }
    else
    {
    	DIAG_WARN(("(GetVersionResponse) Error code  = %d\r\n",statusCode));
	}
	
	TrmResponseRcvd = true;
}




/**
 * @brief TRM GetAllReservationsResponse Message 
 */
void MsgProcessor::operator() (const TRM::GetAllReservationsResponse &msg)
{
    HelperLock();

    int statusCode = msg.getStatus().getStatusCode();
    
    if (TRM::ResponseStatus::kOk == statusCode)
    {
           
        const std::map<std::string, std::list<TunerReservation> > & allReservations = msg.getAllReservations();
		std::map<std::string, std::list<TunerReservation> >::const_iterator it;
		
		/*clear the list */
		conDeviceList.clear();
		for (it = allReservations.begin(); it != allReservations.end(); it++)
		{
			const std::list<TunerReservation> & tunerReservations = it->second;
			std::list<TunerReservation>::const_iterator it1;
			for (it1 = tunerReservations.begin(); it1 != tunerReservations.end(); it1++) {
				if (!((*it1).getDevice().empty()))
				{
					DIAG_DEBUG(("Add device %s to List \r\n",(*it1).getDevice().c_str()));
					conDeviceList.push_back ((*it1).getDevice());
				}
			}
		}
    }
    else
    {
    	DIAG_WARN(("(GetVersionResponse) Error code  = %d\r\n",statusCode));
	}
	
	TrmResponseRcvd = true;
}

/**
 * @brief Get Notify Client Message
 */
void MsgProcessor::operator() (const TRM::NotifyClientConnectionEvent &msg)
{
	HelperLock();
	
	DIAG_DEBUG(("NotifyClientConnectionEvent) Event Name = %s\r\n",msg.getEventName().c_str()));
	DIAG_DEBUG(("NotifyClientConnectionEvent) Device IP = %s\r\n", msg.getClientIP().c_str()));
	DIAG_DEBUG(("NotifyClientConnectionEvent) Time Stamp = %llu\r\n",msg.getEventTimeStamp()));

	/* Update new events to the list*/
	helperConEventLists.push_back(msg);

	/* Increment the Num of Tuner Error Count*/
	numofTRMErrors ++;

	TrmResponseRcvd = true;
}


/**
 * @brief Get Notify Conflict message 
 */
void MsgProcessor::operator() (const TRM::NotifyTunerReservationConflicts &msg)
{
    HelperLock();

    const TRM::ReserveTunerResponse::ConflictCT &conflicts =  msg.getConflicts();
    
    if (conflicts.size() != 0)
    {
        DIAG_WARN(("Diag Helper : Found %d conflict(s)\r\n",conflicts.size()));
        numofTRMErrors++;

        TRM::ReserveTunerResponse::ConflictCT::const_iterator it = conflicts.begin();
        for (it = conflicts.begin(); it != conflicts.end(); it++)
        {
            DIAG_DEBUG(("Device:[%s] with Activity:[%s] Locator:[%s] Token:[%s] is in conflict with:\r\n",
           										 (*it).getDevice().c_str(),
                                                 (const char *)(*it).getActivity().getActivity(),
                                                 (*it).getServiceLocator().c_str(),
                                                 (*it).getReservationToken().c_str()));

            TRM::TunerReservation resv = msg.getTunerReservation();
            DIAG_DEBUG(("Device:[%s] with Activity:[%s] Locator:[%s] Token:[%s]\r\n",
            									 resv.getDevice().c_str(),
                                                 (const char *)resv.getActivity().getActivity(),
                                                 resv.getServiceLocator().c_str(),
                                                 resv.getReservationToken().c_str()));
        }
    }
    TrmResponseRcvd = true;
}



void MsgProcessor::operator() (const TRM::ReserveTunerResponse &msg)
{
    HelperLock();

    TRM::ResponseStatus status = msg.getStatus();
    DIAG_DEBUG(("Received ReserveTunerResponse message \r\n"));

    if ( status == TRM::ResponseStatus::kOk )
    {
        DIAG_DEBUG(("%s - ResponseStatus - kOK \r\n", __FUNCTION__));
        const TRM::ReserveTunerResponse::ConflictCT &conflicts =  msg.getConflicts();
        if (conflicts.size() != 0)
        {
            DIAG_WARN(("%s- Has %d Conflict(s)\r\n", __FUNCTION__,conflicts.size()));
            numofTRMErrors++;
        }
        else
        {
           	DIAG_DEBUG(("%s- Has No(%d) Conflict(s)\r\n", __FUNCTION__,conflicts.size()));
        }
    	
    }
}


/**
 * @brief This function is used to Process the incoming TRM message.
 */
static void* ProcessTRMMessage (void* arg)
{
	int read_trm_count = 0;
	char *buf = NULL;
	const int header_length = 16;
	unsigned int payload_length = 0;

	DIAG_TRACE(("Enter %s():%d \r\n" , __FUNCTION__, __LINE__));

	while (1)
	{
		if (is_connected == 0)
		{
			connect_to_trm();
		}
		if ( is_connected )
		{
			buf = (char *) malloc(header_length);
			if (buf == NULL)
			{
				DIAG_WARN(("%s:%d :  Malloc failed for %d bytes \r\n", __FUNCTION__, __LINE__, header_length));
				continue;
			}
			memset(buf, 0, header_length);
			
					
			/* Read Response from TRM, read header first, then payload */
			read_trm_count = read(trm_diag_fd, buf, header_length);
			
			DIAG_TRACE(("\r\n Read Header from TRM %d vs expected %d\r\n", read_trm_count, header_length));

			#if 0			
				__TIMESTAMP();printf("=====RESPONSE HEADER===================================================\r\n[");
				for (idx = 0; idx < (header_length); idx++) {
					DIAG_TRACE(("%02x:", buf[idx]));
				}
				printf("\r\n:");
			#endif

			if (read_trm_count == header_length) 
			{
				unsigned int payload_length_offset = 12;
				payload_length =((((unsigned char)(buf[payload_length_offset+0])) << 24) |
								 (((unsigned char)(buf[payload_length_offset+1])) << 16) |
								 (((unsigned char)(buf[payload_length_offset+2])) << 8 ) |
								 (((unsigned char)(buf[payload_length_offset+3])) << 0 ));

				if((payload_length > 0) && (payload_length < MAX_PAYLOAD_LEN)) /*Coverity fix, what is the maximum payload size? or how much maximum memory can be assigned*/
				{
					free( buf);
					buf = NULL;
					DIAG_DEBUG(("TRM Response payloads is %d and header %d\r\n", payload_length, header_length));
					fflush(stderr);

					buf = (char *) malloc(payload_length+1);
					memset(buf, 0, payload_length+1);
					read_trm_count = read(trm_diag_fd, buf, payload_length);
					DIAG_DEBUG(("Read Payload from TRM %d vs expected %d\r\n", read_trm_count, payload_length));

					if (read_trm_count != 0) 
					{
						buf[payload_length] = '\0';
						processBuffer(buf, read_trm_count);
						free(buf);
						buf = NULL;
					}
					else 
					{
						/* retry connect after payload-read failure*/
						is_connected = 0;
						free(buf);
						buf = NULL;
						DIAG_WARN(("%s:%d : read_trm_count is  0 \r\n", __FUNCTION__, __LINE__));
						
					}
				}
				else
				{
					/* retry connect after payload-read failure*/

					//is_connected = 0;
					free(buf);
					buf = NULL;
					DIAG_WARN(("%s:%d : payload_length = %d Count Mismatch \r\n", __FUNCTION__, __LINE__,payload_length));
				}
			}
			else 
			{
				DIAG_WARN(("%s:%d :  header-read failure read_trm_count %d \r\n", __FUNCTION__, __LINE__, read_trm_count));
				free(buf);
				buf = NULL;
				/* retry connect after header-read failure */
				is_connected = 0;
			}
		}
		else
		{
			DIAG_WARN(("%s - Not connected- Sleep and retry \r\n", __FUNCTION__));
			sleep(1);
		}
	}
	DIAG_TRACE(("Exit %s():%d \r\n" , __FUNCTION__, __LINE__));

}


/**
 * @brief This function is used to Deocode the parse message from TRM Srv
 */
static void processBuffer( const char* buf, int len)
{
	DIAG_TRACE(("Enter %s():%d \r\n" , __FUNCTION__, __LINE__));

	if (buf != NULL)
	{
		DIAG_DEBUG(("Response %s \r\n", buf));
		DIAG_DEBUG(("Response Length  %d - %d\r\n", strlen(buf),len));

		memset(responseStr,0,sizeof(responseStr));

		std::vector<uint8_t> response;
		strncpy(responseStr,buf,strlen(buf));
		response.insert( response.begin(), buf, buf+len);

		MsgProcessor msgProc;
		TRM::JsonDecoder jdecoder( msgProc);
		jdecoder.decode( response);
	}
	DIAG_TRACE(("Exit %s():%d \r\n" , __FUNCTION__, __LINE__));
}




/**
 * @brief This function is used to connect to TRM Srv
 */
static int connect_to_trm()
{
	int socket_fd ;
	int socket_error = 0;
	struct sockaddr_in trm_address;

	HelperLock();

	DIAG_TRACE(("Enter %s():%d \r\n" , __FUNCTION__, __LINE__));
	
	memset(&trm_address, 0, sizeof(sockaddr_in));

	if (is_connected == 0)
	{
		sleep(5);
		DIAG_WARN(("Connecting to remote %s-%d \r\n",__FUNCTION__, __LINE__));
		trm_address.sin_family = AF_INET;
		trm_address.sin_addr.s_addr = inet_addr(ip);
		trm_address.sin_port = htons(port);
		if (trm_diag_fd == -1 )
		{
			socket_fd = socket(AF_INET, SOCK_STREAM, 0);
		}
		else
		{
			socket_fd = trm_diag_fd;
		}
	
		while(1)
		{
			int retry_count = 10;
			socket_error = connect(socket_fd, (struct sockaddr *) &trm_address, sizeof(struct sockaddr_in));
			if (socket_error == ECONNREFUSED  && retry_count > 0) 
			{
				DIAG_WARN(("TRM Server is not started...retry to connect \r\n" , __FUNCTION__, __LINE__));
				sleep(2);
				retry_count--;
			}
			else 
			{
			   break;
			}
		}

		if (socket_error == 0)
		{
			DIAG_WARN(("%s:%d : Connected \r\n" , __FUNCTION__, __LINE__));
			int current_flags = fcntl(socket_fd, F_GETFL, 0);
			current_flags &= (~O_NONBLOCK);
			fcntl(socket_fd, F_SETFL, current_flags);
			trm_diag_fd = socket_fd;
			is_connected = 1;
		}
		else 
		{
			DIAG_WARN(("%s:%d : socket_error %d, closing socket\r\n" , __FUNCTION__, __LINE__, socket_error));
			close(socket_fd);
			trm_diag_fd = -1;
		}
	}
	
	DIAG_TRACE(("%s:%d : Exit with socket error code %d\r\n",__FUNCTION__, __LINE__, socket_error));
	return socket_error;
}



/**
 * @brief This function is used to  post request to TRM server
 */
static bool url_request_post( const char *payload, int payload_length, unsigned int clientId)
{
    bool ret = false;

    DIAG_TRACE(("Enter %s():%d \r\n" , __FUNCTION__, __LINE__));

    if ( is_connected == 0)
        connect_to_trm();

    if ( is_connected )
    {
        if (payload_length != 0)
        {
            /* First prepend header */
            static int message_id = 0x1000;
            const int header_length = 16;
            unsigned char *buf = NULL;
            buf = (unsigned char *) malloc(payload_length + header_length);
            int idx = 0;
            /* Magic Word */
            buf[idx++] = 'T';
            buf[idx++] = 'R';
            buf[idx++] = 'M';
            buf[idx++] = 'S';
            /* Type, set to UNKNOWN, as it is not used right now*/
            buf[idx++] = (UNKNOWN & 0xFF000000) >> 24;
            buf[idx++] = (UNKNOWN & 0x00FF0000) >> 16;
            buf[idx++] = (UNKNOWN & 0x0000FF00) >> 8;
            buf[idx++] = (UNKNOWN & 0x000000FF) >> 0;
            /* Message id */
            ++message_id;

            DIAG_WARN(("CONNECTION CLIENTID: %02x\r\n",clientId));

            buf[idx++] = (clientId & 0xFF000000) >> 24;
            buf[idx++] = (clientId & 0x00FF0000) >> 16;
            buf[idx++] = (clientId & 0x0000FF00) >> 8;
            buf[idx++] = (clientId & 0x000000FF) >> 0;
            /* Payload length */
            buf[idx++] = (payload_length & 0xFF000000) >> 24;
            buf[idx++] = (payload_length & 0x00FF0000) >> 16;
            buf[idx++] = (payload_length & 0x0000FF00) >> 8;
            buf[idx++] = (payload_length & 0x000000FF) >> 0;

            for (int i =0; i< payload_length; i++)
                buf[idx+i] = payload[i];
            DIAG_TRACE(("====== REQUEST MSG ======\r\n["));
            
            for (idx = 0; idx < (header_length); idx++) {
                DIAG_TRACE(( "%02x", buf[idx]));
            }
            DIAG_TRACE(("]\r\n\n"));

            for (; idx < (payload_length + header_length); idx++) {
                DIAG_TRACE(("%c", buf[idx]));
            }
            DIAG_TRACE(("\n\n"));

            /* Write payload from fastcgi to TRM */
            int write_trm_count = write(trm_diag_fd, buf, payload_length + header_length);
            DIAG_TRACE(("Send to TRM %d vs expected %d\r\n", write_trm_count, payload_length + header_length));
            free(buf);
            buf = NULL;

            if (write_trm_count == 0)
            {
                is_connected = 0;
                DIAG_WARN(("%s():%d : Error in Sending Data to Socket:write_trm_count is now 0 \r\n", __FUNCTION__, __LINE__));
            }
            else
            {
                ret = true;
            }
        }
    }
    else
    {
		DIAG_WARN(("%s():%d : Not Connected to TRM Server\r\n", __FUNCTION__, __LINE__));
    }

    DIAG_TRACE(("Exit %s():%d \r\n" , __FUNCTION__, __LINE__));

    return ret;
}


/**
 * @brief This function waits for TRM respnse */
static bool waitForTRMResponse()
{
    int retry_count = 500;

    DIAG_TRACE(("Enter %s():%d \r\n" , __FUNCTION__, __LINE__));

    while ((false == TrmResponseRcvd) && (retry_count > 0))
    {
		DIAG_DEBUG(("%s():%d ..and loop ..\r\n", __FUNCTION__, __LINE__));
		usleep(10*1000);
		retry_count --;
    }

    if((retry_count == 0))
    {
		DIAG_WARN(("Time out.. waitForTRMResponse %s():%d \r\n" , __FUNCTION__, __LINE__));
		return false;
	}
	DIAG_TRACE(("Exit %s():%d \r\n" , __FUNCTION__, __LINE__));
    return true;
}

