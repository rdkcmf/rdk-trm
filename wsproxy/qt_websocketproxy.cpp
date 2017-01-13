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


#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/reboot.h>
#include <sys/reboot.h>

#include <iostream>
#include <QStringList>
#include <QtCore/QTimer>
#include <QtWebSockets/QWebSocketServer>
#include <QtWebSockets/QWebSocket>

#include <QtCore/QFile>
#include <QtNetwork/QSslCertificate>
#include <QtNetwork/QSslKey>
#include <QSslCipher>
#include <QSettings>

const char* trmPropertiesPath ="/etc/trmProxySetup.properties";
const char* caKeyTagName = "CA_CHAIN_CERTIFICATE";
const char* privateKeyTagName = "CA_SERVER_PRIVATE_KEY";
const char* publicKeyTagName = "CA_SERVER_PUBLIC_CERTIFICATE";


#include "qt_websocketproxy.h"
extern int  begin_request_callback(void *conn);
extern void end_request_callback(void *conn, int reply_status_code);
extern int  websocket_connect_callback(void *conn);
extern int  websocket_disconnect_callback(void *conn);
extern void websocket_ready_callback(void *conn) ;
extern int  log_message_callback(const void *conn, const char *message) ;
extern int  websocket_data_callback(void *conn, int flags, char *trm_data, size_t trm_data_length) ;

static WebSocketProxy *m_proxy = NULL;

QT_USE_NAMESPACE

PingPongTask::PingPongTask(QWebSocket &wssocket) 
: wssocket(wssocket), stopped(false)
{
    __TIMESTAMP(); std::cout << "Ping-Pong created for socket " << (void *)&wssocket << std::endl; 
    connect(&timer, SIGNAL(timeout()), this, SLOT(onTimeout()));
    connect(&wssocket, SIGNAL(pong(quint64, QByteArray)), this, SLOT(onPong(quint64, QByteArray)));
    retry = 0;
}

PingPongTask::~PingPongTask(void) 
{
    if (!stopped) {
        __TIMESTAMP(); std::cout << " Assert: PING-PONG not stopped before deleting\r\n";
    }
}

void PingPongTask::start(void) 
{
    if (!stopped) {
        //__TIMESTAMP(); std::cout << "Ping-Pong started for socket " << (void *)&wssocket << std::endl; 
        wssocket.ping();
        timer.setInterval(5000);
        timer.setSingleShot(true);
        timer.start();
    }
}

void PingPongTask::stop(void) 
{
    __TIMESTAMP(); std::cout << "Ping-Pong stopped for socket " << (void *)&wssocket << std::endl; 
    timer.stop();
    stopped = true;
}

void PingPongTask::onTimeout(void) 
{
    /* Retry on timeout, or close the connection */
    //__TIMESTAMP(); std::cout << "Ping-Pong Timeout " << retry << "times for socket " << (void *)&wssocket << std::endl; 
    retry++;
    if (retry < 3) {
        start();
    }
    else {
        if (retry < 5) {
            __TIMESTAMP(); std::cout << "Ping-Pong Would have closed socket " << (void *)&wssocket << std::endl; 
            start();
        }
        else {
            __TIMESTAMP(); std::cout << "Ping-Pong Timeout closing socket " << (void *)&wssocket << std::endl; 
            wssocket.close();
        }
    }
}

void PingPongTask::onPong(quint64 elapsedTime, QByteArray)
{
    /* reset on PONG */
    if (elapsedTime > 10000) {
        __TIMESTAMP(); std::cout << "Ping-Pong Slow: pong received for socket " << (void *)&wssocket << std::endl; 
        std::cout << " At [" << QTime::currentTime().toString().toUtf8().data();
        std::cout << " ] PONG received epapsedTime = " << elapsedTime << std::endl;
    }
    retry = 0;
}


/* Function to execute system command */
static int exec_sys_command(char* cmd)
{
  char buff[128];
  int len;
  FILE *syscmd = popen(cmd, "re");
  if(!syscmd){
	  std::cout <<"popen failed with error code"<< syscmd <<"to execute system command: "<< cmd;
    return -1;
  }
  memset(buff, 0, 128);
  std::cout << "Executing system command " << cmd << std::endl;

  while(fgets(buff, sizeof(buff), syscmd) != 0){
	  std::cout << "read syscmd buff : " << buff << std::endl;
  }
  pclose(syscmd);
  return 0;
}


WebSocketProxy::WebSocketProxy(const QStringList &boundIPs, quint16 port, QObject *parent) :
    QObject(parent), proxyServers(), connections()
{
#ifdef TRM_USE_SSL
    //Reading TRM configuration file
    QSettings trmSetting( trmPropertiesPath, QSettings::IniFormat );
    QString caChainFile = trmSetting.value( caKeyTagName ).toString();
    QString keyFileName = trmSetting.value( privateKeyTagName  ).toString();
    QString certFileName = trmSetting.value( publicKeyTagName  ).toString();
    if(caChainFile.isNull() || keyFileName.isNull() || certFileName.isNull()  )
    {
		std::cout << "Missing TRM configuration information";
    }
    else
    {
        QStringList::const_iterator it = boundIPs.constBegin();
	    while (it != boundIPs.constEnd())
	    {
		    if (proxyServers.constFind(*it) == proxyServers.constEnd())
		    {
			    QFile certFile(certFileName);
			    QFile keyFile(keyFileName);
			    if(!keyFile.exists())
			    {
			        std::cout << "Server private key not exist. Don't start the server ";
			        break;
			    }
			    certFile.open(QIODevice::ReadOnly);
			    keyFile.open(QIODevice::ReadOnly);
			    QSslCertificate certificate(&certFile, QSsl::Pem);
			    QSslKey sslKey(&keyFile, QSsl::Rsa, QSsl::Pem,QSsl::PrivateKey,QByteArray(""));
			    certFile.close();
			    keyFile.close();
			    QSslConfiguration sslConfiguration;

			    sslConfiguration.setPeerVerifyMode(QSslSocket::VerifyPeer);
			    sslConfiguration.setLocalCertificate(certificate);
			    sslConfiguration.setPrivateKey(sslKey);
			    sslConfiguration.setProtocol(QSsl::TlsV1SslV3);
			    sslConfiguration.setPeerVerifyDepth(2);

			    QList<QSslCertificate> caCerts = QSslCertificate::fromPath(caChainFile);

			    sslConfiguration.setCaCertificates(caCerts);
			    QWebSocketServer *proxyServer = new QWebSocketServer(QString("TRM SecureMode WebsocketServer IP: ") + *it , QWebSocketServer::SecureMode, this);

			    proxyServer->setSslConfiguration(sslConfiguration);

			    std::cout << "TRM WebsocketProxy " <<(*it).toUtf8().data() <<std::endl;
			    if (proxyServer->listen(QHostAddress(*it), port))
			    {
				    connect(proxyServer, SIGNAL(newConnection()), this,
				        SLOT(onNewConnection()));
				    connect(proxyServer, SIGNAL(onSslErrors()), this,
						SLOT(onSslErrors()));
				    proxyServers[*it] = proxyServer;
				    if (proxyServer->secureMode() == QWebSocketServer::SecureMode)
				    {
					    __TIMESTAMP();std::cout << "TRM WebsocketProxy sslMode SecureMode"
							<< std::endl;
				    }
				    else
				    {
					    __TIMESTAMP();std::cout << "TRM WebsocketProxy sslMode NonSecureMode"
							<< std::endl;
			        }
			    }
		    }
		    else
		    {
			__TIMESTAMP();std::cout << "TRM WebsocketProxy already listen on "
			        << (*it).toUtf8().data() << ":" << port << std::endl;
		    }

		    ++it;
	    }
	    char cmd[256];
	    sprintf(cmd, "%s %s", "rm", keyFileName.toLatin1().data());
	    exec_sys_command(cmd);
    }
#endif
	///non secure connection on port 9988
	port--;

	// Temp Code added to support non secure TRM connection. This should be removed once all XI device updated
	// with secure connection capability.
	QStringList::const_iterator itr = boundIPs.constBegin();
	while (itr != boundIPs.constEnd()) {
			QWebSocketServer *proxyServer = new QWebSocketServer(
					QString("TRM NonSecureMode WebsocketServer IP: ") + *itr,
					QWebSocketServer::NonSecureMode, this);
			std::cout << "TRM WebsocketProxy " <<(*itr).toUtf8().data() <<std::endl;
			if (proxyServer->listen(QHostAddress(*itr), port)) {
				connect(proxyServer, SIGNAL(newConnection()), this,
						SLOT(onNewConnection()));
				connect(proxyServer, SIGNAL(onSslErrors()), this,
						SLOT(onSslErrors()));
				proxyServers[*itr] = proxyServer;
				if (proxyServer->secureMode() == QWebSocketServer::SecureMode) {
					__TIMESTAMP()
					;
					std::cout << "TRM WebsocketProxy sslMode SecureMode"
							<< std::endl;
				} else {
					__TIMESTAMP()
					;
					std::cout << "TRM WebsocketProxy sslMode NonSecureMode"
							<< std::endl;
				}

			}

		++itr;
	}

	m_proxy = this;
}

#ifdef TRM_USE_SSL
void WebSocketProxy::onSslErrors(const QList<QSslError> &sslError)
{
    std::cout<<" onSslErrors occured";
}
#endif

void WebSocketProxy::onNewConnection()
{

    {

    	static const char *has_livestream_client_flag_filename ="/tmp/mnt/diska3/persistent/.has_livestream_client";
        struct stat st;

        int ret = ::lstat(has_livestream_client_flag_filename, &st);
#define USE_DELIA_GATEWAY
#ifndef USE_DELIA_GATEWAY
        if (ret >= 0) {
        	/* Already has flag set */
        }
        else {
        	int fd = ::open(has_livestream_client_flag_filename, O_WRONLY|O_CREAT, 0666);
        	if (fd >= 0) {
        		/* Reboot */
        		__TIMESTAMP(); std::cout << "Rebooting STB on the initial xi3 connection \r\n" << std::endl;
        		close(fd);
        		::sync();
        		::reboot(LINUX_REBOOT_CMD_RESTART);
        		return;
        	}
        }
#endif //USE_DELIA_GATEWAY

    }

    QWebSocketServer *proxyServer = qobject_cast<QWebSocketServer *>(sender());
    __TIMESTAMP(); std::cout << "TRM WebsocketProxy connection from server " << (void *)proxyServer << "of name:" << proxyServer->serverName().toUtf8().data() << std::endl;

    QWebSocket *wssocket = proxyServer->nextPendingConnection();
    websocket_connect_callback((void *)wssocket);

    __TIMESTAMP(); std::cout << "TRM WebsocketProxy accept connection " << (void *)wssocket << std::endl;

    // The QtWebSocket version we use doesn't support connected() signal. Instead the
    // newConnection() signal already indicates the completion of ws handshake
    websocket_ready_callback((void *)wssocket);
    connections << wssocket;
    pingPongTasks.insert(wssocket, new PingPongTask(*wssocket));

    /* Connect() signaled when socket is connected and the handshake was successful */
    connect(wssocket, SIGNAL(connected()), this, SLOT(onWebsocketConnect()));
    /* binaryMessageReceived() when there is raw payload on websocket */
    connect(wssocket, SIGNAL(binaryMessageReceived(QByteArray)), this, SLOT(onWebsocketBinaryMessageReceived(QByteArray)));
    connect(wssocket, SIGNAL(textMessageReceived(QString)), this, SLOT(onWebsocketTextMessageReceived(QString)));
    /* write callback */
    /*5.x*///connect(wssocket, SIGNAL(bytesWritten(qint64)), this, SLOT(onWebsocketBytesWritten(qint64)));
    /* disconnected() when the socket is disconnected. */
    connect(wssocket, SIGNAL(disconnected()), this, SLOT(onWebsocketDisconnected()));
    connect(wssocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onWebsocketError(QAbstractSocket::SocketError)));

#if 1
    connect(wssocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(onWebsocketStateChanged(QAbstractSocket::SocketState)));
#else
    connect(wssocket, SIGNAL(aboutToClose()), this, SLOT(onWebSocketAboutClose()));
#endif
    ((pingPongTasks.find(wssocket)).value())->start();

}

void WebSocketProxy::onWebsocketConnect(void)
{
    __TIMESTAMP(); std::cout << "TRM WebsocketProxy onWebsocketConnect" << std::endl;
}

void WebSocketProxy::onWebsocketBinaryMessageReceived(QByteArray byteArray)
{
    __TIMESTAMP(); std::cout << "TRM WebsocketProxy onWebsocketBinaryMessageReceived" << std::endl;
    QWebSocket *wssocket = qobject_cast<QWebSocket *>(sender());
    websocket_data_callback((void *)wssocket, 0, byteArray.data(), byteArray.size());
}

void WebSocketProxy::onWebsocketTextMessageReceived(QString message)
{
    __TIMESTAMP(); std::cout << "TRM WebsocketProxy onWebsocketTextMessageReceived" << std::endl;
    QWebSocket *wssocket = qobject_cast<QWebSocket *>(sender());
    websocket_data_callback((void *)wssocket, 0, message.toUtf8().data(), message.size());
}

void WebSocketProxy::onWebsocketBytesWritten(qint64)
{
}

void WebSocketProxy::onWebsocketDisconnected(void)
{
    QWebSocket *wssocket = qobject_cast<QWebSocket *>(sender());
    __TIMESTAMP(); std::cout << "TRM WebsocketProxy onWebsocketDisconnected " << (void *)wssocket << std::endl;
    if (wssocket && connections.contains(wssocket)) {
        //wssocket->close();
    	wssocket->deleteLater();

        connections.removeAll(wssocket);
    	websocket_disconnect_callback((void *)wssocket);
        /* Delete pingpong before deleting socket */
        ((pingPongTasks.find(wssocket)).value())->stop();
        ((pingPongTasks.find(wssocket)).value())->deleteLater();
    	pingPongTasks.remove(wssocket);
    }
}

void WebSocketProxy::onWebsocketStateChanged(QAbstractSocket::SocketState state)
{
    static const char *states_name[] = {
        "UnconnectedState",
        "HostLookupState",
        "ConnectingState",
        "ConnectedState",
        "BoundState",
        "ListeningState", 
        "ClosingState",
    };
    QWebSocket *wssocket = qobject_cast<QWebSocket *>(sender());
    __TIMESTAMP(); std::cout << "TRM WebsocketProxy onWebsocketStateChanged " << (void *)wssocket << " New State =" << states_name[state] << std::endl;
}

void WebSocketProxy::onWebSocketAboutClose(void)
{
}

void WebSocketProxy::onWebsocketError(QAbstractSocket::SocketError error)
{
    __TIMESTAMP(); std::cout << "TRM WebsocketProxy onWebsocketError = " << error << std::endl;
    QWebSocket *wssocket = qobject_cast<QWebSocket *>(sender());
    if (wssocket) {
        // cannot do socket close here. do so after disconnect 
    	// wssocket->close();
    }
}

void WebSocketProxy::onWebsocketHasDataToWrite(void *conn, void *data)
{
    /* This is thread safe as the caller of mg_websocket_write() already ensure
     * that the connection is still valid.
     */

    QWebSocket *wssocket = (QWebSocket *)conn;
    QByteArray *byteArray= (QByteArray *)data;
    if (connections.contains(wssocket)) { 
        wssocket->sendTextMessage(QString(byteArray->constData()));
    }
    delete byteArray;

}

void WebSocketProxy::onRemoveConnection(void *conn)
{
    /* This is thread safe as the caller of mg_websocket_write() already ensure
     * that the connection is still valid.
     */
    __TIMESTAMP(); std::cout << "onRemoveConnection" << std::endl;

    QWebSocket *wssocket = (QWebSocket *)conn;
    if (connections.contains(wssocket)) {
        emit wssocket->close();
    }
}


int mg_websocket_write(void * conn, const char *data, size_t data_len)
{
    QWebSocket *wssocket = (QWebSocket *)(conn);
    //Want to NULL terminate the message 
    QByteArray *byteArray = new QByteArray(data, data_len);
    byteArray->append('\0');
    QMetaObject::invokeMethod(m_proxy, "onWebsocketHasDataToWrite", Qt::QueuedConnection, Q_ARG(void *, wssocket), Q_ARG(void *, byteArray)); 

    return data_len;
}

int mg_websocket_close(void * conn)
{
    __TIMESTAMP(); std::cout << "mg_websocket_close" << std::endl;
    __TIMESTAMP(); printf("[%s] THREAD SELF is %p\r\n", __FUNCTION__, (void *)pthread_self());

    QWebSocket *wssocket = (QWebSocket *)(conn);
    //Want to NULL terminate the message
    QMetaObject::invokeMethod(m_proxy, "onRemoveConnection", Qt::QueuedConnection, Q_ARG(void *, wssocket));

    return 0;
}


/** @} */
/** @} */
