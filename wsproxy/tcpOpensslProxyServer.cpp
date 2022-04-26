#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <netinet/in.h>
#include <cerrno>
#include <cstring>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>

#include <poll.h>
#include <vector>
#include <unordered_map>

#include <openssl/rand.h>
#include <openssl/x509v3.h>
#include <openssl/pkcs12.h>
#include <openssl/err.h>


#include "tcpOpensslProxyServer.h"
#include <QThread>
#include <thread>

/* Buffer size to be used for transfers */
#define BUFSIZE 8192

typedef struct _connectionContext {
    SSL *ssl;
    tcpOpensslProxyServer* instance;
} connectionContext;


static void *connection_handler(void *socket_desc);
static void *clientRead_handler(void *socket_desc);

tcpOpensslProxyServer::tcpOpensslProxyServer()
{
    m_isServerprocessingEnabled = true;
    m_serverSSlCtx = NULL;
    m_serverSocketListenfd = 0;
    m_instanceServer = this;
    onMessageReceivedCallBack = NULL;
}

tcpOpensslProxyServer::~tcpOpensslProxyServer()
{
    m_isServerprocessingEnabled = false;
    m_serverSSlCtx = NULL;
    m_serverSocketListenfd = 0;
    m_instanceServer = NULL;
    onMessageReceivedCallBack = NULL;
}

void tcpOpensslProxyServer::freeSSLContext(SSL_CTX *ctx)
{
    SSL_CTX_free(ctx);
}

void tcpOpensslProxyServer::closeClient(int clientId) {

    printf ("tcpOpensslProxyServer::%s %d entering client %d\n", __FUNCTION__, __LINE__, clientId);
    if (m_clientSSLCtxList.find(clientId) != m_clientSSLCtxList.end()){
        /* Cleanup the SSL handle  will occure while thread is exiting*/
        //Clinet context is removed to diable anymore write
        m_clientSSLCtxList.erase (clientId);
    }
    if (m_clientSSLIsReadEnabledList.find(clientId) != m_clientSSLIsReadEnabledList.end()){
        printf ("tcpOpensslProxyServer::%s %d Closing client %d\n", __FUNCTION__, __LINE__, clientId);
        bool* isReadEnable = m_clientSSLIsReadEnabledList[clientId];
        *isReadEnable = false;
        m_clientSSLIsReadEnabledList.erase (clientId);
    }
}


void tcpOpensslProxyServer::closeAllClients() {
    //close all client connections
    for (auto& it: m_clientSSLCtxList) {
        closeClient (it.first);
    }
}

void tcpOpensslProxyServer::stopServer() {
    m_isServerprocessingEnabled = false;

    if (m_serverSocketListenfd){
        ::close (m_serverSocketListenfd);
        m_serverSocketListenfd = 0;
    }
    if (NULL != m_serverSSlCtx){
        freeSSLContext (m_serverSSlCtx);
        m_serverSSlCtx = NULL;
    }

    closeAllClients ();

    printf ("\ntcpOpensslProxyServer::%s %d exited\n", __FUNCTION__, __LINE__);
}

int tcpOpensslProxyServer::setUpServer(const char* ipAddress, unsigned short port_num, const char *ca_pem,
                                           const char *cert_pk12, const char *pass)
{
    SSL_CTX *ctx;
    int listen_fd;

    /* Parse the port number, and then validate it's range */
    if (port_num < 1 || port_num > 65535) {
        fprintf(stderr, "tcpOpensslProxyServer::%s %d Invalid port number: %d\n", __FUNCTION__, __LINE__, port_num);
        onSocketError (QString ("Invalid port number"));
        return -1;
    }

    /* Initialize OpenSSL */
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();

    /* Get a server context for our use */
    if (!(ctx = get_server_context_pk12(ca_pem, cert_pk12, pass))) {
        return -1;
    }

    /* Get a socket which is ready to listen on the server's port number */
    if ((listen_fd = get_socket(ipAddress, port_num)) < 0) {
        freeSSLContext(ctx);
        return 0;
    }

    m_serverSSlCtx = ctx;
    m_serverSocketListenfd = listen_fd;

    std::thread t1(connection_handler, m_instanceServer);
    t1.detach();

    return 0;

}

SSL_CTX * tcpOpensslProxyServer::get_server_context_pk12(const char *ca_pem,
                                                   const char *cert_pk12,
                                                   const char *pass) {

    SSL_CTX *ctx;
    FILE *f;
    PKCS12 *p12;
    EVP_PKEY *pri;
    X509 *x509;

    /* Get a default context */
    if (!(ctx = SSL_CTX_new(SSLv23_server_method()))) {
        fprintf(stderr, "tcpOpensslProxyServer::%s %d SSL_CTX_new failed\n", __FUNCTION__, __LINE__);
        onSslErrors (QString("SSL_CTX_new failed"));
        return NULL;
    }

    /* Set the CA file location for the server */
    if (SSL_CTX_load_verify_locations(ctx, ca_pem, NULL) != 1) {
        fprintf(stderr, "tcpOpensslProxyServer::%s %d Could not set the CA file location\n", __FUNCTION__, __LINE__);
        onSslErrors (QString("Could not set the CA file location"));
        freeSSLContext (ctx);
        return NULL;
    }

    /* Load the client's CA file location as well */
    SSL_CTX_set_client_CA_list(ctx, SSL_load_client_CA_file(ca_pem));

    f = fopen(cert_pk12,"rb");
    if (!f) {
        fprintf(stderr, "tcpOpensslProxyServer::could not open PKCS12 file '%s'", cert_pk12);
        return 0;
    }
    p12 = d2i_PKCS12_fp(f, NULL);
    fclose(f);

    PKCS12_PBE_add();

    if (!PKCS12_parse(p12, pass, &pri, &x509, NULL)) {
        fprintf(stderr,
              "tcpOpensslProxyServer::could not parse PKCS12 file, check password, OpenSSL error %s",
              ERR_error_string(ERR_get_error(), NULL));
        return 0;
    }

    PKCS12_free(p12);

    if(SSL_CTX_use_certificate(ctx, x509) != 1) {
        //fprintf(stderr, SSL_CLIENT_CERT_ERR);
        fprintf(stderr, "tcpOpensslProxyServer::pk12 certificate error");
        EVP_PKEY_free(pri);
        X509_free(x509);
        return 0;
    }

    if(SSL_CTX_use_PrivateKey(ctx, pri) != 1) {
        fprintf(stderr, "tcpOpensslProxyServer::unable to use private key from PKCS12 file '%s'",
              cert_pk12);
        EVP_PKEY_free(pri);
        X509_free(x509);
        return 0;
    }

    EVP_PKEY_free(pri);
    X509_free(x509);

    /* We've loaded both certificate and the key, check if they match */
    if (SSL_CTX_check_private_key(ctx) != 1) {
        fprintf(stderr, "tcpOpensslProxyServer::%s %d Server's certificate and the key don't match\n", __FUNCTION__, __LINE__);
        onSslErrors (QString("Server's certificate and the key don't match"));
        freeSSLContext (ctx);
        return NULL;
    }

    //mTLS happens here. Server verify client as well.
    bool is_mTLS = true;
    if (is_mTLS) {
        /* We won't handle incomplete read/writes due to renegotiation */
        SSL_CTX_set_mode(ctx, SSL_MODE_AUTO_RETRY);

        /* Specify that we need to verify the client as well */
        SSL_CTX_set_verify(ctx,
                       SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT,
                       NULL);

        /* We accept only certificates signed only by the CA himself */
        SSL_CTX_set_verify_depth(ctx, 1);
    }

    /* Done, return the context */
    return ctx;

}


bool tcpOpensslProxyServer::SetSocketBlockingEnabled(int fd, bool blocking)
{
     if (fd < 0) return false;
   #ifdef WIN32
       unsigned long mode = blocking ? 0 : 1;
       return (ioctlsocket(fd, FIONBIO, &mode) == 0) ? TRUE : FALSE;
   #else
       int flags = fcntl(fd, F_GETFL, 0);
       if (flags < 0) return false;
       flags = blocking ? (flags&~O_NONBLOCK) : (flags|O_NONBLOCK);
       return (fcntl(fd, F_SETFL, flags) == 0) ? true : false;
   #endif
}

int tcpOpensslProxyServer::get_socket(const char* ipAddress, unsigned short port_num) {

    struct sockaddr_in sin;
    int sock, val;

    /* Create a socket */
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "tcpOpensslProxyServer::%s %d Cannot create a socket\n", __FUNCTION__, __LINE__);
        onSocketError (QString ("Cannot create a socket"));
        return -1;
    }

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) < 0) {
        fprintf(stderr, "tcpOpensslProxyServer::%s %d Could not set SO_REUSEADDR on the socket\n", __FUNCTION__, __LINE__);
        onSocketError (QString ("Could not set SO_REUSEADDR on the socket"));
        ::close(sock); return -1;
    }

    /* Fill up the server's socket structure */
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = inet_addr(ipAddress);
    sin.sin_port = htons(port_num);


    /* Bind the socket to the specified port number */
    if (bind(sock, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
        fprintf(stderr, "tcpOpensslProxyServer::%s %d Could not bind the socket\n", __FUNCTION__, __LINE__);
        onSocketError (QString ("Could not bind the socket"));
        ::close(sock); return -1;
    }

    /* Specify that this is a listener socket */
    if (listen(sock, SOMAXCONN) < 0) {
        fprintf(stderr, "tcpOpensslProxyServer::%s %d Failed to listen on this socket\n", __FUNCTION__, __LINE__);
        onSocketError (QString ("Failed to listen on this socket"));
        ::close(sock); return -1;
    }

    /* Done, return the socket */
    return sock;

}


void *connection_handler(void *instance){
    struct sockaddr_in sin;
    socklen_t sin_len;
    SSL *ssl;
    int net_fd;

    tcpOpensslProxyServer* instanceServer = (tcpOpensslProxyServer*)instance;

    SSL_CTX *ctx = instanceServer->m_serverSSlCtx;
    int listen_fd = instanceServer->m_serverSocketListenfd;


    struct timeval tv;
    tv.tv_sec = 20; //second
    setsockopt(listen_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    fprintf(stderr, "tcpOpensslProxyServer::%s %d Server waiting to accept connection\n", __FUNCTION__, __LINE__);
    /* Get to work */
    while (instanceServer->m_isServerprocessingEnabled) {
        /* Hold on till we can an incoming connection */
        sin_len = sizeof(sin);
        net_fd = ::accept(listen_fd, (struct sockaddr *) &sin, &sin_len);
        if (net_fd < 0) {
            int err = errno;
            //strerror(errno); error  message.
            if (EAGAIN != err) {
                instanceServer->onAcceptError (QString("Failed to accept connection"));
                fprintf(stderr, "tcpOpensslProxyServer::%s %d Failed to accept connection net_fd:%d  err:%d %s\n", __FUNCTION__, __LINE__, net_fd, err, strerror(errno));
            }
            continue;
        }

        /* Get an SSL handle from the context */
        if (!(ssl = SSL_new(ctx))) {
            fprintf(stderr, "tcpOpensslProxyServer::%s %d Could not get an SSL handle from the context\n", __FUNCTION__, __LINE__);
            instanceServer->onSslErrors (QString("Could not get an SSL handle from the context"));
            ::close(net_fd); //return -1;
        }

        /* Associate the newly accepted connection with this handle */
        SSL_set_fd(ssl, net_fd);


        instanceServer->onNewConnection (net_fd, instanceServer);

        connectionContext cc;
        cc.ssl = ssl;
        cc.instance = (tcpOpensslProxyServer*)instance;
        std::thread t1(clientRead_handler, &cc);
        t1.detach();

    }
    printf ("tcpOpensslProxyServer::%s %d server thread exited\n", __FUNCTION__, __LINE__);
    pthread_exit(NULL);
}

void *clientRead_handler(void *socket_desc)
{

    static char buffer[BUFSIZE];
    int rc, len;
    bool isReadEnabled = true;

    printf("\ntcpOpensslProxyServer::%s:%d\n", __FUNCTION__, __LINE__);
    connectionContext *cc = (connectionContext*) socket_desc;
    SSL *ssl = cc->ssl;
    int net_fd = SSL_get_fd(ssl);
    tcpOpensslProxyServer* instance = cc->instance;
    //Client siganl condition to unlock.


    /* Now perform handshake */
    if ((rc = SSL_accept(ssl)) != 1) {
        fprintf(stderr, "tcpOpensslProxyServer::%s %d Could not perform SSL handshake\n", __FUNCTION__, __LINE__);
        instance->onPeerVerifyError ("Could not perform SSL handshake");
        if (rc != 0) {
            SSL_shutdown(ssl);
        }
        SSL_free(ssl);
        pthread_exit(NULL);
    }

    //Write context is only required after ssl hand shake is done.
    instance->m_clientSSLCtxList [net_fd] = ssl;
    instance->onConnected (net_fd, QString("connected")); //need to check how to get connection ip string here

    /* Print success connection message on the server */
    printf("tcpOpensslProxyServer::%s %d SSL handshake successful\n", __FUNCTION__, __LINE__);


    instance->m_clientSSLIsReadEnabledList [net_fd] = &isReadEnabled;
    struct timeval tv;
    tv.tv_sec = 20; //second
    tv.tv_usec = 0; //micro sec
    setsockopt(net_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

    len = 0;
    memset (buffer, '\0', BUFSIZE);
    while (isReadEnabled) {
        len = SSL_read(ssl, buffer, BUFSIZE);

        if (len < 0) {
            //SSL read timeout. Continue here. if server is stopped isReadEnabled will be zero
            //else it will go to next read operation
            continue;
        } else if (0 == len) {
            printf("\ntcpOpensslProxyServer::%s:%d debug socket read data len:%d. Closing the socket\n", __FUNCTION__, __LINE__, len);
            //This means client disconnected. call client stop here.
            instance->closeClient(net_fd);
        }else {
            //Emit message received only if read enabled for the client.
            if (isReadEnabled) {
                //send the message if it is not pong.
                bool isPong = false;
                if (len>7){
                    if (0 == strncmp(buffer, "##PING##", 8)){
                        //PING received will go to on pong
                        instance->onPong (net_fd, buffer+8, len-8);
                        isPong = true;
                    }
                }
                if (!isPong){
                    printf("tcpOpensslProxyServer::%s:%d received: actual data is masked len:%d", __FUNCTION__, __LINE__, len);
                    instance->onMessageReceived (net_fd, buffer, len);
                }
	    }
        }
        memset (buffer, '\0', BUFSIZE);
    }
    printf("\ntcpOpensslProxyServer::%s %d client:%d read exited\n", __FUNCTION__, __LINE__, net_fd);

    instance->onDisconnected (net_fd);
    instance->m_clientSSLCtxList.erase (net_fd);


    /* Cleanup the SSL handle */
    SSL_shutdown(ssl);
    SSL_free(ssl);
    ::close(net_fd);
    instance->closeClient (net_fd);

    printf ("\ntcpOpensslProxyServer::%s %d thread exited len:%d\n", __FUNCTION__, __LINE__, len);
    pthread_exit(NULL);

}

void tcpOpensslProxyServer::sendTextMessage (int clientId, const char* buffer, int len){

    int rc = 0;
    if (m_clientSSLCtxList.find(clientId) != m_clientSSLCtxList.end()){
        SSL *ssl = m_clientSSLCtxList[clientId];
        if ((rc = SSL_write(ssl, buffer, len)) != len){
            fprintf(stderr, "tcpOpensslProxyServer::%s %d SSL write failed\n", __FUNCTION__, __LINE__);
        }

    }
    else {
        printf ("\ntcpOpensslProxyServer::%s %d Not able to signal the write thread operation for the client: %d\n", __FUNCTION__, __LINE__, clientId);
    }

}

void tcpOpensslProxyServer::setPingMsg (int clientId, const char* uniqueId, int len){
    std::string s = "##PING##";
    s.append (uniqueId, len);
    m_clientPingMsgList [clientId] = s;
}
void tcpOpensslProxyServer::ping (int clientId){
    std::string s = "##PING##DUMMY";
    if (m_clientPingMsgList.find(clientId) != m_clientPingMsgList.end()){
        s = m_clientPingMsgList [clientId];
    }
    sendTextMessage (clientId, s.c_str(), strlen(s.c_str()));
    std::chrono::high_resolution_clock::time_point pingTime = std::chrono::high_resolution_clock::now();
    m_clientPingTimeList [clientId] = pingTime;
}

void tcpOpensslProxyServer::onPong (int clientId, char* msg, int len){
    setPingMsg (clientId, msg, len);
    QByteArray databuf = QByteArray((char*)msg, len);
    std::chrono::high_resolution_clock::time_point pingTime;
    if (m_clientPingTimeList.find(clientId) != m_clientPingTimeList.end()){
        pingTime = m_clientPingTimeList [clientId];
    } else {
        pingTime = std::chrono::high_resolution_clock::now();
    }
    std::chrono::high_resolution_clock::time_point pongTime = std::chrono::high_resolution_clock::now();
    std::uint64_t ticks = (std::chrono::duration_cast<std::chrono::milliseconds>(pongTime - pingTime)).count();
    if (NULL != onPongCallBack){
        onPongCallBack (clientId, (quint64)ticks, databuf);
    } else {
        fprintf(stderr, "tcpOpensslProxyServer::%s:%d onPongCallBack NULL\n",__FUNCTION__, __LINE__);
    }
}

void tcpOpensslProxyServer::onNewConnection (int clientId, tcpOpensslProxyServer* ss){
    emit newConnection (clientId, ss);
}

void tcpOpensslProxyServer::onConnected (int clientId, QString msg){
    emit connected (clientId, msg);
}

void tcpOpensslProxyServer::onMessageReceived (int clientId, char* message, size_t len){
    if (NULL != onMessageReceivedCallBack){
        onMessageReceivedCallBack (clientId, message, len);
    }
}

void tcpOpensslProxyServer::onDisconnected (int clientId){
    emit disconnected (clientId);
}

void tcpOpensslProxyServer::onSocketError (QString err){
    emit socketError (err);
}

void tcpOpensslProxyServer::onSslErrors (QString err) {
    emit sslErrors (err);
}

void tcpOpensslProxyServer::onAcceptError (QString err){
    emit acceptError (err);
}

void tcpOpensslProxyServer::onPeerVerifyError (QString err){
    emit peerVerifyError (err);
}
