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
* @defgroup qtapp
* @{
**/


#ifndef _TRM_SERVER_H
#define _TRM_SERVER_H
#include <QObject>
#include <QtCore>
#include <QTcpServer>
#include <QList>
#include <QMap>
#include <QThreadPool>

#include "Connection.h"

namespace TRM {

class Server : public QObject
{
	Q_OBJECT
public:
	Server(const QHostAddress &address, quint16 port, bool start = true, bool sniff = false);
	virtual ~Server(void);
    void start(void);
	void close(void);
	const Connection & getConnection(uint32_t clientId) const;
	void getConnections(std::list<const Connection *> &) const;

signals:
	void messageIn(const std::vector<uint8_t> in) const;
	void messageOut(const std::vector<uint8_t> out) const;

private slots:
	void onDisconnected(Connection *connection);
	void onMessageReceived(const Connection &fromConnection);
    void onHasMessageToSend(const std::vector<uint8_t> out);
	void onReservationUpdated(void);
	void onNewConnection(void);

//	void onTunerReservationExpired(const TunerReservation, QString reason);

private:

private:
	void sendResponseObject(const std::vector<uint8_t> &out, const Connection &connection);
	QList<const Connection *>connections;
	QMap<uint32_t, const Connection *> connectionClientMap;
	QTcpServer  *tcpServer;
    bool started;
    const QHostAddress serverAddress;
    quint16 serverPort;
    bool sniff;

};

}
#endif


/** @} */
/** @} */
