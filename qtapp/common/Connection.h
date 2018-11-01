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


#ifndef _TRM_CONNECTION_H
#define _TRM_CONNECTION_H

#include <QTcpSocket>
#include <QObject>
#include <QDebug>
#include <QQueue>
#include <QByteArray>

#include "trm/TRM.h"
#include "trm/Header.h"

namespace TRM {

class Connection : public QObject
{
	Q_OBJECT
public:

	static const uint32_t kInvalidClientId  	=  (~0);
	static const uint32_t kRecorderClientId 	= 0XFFFFFF00;
	static const uint32_t kTunerAgentId     	= 0XFFFFFF02;
	static const uint32_t kSDVAgentId     	    = 0XFFFFFF03;
	static const uint32_t kTRMMgrClientId 		= 0XFFFFFF04;
	static const uint32_t kTrmClientId 			= 0x00000000;
	static const uint32_t kTrmMonitorId 		= 0xFFFFFFF0;
	static bool isTestClient(uint32_t clientId) {
		return ((clientId != kRecorderClientId) &&
			   ((clientId >= kTrmMonitorId)));
	}

    static bool isXREClient(uint32_t clientId) {
        return (clientId > 0 && clientId < 0xFFFFFF00);
    }

	static bool isTunerAgentClient(uint32_t clientId) {
		return ((clientId == kTunerAgentId));
	}

	static bool isTRMDiagClient(uint32_t clientId) {
		return ((clientId == kTRMMgrClientId));
	}

	static bool isSDVAgentClient(uint32_t clientId) {
		return ((clientId == kSDVAgentId));
	}

	Connection(QTcpSocket &_tcpSocket);
	virtual ~Connection();

	void send(const std::vector<uint8_t> &out) const;
	void sendMessage(const QByteArray &outgoingMessage) const;
	/* caller of recvMessage(void) must free the buffer received */
	void recvMessage(QByteArray &recvBuffer) const;
	const QByteArray & recvMessage(void) const;

	void sendAsync(const std::vector<uint8_t> &out) const;

private:
	class State {
	public:
		State(void) : dataLength(0) {}
		virtual QByteArray & read(QAbstractSocket &socket, QByteArray &readBuffer) = 0;
		virtual const char * stateName(void) = 0;
		void setDataLength(size_t length) { dataLength = length;};
		size_t getDataLength(void) { return dataLength;};
	private:
		size_t dataLength;
	};

	class IdleState : public State {
	public:
		QByteArray & read(QAbstractSocket &socket, QByteArray &readBuffer);
		const char * stateName(void) { return "IdleState";};
	};

	class WaitHeaderState : public State {
	public:
		QByteArray & read(QAbstractSocket &socket, QByteArray &readBuffer);
		const char * stateName(void) { return "WaitHeaderState";};
	};

	class WaitPayloadState : public State {
	public:
		QByteArray & read(QAbstractSocket &socket, QByteArray &readBuffer);
		const char * stateName(void) { return "WaitPayloadState";};
	};

	QAbstractSocket & getSocket(void)
	{
		return socket;
	}


signals:
	void disconnected(Connection *);
	void messageReceived(const Connection &);
	void readyRead(void);
	void hasMessageToSend(const std::vector<uint8_t> out) const;

public slots:
	void onConnected();
	void onReadyRead(void);
	void onBytesWritten(qint64 bytes);
	void onDisconnected(void);
	void onSocketError(QAbstractSocket::SocketError socketError);
	void onStateChanged(QAbstractSocket::SocketState socketState);
	void onHasMessageToSend(const std::vector<uint8_t> out) const;

private:
	IdleState idleState;
	WaitHeaderState  waitHeaderState;
	WaitPayloadState waitPayloadState;

	QAbstractSocket &socket;
	mutable QQueue<QByteArray *> incomingMessages;
	QByteArray *messageByteArray;
	State  *state;
	mutable uint32_t clientId;
};

}
#endif


/** @} */
/** @} */
