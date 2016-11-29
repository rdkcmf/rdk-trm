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


#include <iostream>
#include <cstdio>

#include <QFile>
#include <QCoreApplication>
#include <QNetworkInterface>
#include <QHostAddress>

#include "trm/TRM.h"
#include "Server.h"
#include "Sniffer.h"
#include "rdk_debug.h"

using namespace TRM;

Server *serverInstance = 0;

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QStringList args = app.arguments();
    char *debugConfigFile = NULL;	

    int portNumber= -1;
    QString hostAddressStr;
    QString sniffInterfaceStr;
    bool sniffEnabled = false;

    /*Delia 5860 - Line Buffering*/
    setvbuf(stdout, NULL, _IOLBF, 0);

    QRegExp optionPort("-port");
    QRegExp optionIP("-ip");
    QRegExp optionSniff("-sniff");
    QRegExp optionDebugFile("-debugconfig");

    for (int i = 1; i < args.size(); ++i) {
        if (optionPort.indexIn(args.at(i)) != -1 ) {   
            portNumber= args.at(i+1).toInt();
            ++i;
            qDebug() << "port is " << portNumber;
        }
        else if (optionIP.indexIn(args.at(i)) != -1 ) {   
            hostAddressStr = args.at(i+1);
            ++i;
            qDebug() << "ip is " << hostAddressStr;
        } 
        else if (optionSniff.indexIn(args.at(i)) != -1 ) {   
            sniffInterfaceStr = args.at(i+1);
            ++i;
            qDebug() << "sniff is " << sniffInterfaceStr;
        } 
        else if (optionDebugFile.indexIn(args.at(i)) != -1 ) {   
	    debugConfigFile = argv[i+1];
	    ++i;
            qDebug() << "rdklogger debug file is " << debugConfigFile;

        }
    }

    rdk_logger_init(debugConfigFile);

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TRM", "This is a test line");
    
    if (portNumber < 0 || portNumber > 65535 || hostAddressStr.isEmpty()) {
    	printf("\r\n%s: <ip address> <port number> [sniff]\r\n", args.at(0).toUtf8().data());
    	exit(0);
    }

    QHostAddress hostAddress(hostAddressStr);
    QNetworkInterface sniffInterface;
    if (!sniffInterfaceStr.isEmpty()) {
        sniffInterface = QNetworkInterface::interfaceFromName(sniffInterfaceStr);
        if (sniffInterface.isValid()) {
            sniffEnabled = true;
        }
        else {
            printf("Sniff Interface Invalid, Sniff is not enabled\r\n");
        }
    }

    Server server(hostAddress, portNumber, /*start=*/!sniffEnabled, /*sniff=*/sniffEnabled);
    serverInstance = &server;

    if (!sniffEnabled) return app.exec();

#if TRM_SNIFFER_ENABLED
    Sniffer sniffer(server, sniffInterface.addressEntries().first().ip(), 0, QHostAddress("230.255.255.230"), 9986);
#endif

    if (sniffEnabled) {
        server.start();
    }

    return app.exec();
}


/** @} */
/** @} */
