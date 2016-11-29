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


#ifndef _TRM_CLIENT_H
#define _TRM_CLIENT_H
#include <QObject>
#include <QtCore>
#include <QTcpServer>
#include <QList>
#include <QApplication>

#include "Connection.h"
#include "trm/MessageProcessor.h"

namespace TRM {

class Client : public QObject
{
	Q_OBJECT
public:
	Client (const char *ipaddress, int portNumber);
	Client(const QHostAddress &address, quint16 port);
	virtual ~Client(void);
	const Connection & getConnection(void) const { return *connection;}

private slots:
	void onDisconnected(Connection *_connection);
	void onMessageReceived(const Connection &fromConnection);
	virtual MessageProcessor & getMessageProcessor(uint32_t inClientId) {
		static MessageProcessor proc;
		return proc;
	}
protected:
	QTcpSocket tcpSocket;
	Connection *connection;

};

}

#endif


/** @} */
/** @} */
