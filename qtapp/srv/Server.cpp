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


#include <errno.h>
#include <stdio.h>

#include <QObject>
#include <QTcpSocket>
#include <QDataStream>
#include <QString>
#include <QCoreApplication>
#include <QThread>

#include "trm/TRM.h"
#include "trm/JsonDecoder.h"
#include "Connection.h"
#include "trm/Header.h"
#include "ServerMessageProcessor.h"
#include "Manager.h"
#include "Util.h"

#include "Server.h"

extern void printThread(const char *func);

using namespace TRM;

Server::~Server(void)
{
	//@TODO: traverse and delete connections.
	tcpServer->close();
	delete tcpServer;
}

void Server::close(void)
{
	tcpServer->close();
}

const Connection & Server::getConnection(uint32_t clientId) const
{
	QMap<uint32_t, const Connection *>::const_iterator it =  connectionClientMap.find(clientId);
	if (it == connectionClientMap.end()) {
		throw ConnectionNotFoundException();
	}

	Assert(it.key() == clientId);
	return *(it.value());
}

void Server::getConnections(std::list<const Connection *> &connectionlist) const
{
    QList<const Connection *>::const_iterator it = connections.begin();
    while (it != connections.end()) {
    	connectionlist.push_back(*it);
    	it++;
    }
}

void Server::onDisconnected(Connection *connection)
{
	/* connection is disconnected */
	Log() << "Disconnected connection index is " << (void *)connection << std::endl;
	/* remove all clients */
	QMap<uint32_t, const Connection *>::iterator it =  connectionClientMap.begin();
	while(it != connectionClientMap.end()) {
		if (it.value() == connection) {
			Log() << "Removing client =======================" << (void *)it.key() << std::endl;

			/* relase all reservations that belongs to this client */
			TunerReservation::TokenList tokensForClient;
			Manager::getInstance().getReservationTokens(tokensForClient, it.key());

			TunerReservation::TokenList::const_iterator itt = tokensForClient.begin();
			while(itt != tokensForClient.end()) {
				Log() << "Releasing Token " << (*itt).c_str() << " for client " << (void *)it.key() << std::endl;
				Manager::getInstance().releaseReservation(*itt);
				itt++;
			}

			it = connectionClientMap.erase(it);
		}
		else {
			it++;
		}
	}

	if (connections.removeOne(connection)) {
		connection->deleteLater();
	}
	else {
		Assert(0);
	}
}

void Server::onMessageReceived(const Connection &fromConnection)
{
	try {
		const QByteArray & incomingMessageArray =  fromConnection.recvMessage();
		std::vector<uint8_t> headerBytes  (incomingMessageArray.constData(), incomingMessageArray.constData() + Header::kHeaderLength);
		std::vector<uint8_t> payloadBytes (incomingMessageArray.constData() + Header::kHeaderLength, incomingMessageArray.constData() + incomingMessageArray.size());
       

        /* Send Data for Diag if TRM Manager  is connected */
        uint32_t clientId = Connection::kTRMMgrClientId;
	    QMap<uint32_t, const Connection *>::iterator itCon =  connectionClientMap.find(clientId);
	    if(itCon != connectionClientMap.end() && Connection::isTRMDiagClient((itCon).key())) {
			Log() << "Server::Sent Recvd Message to TRMDiagClient" << std::endl;
	        std::vector<uint8_t> out(incomingMessageArray.begin(),incomingMessageArray.end());
	        (itCon.value())->onHasMessageToSend(out);
	    }

		delete &incomingMessageArray;
		Header header;
		header.deserialize(headerBytes);

		QMap<uint32_t, const Connection *>::iterator it =  connectionClientMap.find(header.getClientId());

		if (it == connectionClientMap.end()) {
			connectionClientMap[header.getClientId()] = &fromConnection;
		}

		it =  connectionClientMap.find(header.getClientId());
		if (it != connectionClientMap.end()) {
			if (payloadBytes.size() > 0) {
				ServerMessageProcessor myP(&fromConnection, header.getClientId());
				JsonDecoder jdecoder(myP);
				jdecoder.decode(payloadBytes);
			}
			else {
			    /* Reset clientId's connection on empty message. Client has disconnected */
				/* connection is disconnected */
				Log() << "Disconnected clientId is " <<  header.getClientId()  << std::endl;
				/* remove all clients */
				QMap<uint32_t, const Connection *>::iterator it =  connectionClientMap.begin();
				while(it != connectionClientMap.end()) {
					if (it.value() == &fromConnection) {
						if (it.key() == header.getClientId()) {

							/* relase all reservations that belongs to this client */
							TunerReservation::TokenList tokensForClient;
							Manager::getInstance().getReservationTokens(tokensForClient, it.key());

							TunerReservation::TokenList::const_iterator itt = tokensForClient.begin();
							while(itt != tokensForClient.end()) {
								Log() << "Releasing Token " <<   (*itt).c_str() << "for client " << (void *)it.key() << std::endl;
								Manager::getInstance().releaseReservation(*itt);
								itt++;
							}

							//remove connection from the map
							Log() << "Removing client " <<  (void *)it.key() << std::endl;
							connectionClientMap.erase(it);
							break;
						}
				    }
					it++;
				}
			}
		}
		else {
			//Discard the invalid message.
			Assert(0);
		}
	}
	catch(...) {
		/* No data. Do nothing */
	}
}

void Server::onHasMessageToSend(const std::vector<uint8_t> out)
{
   /* Send Data for Diag if TRM Manager  is connected */
    uint32_t clientId = Connection::kTRMMgrClientId;
    QMap<uint32_t, const Connection *>::iterator it =  connectionClientMap.find(clientId);
    
    if(it != connectionClientMap.end() && Connection::isTRMDiagClient((it).key())) {
		std::vector<uint8_t> payloadBytes (out.data() + Header::kHeaderLength, out.data() + out.size());
        Log() << "Server::Sent Has Message to TRMDiagClient   " << std::endl;
        (it.value())->onHasMessageToSend(out);
    }
}

void Server::onNewConnection(void)
{
	qDebug() << ("New connection detected\r\n");
    QTcpSocket *clientSocket = tcpServer->nextPendingConnection();
    Connection *connection = new Connection(*clientSocket);
    connections.append(connection);
    QObject::connect(connection, SIGNAL(disconnected(Connection *)), this, SLOT(onDisconnected(Connection *)), Qt::QueuedConnection);
    QObject::connect(connection, SIGNAL(messageReceived(const Connection &)), this, SLOT(onMessageReceived(const Connection &)));
    /* Make it Sniff  independent to listen for TRM SRV response */
    QObject::connect(connection, SIGNAL(hasMessageToSend(const std::vector<uint8_t>)), this, SLOT(onHasMessageToSend(const std::vector<uint8_t>)));
}

void Server::onReservationUpdated(void)
{
	Log() << "Manager::onReservationUpdate  " << std::endl;

	if (1) {
		QMap<uint32_t, const Connection *>::iterator it =  connectionClientMap.begin();

		while (it != connectionClientMap.end()) {
			Log() << "Server::send NotifyTunerReservationUpdate to client  " << std::hex << (it).key() << std::endl;
			if (Connection::isXREClient((it).key()) || Connection::isSDVAgentClient((it).key()) || Connection::isTestClient((it).key())) 
			{
                try {
                    std::vector<uint8_t> out;
                    SerializeMessage(Manager::getInstance().getTunerStatesUpdate(), (it).key(), out);
                    (it.value())->sendAsync(out);
                }
                catch (...) {
                    std::cout << "Exception caught during getTunerStatesUpdate " << std::endl;
                    SafeAssert(0);
                }
			}
			it++;
		}
	}
}

Server::Server(const QHostAddress &address, quint16 port)
{
	//QThreadPool::globalInstance()->setMaxThreadCount(10);
	tcpServer = new QTcpServer();
	tcpServer->listen(address, port);
    QObject::connect(tcpServer, SIGNAL(newConnection()), this, SLOT(onNewConnection()));
   //QObject::connect(&Manager::getInstance(), SIGNAL(reservationUpdated(void)), this, SLOT(onReservationUpdated(void)));
    QObject::connect(&Manager::getInstance(), SIGNAL(reservationUpdated(void)), this, SLOT(onReservationUpdated(void)), Qt::QueuedConnection);
}


/** @} */
/** @} */
