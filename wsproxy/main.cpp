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
* @defgroup wsproxy
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
#include <map>
#include <QCoreApplication>
#include <QRegExp>
#include <iostream>
#include <QStringList>
#include <QString>
#include <QSettings>
#include <QNetworkInterface>
#include <QNetworkAddressEntry>
#include "rdk_debug.h"


#include <QtWebSockets/QWebSocketServer>
#include <QtWebSockets/QWebSocket>
#include <QDateTime>

#include "qt_websocketproxy.h"

static const int  header_length = 16;

enum MessageType {
	REQUEST = 0x1234,
	RESPONSE = 0x1800,
	NOTIFICATION = 0x1400,
	UNKNOWN,
};

static const char *trm_ip = 0;
static const char *trm_port = 0;

static int trm_socket_fd = -1;
static pthread_mutex_t conn_mutex;
typedef std::map<void *, int>  connection_ct_t;
static connection_ct_t connections;
const char* localHost="lo";
const char* devicePropertiesPath ="/etc/device.properties";
const char* mocaIntTagName = "MOCA_INTERFACE";

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


static int connect_to_trm(const char *ip, int port, int *trm_fd)
{
    int socket_fd = -1;
    int socket_error = 0;
    struct sockaddr_in trm_address;
    trm_address.sin_family = AF_INET;
    trm_address.sin_addr.s_addr = inet_addr(ip);
    trm_address.sin_port = htons(port);
    memset(trm_address.sin_zero, 0, sizeof(trm_address.sin_zero));

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    __TIMESTAMP(); fprintf(_TRMPRX_OUT_, "Connecting to remote\r\n");
    while(1) {
        static int retry_count = 10;
        socket_error = connect(socket_fd, (struct sockaddr *) &trm_address, sizeof(struct sockaddr_in));
        if (socket_error == ECONNREFUSED  && retry_count > 0) {
            __TIMESTAMP(); fprintf(_TRMPRX_OUT_, "TRM Server is not started...retry to connect\r\n");
            sleep(2);
            retry_count--;
        }
        else {
            break;
        }
    }

    if (socket_error == 0){
        __TIMESTAMP(); fprintf(_TRMPRX_OUT_, "Connected\r\n");

        int current_flags = fcntl(socket_fd, F_GETFL, 0);
        current_flags &= (~O_NONBLOCK);
        fcntl(socket_fd, F_SETFL, current_flags);
        *trm_fd = socket_fd;
    }
    else {
        close(socket_fd);
        *trm_fd = -1;
    }

    return socket_error;
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
   
    QWebSocket *wssocket = (QWebSocket *)conn;

    /* Notify TRM */
    __TIMESTAMP(); printf("[%s] - removing connection from TRM\r\n", __func__);
    extern int websocket_data_callback(void *conn, int flags, char *trm_data, size_t trm_data_length);

        
    /* Notify TRM with Device IP infomration.*/
    if (wssocket) 
    {
        char eventString[256] = "";
        QString ipString = wssocket->peerAddress().toString();
        qint64 currentEpoch = QDateTime(QDate::currentDate(), QTime::currentTime()).toMSecsSinceEpoch();
        std::string reason = "TRM_Event_Disconnect";

        memset(eventString,'\0',sizeof(eventString));

        int length = sprintf(eventString,"{ \"notifyClientConnectionEvent\": { \"eventName\" :\"%s\",\"clientIP\" : \"%s\",\"timeStamp\": %lld } }",reason.c_str(),ipString.toUtf8().data(),currentEpoch);
        websocket_data_callback(conn, 0, &eventString[0],length);

         //__TIMESTAMP(); printf("Disconnected from IP -[%s]  and Size [%d] \r\n",ipString.toUtf8().data(),ipString.size());
        //__TIMESTAMP(); printf("Current Epoch Time -[%lld]  \r\n",currentEpoch);
        //__TIMESTAMP(); printf("Size of the Event string %d  \r\n",length);

    }

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
    int connection_id = -1;
    {AutoLock();
        connection_id =  conn_to_id(conn);
    }
    
    if (connection_id >= 0 && trm_data_length >= 0) {
        /* First prepend header */
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
        /* client  id */
        buf[idx++] = (connection_id & 0xFF000000) >> 24;
        buf[idx++] = (connection_id & 0x00FF0000) >> 16;
        buf[idx++] = (connection_id & 0x0000FF00) >> 8;
        buf[idx++] = (connection_id & 0x000000FF) >> 0;
        /* Payload length */
        buf[idx++] = (payload_length & 0xFF000000) >> 24;
        buf[idx++] = (payload_length & 0x00FF0000) >> 16;
        buf[idx++] = (payload_length & 0x0000FF00) >> 8;
        buf[idx++] = (payload_length & 0x000000FF) >> 0;
        /* Read payload from ws*/

        if (payload_length > 0) {
            memcpy((void *)&buf[idx], trm_data, payload_length);
        }

        for (idx = 0; idx < (header_length); idx++) {
            fprintf(_TRMPRX_OUT_, "%02x", buf[idx]);
        }
        for (; idx < (payload_length + header_length); idx++) {
            fprintf(_TRMPRX_OUT_, "%c", buf[idx]);
        }

        fprintf(_TRMPRX_OUT_, "]At %d\r\n==============================================================\r\n", idx);

        /* Write payload from fastcgi to TRM */
        int write_trm_count = write(trm_socket_fd, buf, payload_length + header_length);
        __TIMESTAMP(); fprintf(_TRMPRX_OUT_, "Send to TRM  %d vs expected %d\r\n", write_trm_count, payload_length + header_length);
        free(buf);
    }
    else {
        __TIMESTAMP(); printf("invalid connection %p\r\n", conn);
    }

    return keep_connection;
}

static void * TRM_response_listener(void * /*args*/)
{
    while (1) {
    	/* Retry connect OUTSIDE autoLock */
        while (trm_socket_fd < 0)  {
        	/* connection to TRM is lost, Reset all WS connections */
        	{AutoLock();
        		remove_all_connections();
        	}
        	sleep(10);
            __TIMESTAMP(); fprintf(_TRMPRX_OUT_, "Retry Connecting to TRM\r\n");
        	connect_to_trm(trm_ip, atoi(trm_port), &trm_socket_fd);
        }
        
        int idx = 0;
        size_t payload_length = 0;
        int connection_id = -1;
        /*Always start reading from 16 header bytes */
        char *buf = (char *) malloc(header_length);
        /* Read Response from TRM, read header first, then payload */
        int read_trm_count = read(trm_socket_fd, buf, header_length);
        __TIMESTAMP(); fprintf(_TRMPRX_OUT_, "Read Header from TRM %d vs expected %d\r\n", read_trm_count, header_length);
        __TIMESTAMP(); fprintf(_TRMPRX_OUT_, "\r\n=====RESPONSE HEADER===================================================\r\n[");

        for (idx = 0; idx < (header_length); idx++) {
            fprintf(_TRMPRX_OUT_, "%02x", buf[idx]);
        }
        __TIMESTAMP(); fprintf(_TRMPRX_OUT_, "\r\n==============================================================\r\n[");

        if (read_trm_count == header_length) {
        	int magic_cookie_offset = 0;
            int connection_id_offset = 8;
            int payload_length_offset = 12;

            if ((buf[magic_cookie_offset+0] != 'T') ||
            	(buf[magic_cookie_offset+1] != 'R') ||
            	(buf[magic_cookie_offset+2] != 'M') ||
            	(buf[magic_cookie_offset+3] != 'S')) {
            	//TODO: close the non-complying connection!
                fprintf(_TRMPRX_OUT_, "Mismatching Magic! Discard\r\n");
            }

            connection_id =((((unsigned char)(buf[connection_id_offset+0])) << 24) |
                    (((unsigned char)(buf[connection_id_offset+1])) << 16) |
                    (((unsigned char)(buf[connection_id_offset+2])) << 8 ) |
                    (((unsigned char)(buf[connection_id_offset+3])) << 0 ));

            if (1) {

                payload_length =((((unsigned char)(buf[payload_length_offset+0])) << 24) |
                        (((unsigned char)(buf[payload_length_offset+1])) << 16) |
                        (((unsigned char)(buf[payload_length_offset+2])) << 8 ) |
                        (((unsigned char)(buf[payload_length_offset+3])) << 0 ));

                free(buf);
                __TIMESTAMP(); fprintf(_TRMPRX_OUT_, " TRM Response payloads is %d and header %d\r\n", payload_length, header_length);
                fflush(_TRMPRX_OUT_);

                buf = (char *) malloc(payload_length);
                read_trm_count = read(trm_socket_fd, buf, payload_length);
                __TIMESTAMP(); fprintf(_TRMPRX_OUT_, "Read Payload from TRM %d vs expected %d\r\n", read_trm_count, payload_length);

                if (read_trm_count == (int)payload_length) {
                    /* Write Response from TRM to fastcgi */
                    AutoLock();
                    printf("Content-type: text/html\r\n"
                            "Content-length:%d\r\n"
                            "Content-type:application/json\r\n"
                            "\r\n",
                            payload_length);

                    void *conn = id_to_conn(connection_id);

                    if (conn) {
#if 0
                        extern int mg_websocket_write(void * conn, const char *data, size_t data_len);
                        size_t write_ws_count = mg_websocket_write(conn, buf, read_trm_count);
                        fprintf(stderr, "Send to WS  %d vs expected %d\r\n", write_ws_count, payload_length + 4/*WS overhead*/);
                        if (write_ws_count == 0 ) {
                            remove_connection(conn);
                        }
#else
                        //emit signal and let main loop handles it.
                        extern int mg_websocket_write(void * conn, const char *data, size_t data_len);
                        int write_ws_count = mg_websocket_write(conn, buf, read_trm_count);
                        write_ws_count = write_ws_count;
#endif
                    }
                    else {
                    	//discard the buf.
                    }
                }
                else {
                    if (read_trm_count <= 0) {
                       __TIMESTAMP();  fprintf(_TRMPRX_OUT_, "Remote connection is closed...Retry\r\n");
                        close(trm_socket_fd);
                        trm_socket_fd = -1;
                    }
                }

                free(buf);
            }
            else {
                __TIMESTAMP(); fprintf(_TRMPRX_OUT_, "Cannot find connection from id %d\r\n", connection_id);
            }
        }
        else {
            free(buf);
            if (read_trm_count <= 0) {
                __TIMESTAMP(); fprintf(_TRMPRX_OUT_, "Remote connection is closed...Retry\r\n");
                close(trm_socket_fd);
                trm_socket_fd = -1;
            }
        }
    }

    return 0;
}

static  QString getIPFromInterface(QString interfaceName, bool isLoopback)
{
    QString interfaceIp;
    QNetworkInterface netInterface = QNetworkInterface::interfaceFromName(interfaceName);
    if(netInterface.isValid())
    {
        foreach (const QNetworkAddressEntry &address, netInterface.addressEntries())
        {
            if (address.ip().isLoopback() == isLoopback)
            {
                interfaceIp = address.ip().toString();
                break;
            }
        }
    }
    else
    {
      qDebug()<<"TRM Invalid interface";
    }
    return interfaceIp;
}

int main(int argc, char *argv[]) 
{
    /* Start the main loop */
    QCoreApplication app(argc, argv);
    QStringList args = app.arguments();

    int itr = 1;
    char *debugConfigFile = NULL;

    QRegExp optionDebugFile("--debugconfig");
    QRegExp optionBound("-bound");
    QStringList boundIPs;

    for (int i = 1; i < args.size(); ++i) {
        if (optionBound.indexIn(args.at(i)) != -1 ) {   
            boundIPs << args.at(i+1);
            ++i;
            qDebug() << "bound to " << boundIPs;
        } 
        else if (optionDebugFile.indexIn(args.at(i)) != -1 ) {   
            debugConfigFile = argv[i+1];
            ++i;
            qDebug() << "rdklogger debug file is " << debugConfigFile;
        }
    }

    if (argc < 3) {
		printf("TRM-WS-Proxy <server ip addr> <port> --debugconfi <config file> [optionals]\r\n");
		printf("optionals: -bound <bound ip addr1> <bound ip addr2>...\r\n");
		return 0;
	}

    rdk_logger_init(debugConfigFile);

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TRM", "This is a test line");



#define TRM_WS_PROXY_LISTENING_PORT 9989

    //Initiate utilities
    pthread_mutex_init(&conn_mutex, NULL);
    __TIMESTAMP(); printf("Starting TRM-WS-Proxy\r\n");


    /* First create a persistent connection to TRM */
    trm_ip   = argv[1];
    trm_port = argv[2];

    connect_to_trm(trm_ip, atoi(trm_port), &trm_socket_fd);
    pthread_t trm_rsp_listener_thread;
    pthread_create(&trm_rsp_listener_thread, NULL, TRM_response_listener, (void *)trm_socket_fd);

    if (boundIPs.empty())
    {
        //Adding local host ip to the list
        QString localHostIp = getIPFromInterface(localHost, true);
        if(!localHostIp.isNull())
        {
            boundIPs.append(localHostIp);
        }

        //Adding moca interface ip to the list
        QSettings deviceSetting( devicePropertiesPath, QSettings::IniFormat );
        QString mocaInterfaceName = deviceSetting.value( mocaIntTagName).toString();
        if(!mocaInterfaceName.isNull())
        {
            QString mocaIp = getIPFromInterface(mocaInterfaceName, false);
            if(!mocaIp.isNull())
            {
                boundIPs.append(mocaIp);
            }
        }
        else
        {
            qDebug()<<"TRM Moca interface name not found";
        }
    }

    WebSocketProxy wsproxy(boundIPs, TRM_WS_PROXY_LISTENING_PORT);
    
    int app_return = app.exec();

    //Initiate utilities
    pthread_mutex_destroy(&conn_mutex);

    __TIMESTAMP(); printf("TRM WS-Daemon Exit!!\r\n");
    return app_return;
}


/** @} */
/** @} */
