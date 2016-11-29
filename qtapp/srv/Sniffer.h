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


#ifndef _TRM_SNIFFER_H
#define _TRM_SNIFFER_H
#include <stdint.h>
#include <QObject>
#include <QtCore>
#include <QUdpSocket>
#include <QHostAddress>

#if TRM_SNIFFER_ENABLED
namespace TRM {

class Server;

class Sniffer : public QObject
{
	Q_OBJECT
public:
	Sniffer(const Server &server, const QHostAddress &fromAddr, quint16 fromPort, const QHostAddress &outAddr, quint16 outPort );
    ~Sniffer(void);

private slots:
	void onMessageIn(const std::vector<uint8_t> in);
	void onMessageOut(const std::vector<uint8_t> out);

private:
    QUdpSocket outgoingSocket;
    QHostAddress outAddr;
    quint16 outPort;

};

}
#endif
#endif


/** @} */
/** @} */
