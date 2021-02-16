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


#include <stdio.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <sys/time.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>


#include "uuid/uuid.h"
#include "trm/TRM.h"
#include "Util.h"
#include "safec_lib.h"

TRM_BEGIN_NAMESPACE

static int connect_to_authServer(const char *ip, int port, int *auth_fd)
{
    int socket_fd = -1;
    int socket_error = 0;
    struct sockaddr_in auth_address = {0};
    auth_address.sin_family = AF_INET;
    auth_address.sin_addr.s_addr = inet_addr(ip);
    auth_address.sin_port = htons(port);

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_fd < 0)
    {
        Log() << "Socket creation failed with errno : " << errno;  //CID:18159-Resolve the negative returns
        return -1;
    }
    Log() <<  "Connecting to " << ip << " : " << port << std::endl;
    while(1) {
        int retry_count = 10;
        socket_error = connect(socket_fd, (struct sockaddr *) &auth_address, sizeof(struct sockaddr_in));
        if (socket_error == ECONNREFUSED  && retry_count > 0) {
            Log() << "Auth Server is not started...retry to connect" << std::endl;
            sleep(2);
            retry_count--;
        }
        else {
            break;
        }
    }

    if (socket_error == 0){
        Log() <<  "Connected to " << ip << " : " << port << std::endl;
        int current_flags = fcntl(socket_fd, F_GETFL, 0);
        current_flags &= (~O_NONBLOCK);
        /* RDKSEC-811 coverity fix - CHECKED_RETURN */
        if (fcntl(socket_fd, F_SETFL, current_flags) < 0) {
            Log() <<  "fcntl failed " << std::endl;
            close(socket_fd);
            *auth_fd = -1;
            socket_error = -1;
            return socket_error;
        }
        *auth_fd = socket_fd;
    }
    else {
        close(socket_fd);
        *auth_fd = -1;
    }

    return socket_error;
}

const std::string IntToString(int i)
{
	return dynamic_cast< std::ostringstream & >(( std::ostringstream() << std::dec << (i) ) ).str();
}

const std::string IntToString(int i, int j)
{
	return dynamic_cast< std::ostringstream & >(( std::ostringstream() << std::dec << (i) << "." << (j)) ).str();
}

const std::string GenerateUUID(void)
{
	uuid_t value;
	char guid[64];
	uuid_generate(value);
	uuid_unparse(value, guid);
	return std::string(guid);
}

void HexDump(const std::vector<uint8_t> &in)
{
	const unsigned char *ptr = &in[0];
	size_t len = in.size();

	for (int i = 0; i < len; i++) {
		printf("%02X ", ptr[i]);
	}

	printf("\r\n");
}

uint64_t GetCurrentEpoch(void)
{
	struct timeval tp;
	::gettimeofday(&tp, 0);
	uint64_t now = (uint64_t)(((uint64_t) tp.tv_sec * 1000) + tp.tv_usec / 1000);
	return now;
}

std::ostream & Timestamp (std::ostream &os)
{
    struct tm __tm;                                             
    struct timeval __tv;                                        
    char buf[64];
    errno_t safec_rc = -1;
    gettimeofday(&__tv, NULL);                                  
    localtime_r(&__tv.tv_sec, &__tm);                          
    safec_rc = sprintf_s(buf,sizeof(buf), "%02d%02d%02d-%02d:%02d:%02d:%06d [tid=%ld] ",
            __tm.tm_year+1900-2000,                             
            __tm.tm_mon+1,                                      
            __tm.tm_mday,                                     
            __tm.tm_hour,                                    
            __tm.tm_min,                                    
            __tm.tm_sec,                                   
            (int)__tv.tv_usec, 
            syscall(SYS_gettid));
    if(safec_rc < EOK) {
      ERR_CHK(safec_rc);
    }
    os << buf;
    return os;
}

std::ostream & Log(void) 
{
    return std::cout << Timestamp ;
}

std::string GetAuthToken(const char *generateTokenRequest)
{
	static std::string responseBody;
	std::string response;

	if (responseBody.size() != 0) {
		return responseBody;
	}

    if (generateTokenRequest == NULL) {
        return responseBody;
    }

	if (response.size() == 0) {
		static int  writeCount = strlen(generateTokenRequest) + 1;
		int auth_fd = -1;
	    connect_to_authServer("127.0.0.1", 50050, &auth_fd);
	    if (auth_fd >= 0) {
	    	 int ret = write(auth_fd, generateTokenRequest, writeCount);
	    	 if (ret == writeCount) {
	    		 /* read for response till EOF*/
	    		 while(1) {
		    		 char buf[64] = {'\0'};
		    		 ret = read(auth_fd, buf, sizeof(buf));
					 if (ret > 0) {
						 response.insert(response.end(), buf, buf+ret);
					 }
					 else {
						 break;
					 }
	    		 }
	    	 }
			 close(auth_fd);
	    }
	}

	if (response.size() > 0 && (response.find("200 OK") != std::string::npos)) {
		//Log() << "Response : \r\n [" << response << "] " << std::endl;
		/* Get to the body part */
		size_t startPos = response.find("\r\n\r\n");
		if (startPos != std::string::npos) {
            startPos += 4; //\r\n\r\n
			//Log() << "Json Body is [" << response.substr(startPos) << "]" << std::endl;
			responseBody = response.substr(startPos);
		}
		else {
		}
	}

	return responseBody;
}

const SpecVersion & GetSpecVersion(void)
{
#ifndef TRM_VERSION_MAJOR
#define TRM_VERSION_MAJOR 2
#endif

#ifndef TRM_VERSION_MINOR
#define TRM_VERSION_MINOR 1
#endif

	static const SpecVersion ver(TRM_VERSION_MAJOR, TRM_VERSION_MINOR);
	return ver;
}

TRM_END_NAMESPACE


/** @} */
/** @} */
