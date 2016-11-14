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


#ifndef TRM_QT_WEBSOCKET_PROXY_H_
#define TRM_QT_WEBSOCKET_PROXY_H_

#include <time.h>
#include <sys/time.h>
#include <stddef.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <QtCore/QObject>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QTimer>
#include <QtCore/QTime>
#include <QtCore/QByteArray>
#include <QAbstractSocket>
#ifdef TRM_USE_SSL
#include <QtNetwork/QSslError>
#endif

class QWebSocketServer;
class QWebSocket;
class PingPongTask;
class QStringList;

class WebSocketProxy : public QObject
{
    Q_OBJECT
public:
    explicit WebSocketProxy(const QStringList &boundIPs, quint16 port, QObject *parent = Q_NULLPTR);

Q_SIGNALS:

private Q_SLOTS:

#ifdef TRM_USE_SSL
    void onSslErrors(const QList<QSslError> &errors);
#endif
    void onNewConnection(void);
    void onWebsocketConnect(void);
    void onWebsocketBinaryMessageReceived(QByteArray byteArray);
    void onWebsocketTextMessageReceived(QString);
    void onWebsocketBytesWritten(qint64);
    void onWebsocketDisconnected(void);
    void onWebsocketStateChanged(QAbstractSocket::SocketState);
    void onWebSocketAboutClose(void);
    void onWebsocketError(QAbstractSocket::SocketError);
    void onWebsocketHasDataToWrite(void *, void*);
    void onRemoveConnection(void *);

private:
    QMap<QString, QWebSocketServer *>proxyServers;
    QList<QWebSocket *> connections; 
    QMap<QWebSocket *, PingPongTask *> pingPongTasks;
};

class PingPongTask : public QObject {
    Q_OBJECT
public:
    PingPongTask();
    PingPongTask(QWebSocket &wssocket);
    virtual ~PingPongTask(void);
    void start(void);
    void stop(void);

public Q_SLOTS:
    void onTimeout(void);
    void onPong(quint64 elapsedTime, QByteArray);

private:
    QWebSocket &wssocket;
    QTimer timer;
    int retry;
    bool stopped;
};

#define _TRMPRX_OUT_ stdout
#define __TIMESTAMP() do { /*YYMMDD-HH:MM:SS:usec*/               \
                struct tm __tm;                                             \
                struct timeval __tv;                                        \
                gettimeofday(&__tv, NULL);                                  \
                localtime_r(&__tv.tv_sec, &__tm);                           \
                fprintf(_TRMPRX_OUT_, "\r\n[tid=%ld]: %02d%02d%02d-%02d:%02d:%02d:%06d ",                 \
                                                                        syscall(SYS_gettid), \
                                                                        __tm.tm_year+1900-2000,                             \
                                                                        __tm.tm_mon+1,                                      \
                                                                        __tm.tm_mday,                                       \
                                                                        __tm.tm_hour,                                       \
                                                                        __tm.tm_min,                                        \
                                                                        __tm.tm_sec,                                        \
                                                                        (int)__tv.tv_usec);                                      \
} while(0)                              

#endif //TRM_WEBSOCKET_PROXY_H_


/** @} */
/** @} */
