#ifndef TRM_QT_WEBSOCKETSERVER_H
#define TRM_QT_WEBSOCKETSERVER_H

#include <openssl/ssl.h>

#include <iostream>
#include <QtCore/QObject>
#include <QString>

#include <unordered_map>
#include <functional>

#include <chrono>         // std::chrono::seconds

class tcpOpensslProxyServer : public QObject
{
Q_OBJECT

public:
    tcpOpensslProxyServer ();
    ~tcpOpensslProxyServer();
    int setUpServer(const char* ipAddress, unsigned short port_num, const char *ca_pem,
            const char *cert_pem, const char *key_pem);

signals:
    void newConnection (int clientId, tcpOpensslProxyServer* ss);
    void connected (int clientId, QString msg);
    void messageReceived (int clientId, char* message, size_t len);
    void disconnected (int clientId);
    void socketError (QString err);
    void sslErrors (QString err);
    void acceptError (QString err);
    void peerVerifyError (QString err);


private:
    void freeSSLContext(SSL_CTX *ctx);
    SSL_CTX * get_server_context_pk12(const char *ca_pem,
                const char *cert_pk12, const char *pass);
    int get_socket(const char* ipAddress, unsigned short port_num);
    bool SetSocketBlockingEnabled(int fd, bool blocking);

    void onSocketError (QString err);
    void setPingMsg (int clientId, const char* uniqueId, int len);
    std::unordered_map <int, std::chrono::high_resolution_clock::time_point> m_clientPingTimeList;

public:
    bool m_isServerprocessingEnabled;
    std::unordered_map <int, SSL*> m_clientSSLCtxList;
    std::unordered_map <int, bool*> m_clientSSLIsReadEnabledList;
    std::unordered_map <int, std::string> m_clientPingMsgList;
    SSL_CTX *m_serverSSlCtx;
    int m_serverSocketListenfd;
    tcpOpensslProxyServer* m_instanceServer;

    std::function<void(int, char*, unsigned int)> onMessageReceivedCallBack;
        std::function<void(int, long long unsigned int, QByteArray&)> onPongCallBack;

    void closeClient(int clientId);
    void closeAllClients();
    void stopServer();

    void onNewConnection (int clientId, tcpOpensslProxyServer* ss);
    void onConnected (int clientId, QString msg);
    void onMessageReceived (int clientId, char* message, size_t len);
    void onDisconnected (int clientId);
    void onSslErrors (QString err);
    void onPeerVerifyError (QString err);
    void onAcceptError (QString err);

    void sendTextMessage (int clientId, const char* buffer, int len);
    void ping (int clientId);
    void onPong (int clientId, char* msg, int len);
};

#endif //TRM_QT_WEBSOCKETSERVER_H
