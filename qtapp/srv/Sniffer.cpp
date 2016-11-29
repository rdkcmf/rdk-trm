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


#if TRM_SNIFFER_ENABLED
#warning "TRM_SNIFFER_ENABLED=TRUE"
#include <errno.h>
#include <stdio.h>
#include <vector>

#include <QObject>
#include <QString>

#include "trm/TRM.h"
#include "Util.h"

#include "Server.h"
#include "Sniffer.h"

extern void printThread(const char *func);

using namespace TRM;

void Sniffer::onMessageIn(const std::vector<uint8_t> in) 
{
    /* send to outAddr */
    unsigned char buf[4] = {0};
    buf[0] = ((in.size() & 0xFF000000) >> 24) & 0xFF;
    buf[1] = ((in.size() & 0x00FF0000) >> 16) & 0xFF;
    buf[2] = ((in.size() & 0x0000FF00) >>  8) & 0xFF;
    buf[3] = ((in.size() & 0x000000FF) >>  0) & 0xFF;
    printf("SNIFF: outgoing length %x %x %x %x\r\n", buf[0], buf[1], buf[2], buf[3]);
    outgoingSocket.writeDatagram((const char *)buf, 4, outAddr, outPort);
    outgoingSocket.writeDatagram((const char *)in.data(), in.size(), outAddr, outPort);
}

void Sniffer::onMessageOut(const std::vector<uint8_t> out) 
{
    /* send to outAddr */
    unsigned char buf[4] = {0};
    buf[0] = ((out.size() & 0xFF000000) >> 24) & 0xFF;
    buf[1] = ((out.size() & 0x00FF0000) >> 16) & 0xFF;
    buf[2] = ((out.size() & 0x0000FF00) >>  8) & 0xFF;
    buf[3] = ((out.size() & 0x000000FF) >>  0) & 0xFF;
    printf("SNIFF: outgoing length2 %x %x %x %x\r\n", buf[0], buf[1], buf[2], buf[3]);
    outgoingSocket.writeDatagram((const char *)buf, 4, outAddr, outPort);
    outgoingSocket.writeDatagram((const char *)out.data(), out.size(), outAddr, outPort);
}

/*
 * outAddr is in most cases a multicast address
 */
Sniffer::Sniffer(const Server &server, const QHostAddress &fromAddr, quint16 fromPort, const QHostAddress &outAddr, quint16 outPort )
: outAddr(outAddr), outPort(outPort)
{
    printf("Start sniffing at [%s]\r\n", fromAddr.toString().toUtf8().data());
    /* Multicast Implmentation */
    outgoingSocket.bind(fromAddr, fromPort);
    QObject::connect(&server, SIGNAL(messageIn (const std::vector<uint8_t>)), this, SLOT(onMessageIn (const std::vector<uint8_t>)));
    QObject::connect(&server, SIGNAL(messageOut(const std::vector<uint8_t>)), this, SLOT(onMessageOut(const std::vector<uint8_t>)));
}

Sniffer::~Sniffer(void)
{
    outgoingSocket.close();
}

#else
#warning "TRM_SNIFFER_ENABLED=FALSE"
#endif


/** @} */
/** @} */
