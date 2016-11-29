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
* @defgroup sniffer
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
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>

#include <map>
#include <QCoreApplication>
#include <QRegExp>
#include <QStringList>
#include <QFile>
#include "rdk_debug.h"
#include "qt_websocketproxy.h"

static QString mcastIP;
static QString hostIF;
static int mcastPort;

enum MessageType {
	REQUEST = 0x1234,
	RESPONSE = 0x1800,
	NOTIFICATION = 0x1400,
	UNKNOWN,
};

#define ASSERT_RET(ret) \
    do{\
        if (ret == -1) {\
            printf("call failed at [%d] for [%s]\r\n", __LINE__, strerror(errno));\
            _exit(0);\
        }\
    } while(0)\

static int trm_sniff_fd = -1;
static pthread_mutex_t conn_mutex;
typedef std::map<void *, int>  connection_ct_t;
static connection_ct_t connections;

class AutoLock_
{
public:
    AutoLock_(void) {
        pthread_mutex_lock(&conn_mutex);
    }
    ~AutoLock_(void) {
        pthread_mutex_unlock(&conn_mutex);
    }
};

#define AutoLock() AutoLock_ a()

static int get_connection_id(void)
{
    static int connection_id = 0;
    int connid = 0;
    connection_id++;
    connid = connection_id;
    return connid;
}

static int conn_to_id(void *conn)
{
    int connection_id = -1;

    connection_ct_t::iterator it =  connections.find(conn);
    if (it != connections.end()) {
        connection_id = it->second;
    }
    else {
        __TIMESTAMP(); printf("Cannot find connection\r\n");
        connection_id = -1;
    }

    return connection_id;
}

static void * id_to_conn(int connection_id)
{
    void *conn = 0;

    if (connection_id >= 0) {
        connection_ct_t::iterator it; 
        for (it = connections.begin(); it != connections.end(); it++) {
            __TIMESTAMP(); printf("id_to_conn::Trying Connection Pair [%d] with [%p]\r\n", it->second, it->first);

            if  (connection_id ==  (it->second)) {
                conn = it->first;
                break;
            }
        }
    }

    return conn;
}

static void add_connection(void *conn)
{
    int connection_id = get_connection_id();
    __TIMESTAMP(); printf("Adding Connection [%d] with [%p]\r\n", connection_id, conn);
    connections.insert(connections.end(), std::pair<void*, int>(conn, connection_id));
}

static void remove_connection(void *conn)
{
    __TIMESTAMP(); printf("Removing Connection [%p]\r\n", conn);

    connection_ct_t::iterator it =  connections.find(conn);
    if (it != connections.end()) {
        connections.erase(it);
    }
}

static void remove_all_connections(void)
{
    connection_ct_t::iterator it;
    for (it = connections.begin(); it != connections.end(); it++) {
        __TIMESTAMP(); printf("Removing Connection [%p]\r\n", it->first);
        extern int mg_websocket_close(void * conn);
        mg_websocket_close(it->first);
    }

    connections.clear();

}

static int inet_addr_from_iface(const char *iface, struct in_addr *in)
{
    struct ifreq ifr;
    int dummy_fd;
    int ret = 0;

    dummy_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (dummy_fd >= 0) {
        ifr.ifr_addr.sa_family = AF_INET;
        strncpy(ifr.ifr_name, iface, IFNAMSIZ-1);
        printf("Interface name is %s\r\n", ifr.ifr_name);
        ret = ioctl(dummy_fd, SIOCGIFADDR, &ifr);
        ASSERT_RET(ret);
        *in = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr;;
        printf("Interface addr is %s\r\n", inet_ntoa(*in));
        close(dummy_fd);
    }
    else {
        return -1;
    }

}

static int connect_to_mcast(const char *host, const char *mcast, int port, int *sniff_fd)
{
    int ret = -1;
    int groupfd = -1;
    struct sockaddr_in groupaddr;

    const char *HOST_IPA = host;
    const char *GROUP_IPA = mcast;
 
    groupfd = socket(AF_INET, SOCK_DGRAM, 0);

    memset((void*)&groupaddr, 0, sizeof(groupaddr));
    groupaddr.sin_family = AF_INET;
    groupaddr.sin_port = htons(port);
    groupaddr.sin_addr.s_addr = inet_addr(GROUP_IPA);

    int enable_mcastloop = 1;
    ret = setsockopt(groupfd, SOL_SOCKET, IP_MULTICAST_LOOP, (char *)&enable_mcastloop, sizeof(enable_mcastloop));
    ASSERT_RET(ret);

    char ttl = 4;
    setsockopt(groupfd, IPPROTO_IP, IP_MULTICAST_TTL, (char *)&ttl, sizeof(ttl));


    struct in_addr hostaddr;
    memset((void*)&hostaddr, 0, sizeof(hostaddr));
    inet_addr_from_iface(HOST_IPA, &(hostaddr));
    ret = setsockopt(groupfd, SOL_SOCKET, IP_MULTICAST_IF,  (char *)&hostaddr, sizeof(hostaddr));
    ASSERT_RET(ret);

    int enable_reuseaddr = 1;
    ret = setsockopt(groupfd, SOL_SOCKET, SO_REUSEADDR, (char *)&enable_reuseaddr, sizeof(enable_reuseaddr));
    ASSERT_RET(ret);

    ret = bind(groupfd, (struct sockaddr *) &groupaddr, sizeof(groupaddr));
    ASSERT_RET(ret);

    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(GROUP_IPA);
    mreq.imr_interface.s_addr = (hostaddr.s_addr); 

    ret=setsockopt(groupfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mreq, sizeof(mreq));
    ASSERT_RET(ret);

    *sniff_fd = groupfd;

    printf("SNIFF connected %d\r\n", *sniff_fd);
}

int  begin_request_callback(void */*conn*/)
{
    AutoLock();
   __TIMESTAMP();  printf("[%s]\r\n", __func__);
    return 0;
}

void end_request_callback(void  */*conn*/, int /*reply_status_code*/)
{
    AutoLock();
    __TIMESTAMP(); printf("[%s]\r\n", __func__);
}

int websocket_connect_callback(void */*conn*/)
{
    AutoLock();
    __TIMESTAMP(); printf("[%s]\r\n", __func__);
    return 0;
}

int websocket_disconnect_callback(void *conn)
{
    /* Notify TRM */
    __TIMESTAMP(); printf("[%s] - removing connection from TRM\r\n", __func__);
    extern int websocket_data_callback(void *conn, int flags, char *trm_data, size_t trm_data_length);
    websocket_data_callback(conn, 8, NULL, 0);

    AutoLock();
    __TIMESTAMP(); printf("[%s]\r\n", __func__);
    remove_connection(conn);
    return 0;
}

void websocket_ready_callback(void *conn) 
{
    AutoLock();
    __TIMESTAMP(); printf("[%s]\r\n", __func__);
    add_connection(conn);
}

int log_message_callback(void */*conn*/, const char *message)
{
    AutoLock();
    __TIMESTAMP(); printf("[wsLog][%s]\r\n", message);
    return 0;
}

int websocket_data_callback(void *conn, int flags, char *trm_data, size_t trm_data_length) 
{
    int keep_connection = 1;
    flags = flags;
    __TIMESTAMP(); printf("[%s][%.*s]\r\n", __func__, trm_data_length, trm_data);
    /* Packaget into TRM message. Prexif transport protocol */
    size_t payload_length = trm_data_length;
    int connection_id =  conn_to_id(conn);
    
    if (connection_id >= 0 && trm_data_length >= 0) {
    }
    else {
        __TIMESTAMP(); printf("invalid connection %p\r\n", conn);
    }

    return keep_connection;
}

static void * TRM_traffic_listener(void * /*args*/)
{
    while (1) {

    	/* Retry connect OUTSIDE autoLock */
        while (trm_sniff_fd < 0)  {
        	/* connection to TRM is lost, Reset all WS connections */
        	{AutoLock();
        	}
        	sleep(10);
            __TIMESTAMP(); printf("SNIFF: Retry Connecting to sniffer\r\n");
            connect_to_mcast(hostIF.toUtf8().data(), mcastIP.toUtf8().data(), (mcastPort), &trm_sniff_fd);
        }

        int idx = 0;
        size_t header_length = 4; //4 bytes of length
        unsigned char header[4] = {0};

        int read_packet_count = read(trm_sniff_fd, header, header_length);
        if (read_packet_count == header_length) {
            printf("SNIFF: Read 4 byte of header\r\n");
            size_t payload_length = (header[0] << 24) | (header[1] << 16) | (header[2] << 8) | header[3];
            printf("SNIFF: payload length is %d\r\n", payload_length);
            unsigned char *buf = (unsigned char *) malloc(payload_length);
            if (buf) {
                read_packet_count = read(trm_sniff_fd, buf, payload_length);
                if (read_packet_count == (int)payload_length) {
                    extern int mg_websocket_write(void * conn, const char *data, size_t data_len);
                    AutoLock();
                    /* send to all websocket connections */
                    connection_ct_t::iterator it; 
                    for (it = connections.begin(); it != connections.end(); it++) {
                        __TIMESTAMP(); printf("SNIFF: id_to_conn::Trying Connection Pair [%d] with [%p]\r\n", it->second, it->first);
                        void *conn = it->first;
                        int write_packet_count = mg_websocket_write(conn, (const char *)buf, payload_length);
                        write_packet_count = write_packet_count;
                    }
                }
                else {
                    if (read_packet_count <= 0) {
                        printf("Remote connection is closed...Retry\r\n");
                        close(trm_sniff_fd);
                        trm_sniff_fd = -1;
                    }
                }

                free(buf);
            }
        }
        else {
            printf("Remote connection is closed...Retry\r\n");
            close(trm_sniff_fd);
            trm_sniff_fd = -1;
        }
    }

    return 0;
}

int main(int argc, char *argv[]) 
{
    /* Start the main loop */
    QCoreApplication app(argc, argv);
    QStringList args = app.arguments();

    char *debugConfigFile = NULL;

    QRegExp optionBound("-bound");
    QRegExp optionHostInterface("-hostIF");
    QRegExp optionMcastIP("-mcastIP");
    QRegExp optionMcastPort("-mcastPort");

    QStringList boundIPs;

    for (int i = 1; i < args.size(); ++i) {
        if (optionBound.indexIn(args.at(i)) != -1 ) {   
            boundIPs << args.at(i+1);
            ++i;
            qDebug() << "bound to " << boundIPs;
        } 
        if (optionHostInterface.indexIn(args.at(i)) != -1 ) {   
            hostIF = args.at(i+1);
            ++i;
            qDebug() << "host-interface is " << hostIF;
        }   
        if (optionMcastPort.indexIn(args.at(i)) != -1 ) {   
            mcastPort = args.at(i+1).toInt();
            ++i;
            qDebug() << "mcast-port is " << mcastPort;
        }   
        else if (optionMcastIP.indexIn(args.at(i)) != -1 ) {   
            mcastIP = args.at(i+1);
            ++i;
            qDebug() << "mcast-ip is " << mcastIP;
        }   
    }

    if (argc < 3) {
		printf("TRM-WS-Sniffer-Proxy <-mcastIP> <mcastPort> [optionals]\r\n");
		printf("optionals: -bound <bound ip addr1> <bound ip addr2>...\r\n");
		return 0;
	}

#define TRM_WS_SNIFF_LISTENING_PORT 9989

    //Initiate utilities
    pthread_mutex_init(&conn_mutex, NULL);
    __TIMESTAMP(); printf("Starting TRM-WS-Sniffer-Proxy\r\n");


    connect_to_mcast(hostIF.toUtf8().data(), mcastIP.toUtf8().data(), (mcastPort), &trm_sniff_fd);
    pthread_t trm_traffic_listener_thread;
    pthread_create(&trm_traffic_listener_thread, NULL, TRM_traffic_listener, (void *)trm_sniff_fd);

    if (boundIPs.empty()) {
        boundIPs << QString("0.0.0.0");
    }

    WebSocketProxy wsproxy(boundIPs, TRM_WS_SNIFF_LISTENING_PORT);
    
    int app_return = app.exec();

    //Initiate utilities
    pthread_mutex_destroy(&conn_mutex);

    __TIMESTAMP(); printf("TRM TRM-WS-Sniffer Exit!!\r\n");
    return app_return;
}


/** @} */
/** @} */
