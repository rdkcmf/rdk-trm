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


#include <QThread>
#include <QDebug>
#include <QDataStream>
#include <QThreadPool>

#include <vector>

#include "Util.h"
#include "Connection.h"
#include "trm/Header.h"

void printThread(const char *func)
{
	int threadId = (int)QThread::currentThread();
	printf("[%s]==========================Run  from thread %x\r\n", func, threadId);
}


using namespace TRM;
#define _USE_THREADPOOL_ 1

#if 1

Connection::Connection(QTcpSocket &_tcpSocket) : socket(_tcpSocket), state(&idleState), clientId(kInvalidClientId)
{
	Assert(state == &idleState);

	waitHeaderState.setDataLength(Header::kHeaderLength);
	messageByteArray = new QByteArray();
	state = &waitHeaderState;

	QObject::connect(&socket, SIGNAL(disconnected()), this, SLOT(onDisconnected()));
    QObject::connect(&socket, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
	QObject::connect(&socket, SIGNAL(connected()), this, SLOT(onConnected()));
	QObject::connect(&socket, SIGNAL(bytesWritten(qint64)), this, SLOT(onBytesWritten(qint64)));
    QObject::connect(&socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onSocketError(QAbstractSocket::SocketError)));
    QObject::connect(&socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(onStateChanged(QAbstractSocket::SocketState)));

    QObject::connect(this, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
    QObject::connect(this, SIGNAL(hasMessageToSend(const std::vector<uint8_t>)), this, SLOT(onHasMessageToSend(const std::vector<uint8_t>)));

};

Connection::~Connection(void)
{

#if _USE_THREADPOOL_
	QThreadPool::globalInstance()->waitForDone();
#endif

	while (!incomingMessages.isEmpty()) {
		QByteArray *byteArray = incomingMessages.dequeue();
		delete byteArray;
	}

	QObject::disconnect(this, SLOT(onDisconnected()));
    QObject::disconnect(this, SLOT(onReadyRead()));
	QObject::disconnect(this, SLOT(onConnected()));
	QObject::disconnect(this, SLOT(onBytesWritten(qint64)));
    QObject::disconnect(this, SLOT(onSocketError(QAbstractSocket::SocketError)));
    QObject::disconnect(this, SLOT(onStateChanged(QAbstractSocket::SocketState)));

    QObject::disconnect(this, SLOT(onReadyRead()));
    QObject::disconnect(this, SLOT(onHasMessageToSend(const std::vector<uint8_t>)));

	Log() << "Connection " << (void *)this << " is deleted" << std::endl;
}
#endif

void Connection::send(const std::vector<uint8_t> &out) const
{
	::printThread("Connection::send");

	socket.write((const char *)&out[0], out.size());
	socket.flush();
}

void Connection::sendAsync(const std::vector<uint8_t> &out) const
{
	Log() << "sendAsync via emit" << std::endl;
	emit hasMessageToSend(out);
}


void Connection::sendMessage(const QByteArray &sendBuffer) const
{
	::printThread("Connection::sendMessage");

	socket.write(sendBuffer);
	socket.flush();
}

void Connection::recvMessage(QByteArray &recvBuffer) const
{
	::printThread("Connection::recvMessage");

	if (!incomingMessages.isEmpty()) {
		QByteArray *byteArray = incomingMessages.dequeue();
		recvBuffer = *byteArray;
		delete byteArray;
	}
}

const QByteArray & Connection::recvMessage(void) const
{
	if (incomingMessages.isEmpty()) {
		throw InvalidOperationException();
	}

	QByteArray *byteArray = incomingMessages.dequeue();
	return *byteArray;
}

void Connection::onBytesWritten(qint64 bytes)
{
	Log() << "Connection written " << bytes << "bytes" << std::endl;
}

void Connection::onConnected(void)
{
	Log() << "Connection establised " << std::endl;
}

void Connection::onHasMessageToSend(const std::vector<uint8_t> out) const
{
	Log() << "onHasMessageToSend" << std::endl;
	send(out);
}

void Connection::onReadyRead(void)
{
	if (messageByteArray == 0) {
		//Connection already deleted.
		return;
	}

	try {
		(*state).read(socket, *messageByteArray);
		/*
		 * The readyRead() signal is emitted every time a new chunk of data has arrived.
		 * state: IDLE --> READ_HEADER -- > READ_PAYLOAD --> IDLE
		 */
		if (state == &waitHeaderState) {
			Header header;
			std::vector<uint8_t> headerbytes(messageByteArray->constData(), messageByteArray->constData() + (*state).getDataLength());
			header.deserialize(headerbytes);

	 		Log() << "Trans to waitPayloadState and expcets " << header.getPayloadLength() << " Bytes" << std::endl;

			waitPayloadState.setDataLength(header.getPayloadLength());
			state = &waitPayloadState;
		}
		else if (state == &waitPayloadState) {

	 		Log() << "Enqueing msg with " << messageByteArray->count() << " Bytes" << std::endl;

			incomingMessages.enqueue(messageByteArray);
			emit messageReceived(*this);

	 		Log() << "Trans to waitHeaderState and expcets " << Header::kHeaderLength << " Bytes" << std::endl;

			messageByteArray = new QByteArray();
			waitHeaderState.setDataLength(Header::kHeaderLength);
			state = &waitHeaderState;
		}

		if ((socket.bytesAvailable() > 0) || (state->getDataLength() == 0)) emit readyRead();

	}
	catch (...) {
 		Log() << "Not enough data arriaval " << std::endl;

	}
}

QByteArray & Connection::IdleState::read(QAbstractSocket &, QByteArray &readBuffer)
{
	/*Do nothing */
	Assert(0);

	return readBuffer;
}

QByteArray & Connection::WaitHeaderState::read(QAbstractSocket &socket, QByteArray &readBuffer)
{
	readBuffer.clear();
	State::read(socket, readBuffer);
	/* Buffer should contains header bytes only */
	Assert(readBuffer.size() == (int)getDataLength());

	for (int i = 0; i < readBuffer.count(); i++) {
		printf("%02X ", readBuffer.constData()[i]);
	}
	printf("\r\n");

	return readBuffer;
}

QByteArray & Connection::WaitPayloadState::read(QAbstractSocket &socket, QByteArray &readBuffer)
{
	if (getDataLength() == 0) {
		/* Empty Message */
		printf("Received Empty Message\r\n");
		return readBuffer;
	}

	State::read(socket, readBuffer);
	/* Buffer should contains header bytes + payload bytes */
	Assert(readBuffer.size() == (getDataLength() + Header::kHeaderLength));

	for (int i = Header::kHeaderLength; i < readBuffer.count(); i++) {
		printf("%C", readBuffer.constData()[i]);
	}
	printf("\r\n");

	return readBuffer;
}

QByteArray & Connection::State::read(QAbstractSocket &socket, QByteArray &readBuffer)
{
	size_t count = 0;
	/* Read expected bytes */
	if (socket.bytesAvailable() >= getDataLength()) {
 		Log() << "At State " << stateName() << "expcets " << getDataLength() << " but receives " << socket.bytesAvailable() << " Bytes" << std::endl;
		char * expectedBytes = new char[getDataLength()];
		count = socket.read(expectedBytes, getDataLength());
		if (count == getDataLength()) {
	 		Log() << "At State " << stateName() << "read " << count << " bytes "  << std::endl;
			readBuffer.append(expectedBytes, getDataLength());
		}
		else if (count == -1) {
			// Disconnected.
		}

		delete[] expectedBytes;
	}
	else {
		// Throw data_unavailable exception.
	}

	if (count <= 0) throw count;

	return readBuffer;
}

void Connection::onDisconnected(void)
{
	Log()<< "onDisconnected()" << std::endl;
	state = &idleState;
	emit disconnected(this);
}

void Connection::onSocketError(QAbstractSocket::SocketError socketError)
{
	Log()<< "onSocketError()" << socketError;
}
void Connection::onStateChanged(QAbstractSocket::SocketState socketState)
{
	Log()<< "onStateChanged()" << socketState;
}


/** @} */
/** @} */
