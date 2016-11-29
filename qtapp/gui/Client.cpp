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


#include <QObject>
#include <QtCore>
#include <QTcpServer>
#include <QList>
#include <QApplication>

#include "trm/Header.h"
#include "trm/JsonEncoder.h"
#include "trm/JsonDecoder.h"

#include "Connection.h"
#include "Client.h"

using namespace TRM;

Client::Client (const char *ipaddress, int portNumber)
{
	Client(QHostAddress(ipaddress), portNumber);
}

Client::Client(const QHostAddress &address, quint16 port)
{
	qDebug() << "Creating client\r\n";
	std::cout << ("Connecting To Server ...");
	do {
		tcpSocket.connectToHost(address, port);
		std::cout << ".";
		std::flush(std::cout);
		sleep(1);
	} while(!tcpSocket.waitForConnected(1000));

	connection = new Connection(tcpSocket);
	QObject::connect(connection, SIGNAL(disconnected(Connection *)), this, SLOT(onDisconnected(Connection *)));
	QObject::connect(connection, SIGNAL(messageReceived(const Connection &)), this, SLOT(onMessageReceived(const Connection &)));
}


Client::~Client(void)
{
	printf("Deleting client\r\n");
	tcpSocket.close();
	//@TODO: traverse and delete processors.
	if (connection != 0) {
		printf("Deleting connection\r\n");
		delete connection;
	}
}

void Client::onDisconnected(Connection *_connection)
{
	_connection = _connection;
	printf("CLIENT CONNEC is disconnected\r\n");
	delete connection;
	connection = 0;
}

void Client::onMessageReceived(const Connection &fromConnection)
{
	QByteArray incomingMessageArray;
	fromConnection.recvMessage(incomingMessageArray);
	std::vector<uint8_t> headerBytes (incomingMessageArray.data(), incomingMessageArray.data() + Header::kHeaderLength);
	std::vector<uint8_t> payloadBytes (incomingMessageArray.data() + Header::kHeaderLength, incomingMessageArray.data() + incomingMessageArray.size());
	Header header;
	header.deserialize(headerBytes);
	JsonDecoder jdecoder(getMessageProcessor(header.getClientId()));
	jdecoder.decode(payloadBytes);
}



/** @} */
/** @} */
