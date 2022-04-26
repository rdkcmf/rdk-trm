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
#include "safec_lib.h"
#include "rfcapi.h"

#define DYNAMIC_PASSCODE_UTILITY "/usr/bin/rdkssacli"
#define STATIC_PASSCODE_UTILITY "/usr/bin/GetConfigFile"
#define DYNAMIC_PASSCODE_ARG   "\"{STOR=GET,SRC=kquhqtoczcbx,DST=/dev/stdout}\""
#define STATIC_PASSCODE_ARG_FILE "/tmp/.cfgStaticxpki"


const char* trmPropertiesPath ="/etc/trmProxySetup.properties";
const char* caKeyTagName = "CA_CHAIN_CERTIFICATE";
const char* privateKeyTagName = "CA_SERVER_PRIVATE_KEY";
const char* publicKeyTagName = "CA_SERVER_PUBLIC_CERTIFICATE";
const char* xpkiDynamicCertificate = "CA_SERVER_XPKI_DYNAMIC_CERTIFICATE";
const char* xpkiStaticCertificate = "CA_SERVER_XPKI_STATIC_CERTIFICATE";


#include "qt_websocketproxy.h"

#include "tcpOpensslProxyServer.h"
extern int  begin_request_callback(void *conn);
extern void end_request_callback(void *conn, int reply_status_code);
extern int  websocket_connect_callback(void *conn);
extern int  websocket_disconnect_callback(void *conn);
extern void websocket_ready_callback(void *conn) ;
extern int  log_message_callback(const void *conn, const char *message) ;
extern int  websocket_data_callback(void *conn, int flags, char *trm_data, size_t trm_data_length) ;
extern int  websocket_data_callback_with_clientId(int connection_id, int flags, char *trm_data, size_t trm_data_length);
static void onWebsocketMessageReceivedSSL(int clientId, char* message, size_t len);
static void onPongSSL(int clientId, long long unsigned int ticks, QByteArray &qmsg);

static WebSocketProxy *m_proxy = NULL;

QT_USE_NAMESPACE

PingPongTask::PingPongTask(QWebSocket* wssocket)
    : wssocket(wssocket), stopped(false)
{
    m_clientId = -1;
    m_sslProxyServer = NULL;
    __TIMESTAMP();
    std::cout << "Ping-Pong created for socket " << (void *)wssocket << std::endl;
    connect(&timer, SIGNAL(timeout()), this, SLOT(onTimeout()));
    connect(wssocket, SIGNAL(pong(quint64, QByteArray)), this, SLOT(onPong(quint64, QByteArray)));
    retry = 0;
}

PingPongTask::PingPongTask(tcpOpensslProxyServer *sslProxyServer, int clientId)
    : m_sslProxyServer(sslProxyServer), m_clientId(clientId), stopped(false)
{
    wssocket = NULL;
    __TIMESTAMP();
    std::cout << "Ping-Pong created for socket " << (void *)wssocket << " m_sslProxyServer "<< m_sslProxyServer <<std::endl;
    connect(&timer, SIGNAL(timeout()), this, SLOT(onTimeout()));
    //On pong signal handle is implemented during new connection
    retry = 0;
}

PingPongTask::~PingPongTask(void)
{
    if (!stopped) {
        __TIMESTAMP();
        std::cout << " Assert: PING-PONG not stopped before deleting\r\n";
    }
}

void PingPongTask::start(void)
{
    if (!stopped) {
        //__TIMESTAMP(); std::cout << "Ping-Pong started for socket " << (void *)&wssocket << " m_sslProxyServer "<< m_sslProxyServer << " m_clientId " << m_clientId <<std::endl;
        if (NULL != wssocket) {
            wssocket->ping();
        }
        if (NULL != m_sslProxyServer) {
            m_sslProxyServer->ping (m_clientId);
        }
        timer.setInterval(5000);
        timer.setSingleShot(true);
        timer.start();
    }
}

void PingPongTask::stop(void)
{
    __TIMESTAMP();
    std::cout << "Ping-Pong stopped for socket " << (void *)wssocket << " m_sslProxyServer "<< m_sslProxyServer <<std::endl;
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
            __TIMESTAMP();
            std::cout << "Ping-Pong Would have closed socket " << (void *)wssocket << " m_sslProxyServer "<< m_sslProxyServer <<std::endl;
            start();
        }
        else {
            __TIMESTAMP();
            std::cout << "Ping-Pong Timeout closing socket " << (void *)wssocket << " m_sslProxyServer "<< m_sslProxyServer << " m_clientId " << m_clientId << std::endl;
	    if (NULL != wssocket) {
                wssocket->close();
	    }
            if (NULL != m_sslProxyServer) {
                m_sslProxyServer->closeClient (m_clientId);
            }
        }
    }
}

void PingPongTask::onPong(quint64 elapsedTime, QByteArray)
{
    /* reset on PONG */
    if (elapsedTime > 10000) {
        __TIMESTAMP();
        std::cout << "Ping-Pong Slow: pong received for socket " << (void *)wssocket << " m_sslProxyServer "<< m_sslProxyServer <<std::endl;
        std::cout << " At [" << QTime::currentTime().toString().toUtf8().data();
        std::cout << " ] PONG received epapsedTime = " << elapsedTime << std::endl;
    }
    retry = 0;
}


/* Function to execute system command */
static int exec_sys_command(char* cmd)
{
    char buff[128];
    FILE *syscmd = popen(cmd, "re");
    if(!syscmd) {
        std::cout <<"popen failed with error code"<< syscmd <<"to execute system command: "<< cmd;
        return -1;
    }
    memset(buff, 0, 128);
    std::cout << "Executing system command " << cmd << std::endl;

    while(fgets(buff, sizeof(buff), syscmd) != 0) {
        std::cout << "read syscmd buff : " << buff << std::endl;
    }
    pclose(syscmd);
    return 0;
}

#define  TRM_USE_RFC 1
static bool rfc_get_trmssl_status()
{
    bool isTRMSSLEnabled = true;
#ifdef TRM_USE_RFC
    int sysRet = system(". /lib/rdk/isFeatureEnabled.sh NOTRMSSL");
    if((WEXITSTATUS(sysRet) == true) && (WIFEXITED(sysRet) == true))
    {
        std::cout << "RFC NOTRMSSL feature Enabled "<<std::endl;
        isTRMSSLEnabled = false;
    }
#else
    isTRMSSLEnabled = true;
#endif

    std::cout << "RFC TRMSSL feature status:"<< isTRMSSLEnabled<<std::endl;
    return isTRMSSLEnabled;
}

typedef enum {
    RFC_PARM_mTLS = 0,
    RFC_PARM_qtPort,
}TRM_RFC_PARAM_TYPE;

static bool is_TRM_RFC_param_enable (TRM_RFC_PARAM_TYPE type)
{
    bool ret = false;
    RFC_ParamData_t param;
    WDMP_STATUS status = WDMP_ERR_INVALID_PARAMETER_NAME;
    std::string parmName = std::string ("invalid");
    switch (type) {
        case RFC_PARM_mTLS:
            status = getRFCParameter((const char*)"WSPROXY", (const char*)"Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.TRM.EnableMTLS", &param);
            parmName = std::string ("mTLS");
            break;
        case RFC_PARM_qtPort:
            status = getRFCParameter((const char*)"WSPROXY", (const char*)"Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.TRMQtSecurePort.Enable", &param);
            parmName = std::string ("qtPort");
            break;
    }
    if (status == WDMP_SUCCESS) {
        printf ("name = %s, type = %d, value = %s\n", param.name, param.type, param.value);

        if (!strncmp(param.value, "true", strlen("true"))) {
            printf ("TRM %s is enabled.\n", parmName.c_str());
            ret = true;
        }
        else {
            printf ("TRM %s is disabled.\n", parmName.c_str());
        }
    }
    else {
        printf ("getRFCParameter Failed : %s\n", getRFCErrorString(status));
	if (RFC_PARM_qtPort == type){
            //TRMQtSecurePort default value is enabled.
            ret = true;
	}
    }
    return ret;
}

static void delete_file (QString &fileName)
{
    char cmd[256];
    sprintf(cmd, "%s %s", "rm", fileName.toLatin1().data());
    exec_sys_command(cmd);
}

static void executeCommand (std::string command, std::string &output){
    FILE *fp;
    int i = 0;
    int maxResult = 1024;
    char result[maxResult] = {'\0'};
    fp = popen(command.c_str(), "r");
    if (fp == NULL) {
        printf("\n%s:%d Failed to run command: %s\n", __FUNCTION__, __LINE__, command.c_str());
        return;
    }

    i = 0;
    /* Read the output a line at a time - output it. */
    while (fgets(result, sizeof(result), fp) != NULL) {
        if (i++ > maxResult) {
            break;
        }
    }
    if (maxResult <=i){
        printf("\n%s:%d command %s . running in endless loop\n", __FUNCTION__, __LINE__, command.c_str());
    }

    output = result;
    /* close */
    pclose(fp);
    return;

}

static bool selectPk12KeyAndPass (std::string dynKeyFilePath, std::string staticKeyFilePath,
		std::string &outputKeyFilePath, std::string &pass)
{
    bool eRet = true;
    if (!(access(dynKeyFilePath.c_str(), F_OK) == 0)) {
        //dynamic pk12 certificate not exists
        printf("\n%s:%d dynamic pk12 certificate:%s not exists. Pass key wont be generated\n", __FUNCTION__, __LINE__, dynKeyFilePath.c_str());
        eRet = false;
    } else {
        if (!(access(DYNAMIC_PASSCODE_UTILITY, F_OK) == 0)) {
            //dynamic pk12 pass key generator exists
            printf("\n%s:%d dynamic pk12 pass key generator not exists\n", __FUNCTION__, __LINE__);
            eRet = false;
        }

        pass="";
        if (true == eRet) {
            std::string command = DYNAMIC_PASSCODE_UTILITY;
            command.append (" ");
            command.append (DYNAMIC_PASSCODE_ARG);
            executeCommand (command, pass);
	    //Remove new line if present in the password
	    pass.erase(std::remove(pass.begin(), pass.end(), '\n'), pass.end());
        }

        outputKeyFilePath = "";
        if (!pass.empty()) {
            outputKeyFilePath = dynKeyFilePath;
	    printf("\n%s:%d using dynamic pk12 certificate\n", __FUNCTION__, __LINE__);
            eRet = true;
            return eRet;
        } else {
            printf("\n%s:%d dynamic pk12 pass key null\n", __FUNCTION__, __LINE__);
        }
    }
    printf("\n%s:%d dynamic pk12 certificate check failed\n", __FUNCTION__, __LINE__);

    outputKeyFilePath = ""; pass="";
    //Not able to setup dynamic key set static key
    if (!(access(staticKeyFilePath.c_str(), F_OK) == 0)) {
        //static pk12 certificate not exists
        printf("\n%s:%d static pk12 certificate:%s not exists\n", __FUNCTION__, __LINE__, staticKeyFilePath.c_str());
        eRet = false;
        return eRet;
    }
    if (!(access(STATIC_PASSCODE_UTILITY, F_OK) == 0)) {
        //static pk12 pass key generator exists
        printf("\n%s:%d static pk12 pass key generator not exists\n", __FUNCTION__, __LINE__);
        eRet = false;
        return eRet;
    }

    pass="";
    std::string command = STATIC_PASSCODE_UTILITY;
    command.append (" ");
    command.append (STATIC_PASSCODE_ARG_FILE);
    executeCommand (command, pass);
    if (!(access(STATIC_PASSCODE_ARG_FILE, F_OK) == 0)) {
        printf("\n%s:%d static config file generation failed\n", __FUNCTION__, __LINE__);
        eRet = false;
        return eRet;
    }

    pass="";
    command = "cat ";
    command.append (STATIC_PASSCODE_ARG_FILE);
    executeCommand (command, pass);
    //Remove new line if present in the password
    pass.erase(std::remove(pass.begin(), pass.end(), '\n'), pass.end());

    outputKeyFilePath = "";
    if (!pass.empty()) {
        outputKeyFilePath = staticKeyFilePath;
	printf("\n%s:%d using static pk12 certificate\n", __FUNCTION__, __LINE__);
        eRet = true;
        return eRet;
    } else {
        printf("\n%s:%d static pk12 pass key null\n", __FUNCTION__, __LINE__);
    }

    printf("\n%s:%d dynamic and static pk12 certificate check failed\n", __FUNCTION__, __LINE__);
    //Not expecting to reach here
    return false;
}

WebSocketProxy::WebSocketProxy(const QStringList &boundIPs, quint16 port, QObject *parent) :
    QObject(parent), proxyServers(), connections()
{
#ifdef TRM_USE_SSL
    if(rfc_get_trmssl_status() == true)
    {
        //Reading TRM configuration file
        QSettings trmSetting( trmPropertiesPath, QSettings::IniFormat );
        QString caChainFile = trmSetting.value( caKeyTagName ).toString();
        QString keyFileName = trmSetting.value( privateKeyTagName  ).toString();
        QString certFileName = trmSetting.value( publicKeyTagName  ).toString();
        QString xpkiDynFilePath = trmSetting.value( xpkiDynamicCertificate  ).toString();
        QString xpkiStaticFilePath = trmSetting.value( xpkiStaticCertificate  ).toString();

        if(caChainFile.isNull() || keyFileName.isNull() || certFileName.isNull())

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

                    if(is_TRM_RFC_param_enable(RFC_PARM_qtPort)) {
                        //If xPKI RFC enabled use xPKI certificates.
                        std::cout <<"QT secure port enabled"<<std::endl;

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

                        if (true == is_TRM_RFC_param_enable(RFC_PARM_mTLS))
                        {
                            std::cout << "QSslSocket peerVerifyMode is QSslSocket::VerifyPeer. " << std::endl;
                            sslConfiguration.setPeerVerifyMode(QSslSocket::VerifyPeer);
                        }
                        else
                        {
                            std::cout << "QSslSocket peerVerifyMode is QSslSocket::QueryPeer. " << std::endl;
                            sslConfiguration.setPeerVerifyMode(QSslSocket::QueryPeer);
                        }

                        sslConfiguration.setLocalCertificate(certificate);
                        sslConfiguration.setPrivateKey(sslKey);
                        sslConfiguration.setProtocol(QSsl::TlsV1_2);
                        sslConfiguration.setPeerVerifyDepth(2);

                        QList<QSslCertificate> caCerts = QSslCertificate::fromPath(caChainFile);

                        sslConfiguration.setCaCertificates(caCerts);
                        QWebSocketServer *proxyServer = new QWebSocketServer(QString("TRM SecureMode WebsocketServer IP: ") + *it , QWebSocketServer::SecureMode, this);

                        proxyServer->setSslConfiguration(sslConfiguration);

                        std::cout << "TRM WebsocketProxy starting server on ip: " <<(*it).toUtf8().data()
                              <<" SecureMode: "<<proxyServer->secureMode()<< std::endl;
                        if (proxyServer->listen(QHostAddress(*it), port))
                        {
                            connect(proxyServer, SIGNAL(newConnection()), this,
                                SLOT(onNewConnection()));
                            connect(proxyServer,&QWebSocketServer::sslErrors ,this,
                                &WebSocketProxy::onSslErrors);
                            connect(proxyServer, &QWebSocketServer::acceptError, this,
                                &WebSocketProxy::onAcceptError);
                            connect(proxyServer, &QWebSocketServer::peerVerifyError, this,
                                &WebSocketProxy::onPeerVerifyError);
                            proxyServers[*it] = proxyServer;
                        }
                        else
                        {
                            std::cout << "TRM WebsocketProxy Failed to listen" << std::endl;
                        }
                    }

		    std::string pk12keyFilePath = "";
		    std::string pk12pass = "";
		    bool isPk12CertAvailable = selectPk12KeyAndPass (xpkiDynFilePath.toStdString(), xpkiStaticFilePath.toStdString(), pk12keyFilePath, pk12pass);
		    if (isPk12CertAvailable) {
                        std::cout << "Opening tcpOpensslProxyServer with ip"<<(*it).toStdString().c_str() << std::endl;
                        tcpOpensslProxyServer *sslProxyServer = new tcpOpensslProxyServer();
		        sslProxyServer->onMessageReceivedCallBack = onWebsocketMessageReceivedSSL;
		        sslProxyServer->onPongCallBack = onPongSSL;

                        //Need to specify ip also here. Use port 9990 here using port old secure port 9988
		        //Will have connection issue with old client boxes since QT auth is not compatible.
                        int ret = sslProxyServer->setUpServer(
                                     (*it).toStdString().c_str(), 9990,
                                     caChainFile.toStdString().c_str(),
                                     pk12keyFilePath.c_str(),
                                     pk12pass.c_str());
                        if (0==ret){
                            connect(sslProxyServer, SIGNAL(newConnection (int, tcpOpensslProxyServer*)), this,
                                SLOT(onNewConnectionSSl(int, tcpOpensslProxyServer*)));
                            connect(sslProxyServer,SIGNAL(sslErrors (QString)) ,this,
                                SLOT(onSslErrorsSSL(QString)));
                            connect(sslProxyServer, SIGNAL(acceptError (QString)), this,
                                SLOT(onAcceptErrorSSL(QString)));
                            connect(sslProxyServer, SIGNAL(peerVerifyError (QString)), this,
                                SLOT(onPeerVerifyErrorSSL(QString)));

                            connect(sslProxyServer, SIGNAL(connected (int, QString)), this,
                                SLOT(onWebsocketConnectSSL(int, QString)));
                            connect(sslProxyServer, SIGNAL(disconnected (int)), this,
                                SLOT(onWebsocketDisconnectedSSL(int)));
                            connect(sslProxyServer, SIGNAL(socketError (QString)), this,
                                SLOT(onWebsocketErrorSSL(QString)));
                            sslProxyServers[*it] = sslProxyServer;
                        } else {
                            std::cout << "TRM WebsocketProxy Failed to listen" << std::endl;
                        }
                    } else {
                        std::cout << "TRM WebsocketProxy pk12 certificates depenedncies not available" << std::endl;
		    }
                }
                else
                {
                    __TIMESTAMP();
                    std::cout << "TRM WebsocketProxy already listen on "
                              << (*it).toUtf8().data() << ":" << port << std::endl;
                }

                ++it;
            }
            delete_file (keyFileName);
        }
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
        std::cout << "TRM WebsocketProxy starting server on ip: " <<(*itr).toUtf8().data()
                  <<" SecureMode: "<<proxyServer->secureMode()<< std::endl;
        if (proxyServer->listen(QHostAddress(*itr), port)) {
            connect(proxyServer, SIGNAL(newConnection()), this,
                    SLOT(onNewConnection()));
            proxyServers[*itr] = proxyServer;
        }
        else
        {
            std::cout << "TRM WebsocketProxy Failed to listen" << std::endl;
        }

        ++itr;
    }

    m_proxy = this;
}

#ifdef TRM_USE_SSL
void WebSocketProxy::onSslErrorsSSL(QString err){
    std::cout << "onSslErrorsSSL:" << err.toStdString()<<endl;
}
void WebSocketProxy::onAcceptErrorSSL(QString err) {
    std::cout<<"onAcceptErrorSSL occured:"<<err.toStdString()<<endl;
}
void WebSocketProxy::onPeerVerifyErrorSSL(QString err) {
    std::cout<<"onPeerVerifyErrorSSL :"<<err.toStdString()<<endl;
}

void WebSocketProxy::onWebsocketConnectSSL(int clientId, QString msg)
{
    __TIMESTAMP();
    std::cout << "TRM WebsocketProxy onWebsocketConnectSSL" << std::endl;
    if (sslProxyPingPongTasks.find(clientId) != sslProxyPingPongTasks.end()){
        PingPongTask* pp = sslProxyPingPongTasks [clientId];
        pp->start();
    }
}

static void onWebsocketMessageReceivedSSL(int clientId, char* message, size_t len)
{
    __TIMESTAMP();
    std::cout << "TRM WebsocketProxy onWebsocketMessageReceivedSSL msg:" << message << std::endl;
    websocket_data_callback_with_clientId (clientId, 0, message, len);
}

static void onPongSSL(int clientId, uint64_t ticks, QByteArray &qmsg)
{
    __TIMESTAMP();
    //std::cout << "TRM WebsocketProxy onPongSSL clientId:" << clientId << std::endl;
    if (m_proxy->sslProxyPingPongTasks.find(clientId) != m_proxy->sslProxyPingPongTasks.end()){
        //send to curresponding pingpong object
        m_proxy->sslProxyPingPongTasks[clientId]->onPong (ticks, qmsg);
    }

}

void WebSocketProxy::onWebsocketDisconnectedSSL(int clientId) {
    __TIMESTAMP();
    std::cout << "TRM WebsocketProxy onWebsocketDisconnectedSSL clientId:"<< clientId << std::endl;
    if (sslProxyMap.find(clientId) != sslProxyMap.end()){
        sslProxyMap.erase (clientId);
    }
    if (sslProxyPingPongTasks.find(clientId) != sslProxyPingPongTasks.end()){
        //if existing client mapping present remove it;
        PingPongTask* oldPp = sslProxyPingPongTasks [clientId];
        sslProxyPingPongTasks.erase(clientId);
	oldPp->stop();
	oldPp->deleteLater();
    }
}

void WebSocketProxy::onWebsocketErrorSSL(QString err)
{
    __TIMESTAMP();
    std::cout << "TRM WebsocketProxy onWebsocketErrorSSL = " << err.toStdString()<<endl;
}



void WebSocketProxy::onSslErrors(const QList<QSslError> &sslError)
{
    qDebug() << "onSslErrors :" << sslError;
}

void WebSocketProxy::onAcceptError(QAbstractSocket::SocketError socketError)
{
    std::cout<<" onAcceptError occured:"<<socketError<<std::endl;
}
void WebSocketProxy::onPeerVerifyError(const QSslError &error)
{
    qDebug() << "onPeerVerifyError :" << error;
}
#endif


void WebSocketProxy::onNewConnectionSSl(int clientId, tcpOpensslProxyServer *ss){
    std::cout<<" WebSocketProxy::onNewConnectionSSl:"<<std::endl;
    {

        static const char *has_livestream_client_flag_filename ="/tmp/mnt/diska3/persistent/.has_livestream_client";
        struct stat st;

        int ret = ::lstat(has_livestream_client_flag_filename, &st);
    }
    sslProxyMap [clientId] = ss;
    PingPongTask* ppTask = new PingPongTask(ss, clientId);
    if (sslProxyPingPongTasks.find(clientId) != sslProxyPingPongTasks.end()){
        //if existing client mapping present remove it;
        PingPongTask* oldPP = sslProxyPingPongTasks [clientId];
        sslProxyPingPongTasks.erase(clientId);
	oldPP->stop();
	oldPP->deleteLater();
    }
    sslProxyPingPongTasks [clientId] = ppTask;
}

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
                __TIMESTAMP();
                std::cout << "Rebooting STB on the initial xi3 connection \r\n" << std::endl;
                close(fd);
                ::sync();
                ::system( "sh /rebootNow.sh -s websocketproxyinit" );
                return;
            }
        }
#endif //USE_DELIA_GATEWAY

    }

    QWebSocketServer *proxyServer = qobject_cast<QWebSocketServer *>(sender());
    __TIMESTAMP();
    std::cout << "TRM WebsocketProxy connection from server " << (void *)proxyServer << " of name:" << proxyServer->serverName().toUtf8().data() << std::endl;

    QWebSocket *wssocket = proxyServer->nextPendingConnection();
    websocket_connect_callback((void *)wssocket);
    __TIMESTAMP();
    std::cout << "TRM WebsocketProxy peerAddress: "  << qPrintable(wssocket->peerAddress().toString()) << " Port: "<< wssocket->peerPort();
    __TIMESTAMP();
    std::cout << "TRM WebsocketProxy accept connection " << (void *)wssocket << std::endl;

    // The QtWebSocket version we use doesn't support connected() signal. Instead the
    // newConnection() signal already indicates the completion of ws handshake
    websocket_ready_callback((void *)wssocket);
    connections << wssocket;
    pingPongTasks.insert(wssocket, new PingPongTask(wssocket));

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
    __TIMESTAMP();
    std::cout << "TRM WebsocketProxy onWebsocketConnect" << std::endl;
}

void WebSocketProxy::onWebsocketBinaryMessageReceived(QByteArray byteArray)
{
    __TIMESTAMP();
    std::cout << "TRM WebsocketProxy onWebsocketBinaryMessageReceived" << std::endl;
    QWebSocket *wssocket = qobject_cast<QWebSocket *>(sender());
    websocket_data_callback((void *)wssocket, 0, byteArray.data(), byteArray.size());
}

void WebSocketProxy::onWebsocketTextMessageReceived(QString message)
{
    __TIMESTAMP();
    std::cout << "TRM WebsocketProxy onWebsocketTextMessageReceived" << std::endl;
    QWebSocket *wssocket = qobject_cast<QWebSocket *>(sender());
    websocket_data_callback((void *)wssocket, 0, message.toUtf8().data(), message.size());
}

void WebSocketProxy::onWebsocketBytesWritten(qint64)
{
}

void WebSocketProxy::onWebsocketDisconnected(void)
{
    QWebSocket *wssocket = qobject_cast<QWebSocket *>(sender());
    __TIMESTAMP();
    std::cout << "TRM WebsocketProxy onWebsocketDisconnected " << (void *)wssocket << std::endl;
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
    __TIMESTAMP();
    std::cout << "TRM WebsocketProxy onWebsocketStateChanged " << (void *)wssocket << " New State =" << states_name[state] << std::endl;
}

void WebSocketProxy::onWebSocketAboutClose(void)
{
}

void WebSocketProxy::onWebsocketError(QAbstractSocket::SocketError error)
{
    __TIMESTAMP();
    std::cout << "TRM WebsocketProxy onWebsocketError = " << error << std::endl;
    QWebSocket *wssocket = qobject_cast<QWebSocket *>(sender());
    if (wssocket) {
        // cannot do socket close here. do so after disconnect
        // wssocket->close();
    }
}

int WebSocketProxy::onWebsocketHasDataToWriteSSL(int clientId, char* data, int len)
{
    if (sslProxyMap.find(clientId) != sslProxyMap.end()){
        tcpOpensslProxyServer *ss = sslProxyMap[clientId];
        printf ("WebSocketProxy::%s:%d data: masked len:%d\n", __FUNCTION__, __LINE__, len);
	ss->sendTextMessage (clientId, data, len);
	return len;
    } else {
        return 0;
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
    __TIMESTAMP();
    std::cout << "onRemoveConnection" << std::endl;

    QWebSocket *wssocket = (QWebSocket *)conn;
    if (connections.contains(wssocket)) {
        emit wssocket->close();
    }
}

void WebSocketProxy::onRemoveAllOpenSslConnections() {
         QMap<QString, tcpOpensslProxyServer *>::iterator iter;
         for (iter = sslProxyServers.begin(); iter != sslProxyServers.end(); ++iter) {
             tcpOpensslProxyServer * sslProxyServer = iter.value();
	     sslProxyServer->closeAllClients();
         }
}

int mg_websocket_write_ssl(int clientId, char *data, int data_len)
{
    int ret = 0;
    printf ("%s:%d data masked data_len:%d\n", __FUNCTION__, __LINE__, data_len);
    if (NULL != m_proxy) {
        ret = m_proxy->onWebsocketHasDataToWriteSSL (clientId, data, data_len);
    }
    return ret;
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

int mg_websocket_close_all_ssl()
{
    printf ("%s:%d \n", __FUNCTION__, __LINE__);
    if (NULL != m_proxy) {
        m_proxy->onRemoveAllOpenSslConnections();
    }
    return 0;
}

int mg_websocket_close(void * conn)
{
    __TIMESTAMP();
    std::cout << "mg_websocket_close" << std::endl;
    __TIMESTAMP();
    printf("[%s] THREAD SELF is %p\r\n", __FUNCTION__, (void *)pthread_self());

    QWebSocket *wssocket = (QWebSocket *)(conn);
    //Want to NULL terminate the message
    QMetaObject::invokeMethod(m_proxy, "onRemoveConnection", Qt::QueuedConnection, Q_ARG(void *, wssocket));

    return 0;
}


/** @} */
/** @} */
