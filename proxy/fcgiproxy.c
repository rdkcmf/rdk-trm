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
* @defgroup proxy
* @{
**/


#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include "rdk_debug.h"
#include "safec_lib.h"

enum MessageType {
	REQUEST = 0x1234,
	RESPONSE = 0x1800,
	NOTIFICATION = 0x1400,
	UNKNOWN,
};

static int trm_socket_fd = -1;

static int connect_to_trm(const char *ip, int port, int *trm_fd)
{
	int socket_fd = -1;
	int socket_error = 0;
	struct sockaddr_in trm_address = {0}; /*Initialize the struct - Coverty DELIA-4581*/


	trm_address.sin_family = AF_INET;
	trm_address.sin_addr.s_addr = inet_addr(ip);
	trm_address.sin_port = htons(port);

	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	fprintf(stderr, "Connecting to remote\r\n");
	while(1) {
		static int retry_count = 10;
	   socket_error = connect(socket_fd, (struct sockaddr *) &trm_address, sizeof(struct sockaddr_in));
	   if (socket_error == ECONNREFUSED  && retry_count > 0) {
			fprintf(stderr, "TRM Server is not started...retry to connect\r\n");
		   sleep(2);
		   retry_count--;
	   }
	   else {
		   break;
	   }
	}

	if (socket_error == 0){
		fprintf(stderr, "Connected\r\n");

		int current_flags = fcntl(socket_fd, F_GETFL, 0);
		current_flags &= (~O_NONBLOCK);
                /* RDKSEC-811 coverity fix-CHECKED_RETURN */
                if (fcntl(socket_fd, F_SETFL, current_flags) < 0) {
                    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TRM", "fcntl failed");
                    close(socket_fd);
                    *trm_fd = -1;
                    socket_error = -1;
                    return socket_error;
                }
		*trm_fd = socket_fd;
	}
	else {
		close(socket_fd);
		*trm_fd = -1;
	}

	return socket_error;
}

#include "fcgi_stdio.h"

int main(int argc, char *argv[])
{
	int opt,itr = 1;
	char *debugConfigFile = NULL;

	printf("Starting TRM-FastCGI-Proxy\r\n");
	/* First create a persistent connection to TRM */
	int is_connected = 0;
	errno_t safec_rc = -1;
	int ind = -1;

	while (itr < argc)
        {
                safec_rc = strcmp_s("--debugconfig", strlen("--debugconfig"), argv[itr], &ind);
                ERR_CHK(safec_rc);
                if((safec_rc == EOK) && (ind == 0))
                {
                        itr++;
                        if (itr < argc)
                        {
                                debugConfigFile = argv[itr];
                        }
                        else
                        {
                                break;
                        }
                }
                itr++;
        }

	rdk_logger_init(debugConfigFile);

	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TRM", "This is a test line");

	is_connected = (connect_to_trm(argv[1], atoi(argv[2]), &trm_socket_fd) == 0);

	while (is_connected && FCGI_Accept() >= 0) {
        /* Now start the fcgi loop */
        char *content_length = getenv("CONTENT_LENGTH");
        int payload_length = 0;
        if (content_length != NULL) {
        	payload_length = strtol(content_length, NULL, 10);
    		fprintf(stderr, "content length is %d\r\n", payload_length);
        }
        else {
        	payload_length = 0;
        }

        if (payload_length != 0) {
        	/* First prepend header */
        	static int message_id = 0x1000;
        	const int header_length = 16;
        	unsigned char *buf = (unsigned char *) malloc(payload_length + header_length);
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
        	buf[idx++] = (message_id & 0xFF000000) >> 24;
        	buf[idx++] = (message_id & 0x00FF0000) >> 16;
        	buf[idx++] = (message_id & 0x0000FF00) >> 8;
        	buf[idx++] = (message_id & 0x000000FF) >> 0;
        	/* Payload length */
        	buf[idx++] = (payload_length & 0xFF000000) >> 24;
        	buf[idx++] = (payload_length & 0x00FF0000) >> 16;
        	buf[idx++] = (payload_length & 0x0000FF00) >> 8;
        	buf[idx++] = (payload_length & 0x000000FF) >> 0;
        	/* Read payload from fastcgi */
        	int read_fcgi_count = fread((void *)&buf[idx], 1, payload_length, stdin);
        	if (read_fcgi_count == 0) {
        		free(buf);
        		continue;
        	}

        	fprintf(stderr, "At %d Read from FastCGI %d vs expected %d\r\n", idx, read_fcgi_count, payload_length);
        	fprintf(stderr, "\r\n========REQUEST MSG================================================\r\n[");
        	for (idx = 0; idx < (header_length); idx++) {
            	fprintf(stderr, "%02x", buf[idx]);
        	}
        	for (; idx < (payload_length + header_length); idx++) {
            	fprintf(stderr, "%c", buf[idx]);
        	}

        	fprintf(stderr, "]At %d\r\n==============================================================\r\n", idx);

        	/* Write payload from fastcgi to TRM */
        	int write_trm_count = write(trm_socket_fd, buf, payload_length + header_length);
			fprintf(stderr, "Send to TRM  %d vs expected %d\r\n", write_trm_count, payload_length + header_length);
			free(buf);

        	if (write_trm_count != 0) {
				buf = (char *) malloc(header_length);
				/* Read Response from TRM, read header first, then payload */
				int read_trm_count = read(trm_socket_fd, buf, header_length);
				fprintf(stderr, "Read Header from TRM %d vs expected %d\r\n", read_trm_count, header_length);
	        	fprintf(stderr, "\r\n=====RESPONSE HEADER===================================================\r\n[");

	        	for (idx = 0; idx < (header_length); idx++) {
	            	fprintf(stderr, "%02x", buf[idx]);
	        	}
	        	fprintf(stderr, "\r\n==============================================================\r\n[");

				if (read_trm_count == header_length) {
					int payload_length_offset = 12;
					payload_length =((((unsigned char)(buf[payload_length_offset+0])) << 24) |
									 (((unsigned char)(buf[payload_length_offset+1])) << 16) |
									 (((unsigned char)(buf[payload_length_offset+2])) << 8 ) |
									 (((unsigned char)(buf[payload_length_offset+3])) << 0 ));

					free(buf);
					fprintf(stderr, " TRM Response payloads is %d and header %d\r\n", payload_length, header_length);
                    fflush(stderr);

					buf = (char *) malloc(payload_length);
					read_trm_count = read(trm_socket_fd, buf, payload_length);
					fprintf(stderr, "Read Payload from TRM %d vs expected %d\r\n", read_trm_count, payload_length);

					if (read_trm_count != 0) {
						/* Write Response from TRM to fastcgi */
					    printf("Content-type: text/html\r\n"
					    	   "Content-length:%d\r\n"
						       "Content-type:application/json\r\n"
					           "\r\n",
					           payload_length);

						int write_fcgi_count = fwrite((void *)buf, 1, payload_length, stdout);
						fprintf(stderr, "Send to FastCGI  %d vs expected %d\r\n", write_fcgi_count, payload_length);
						if (write_fcgi_count == 0) {
							free(buf);
							continue;
						}
					}
					else {
		        		/* retry connect after payload-read failure*/
		        		is_connected = 0;
					}
					free(buf);
				}
				else {
					free(buf);
	        		/* retry connect after header-read failure */
	        		is_connected = 0;
				}
        	}
        	else {
        		/* retry connect after write failure*/
        		is_connected = 0;
        	}
        }

        if (!is_connected) {
		    printf("Content-type: text/html\r\n"
		    	   "Content-length:%d\r\n"
			       "Content-type:application/json\r\n"
		           "\r\n",
		           payload_length);

        	FCGI_Finish();
        	is_connected = (connect_to_trm(argv[1], atoi(argv[2]), &trm_socket_fd) == 0);
        }
	}

	fprintf(stderr, "Out of FCGI Loop\r\n");
}


/** @} */
/** @} */
