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


#include <QApplication>
#include <QDialog>
#include <QPainter>
#include <QDebug>
#include "TRMMonitorGUI.h"

using namespace TRM;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QStringList args = app.arguments();

    int portNumber= -1;
    QString hostAddressStr;
    QString clientId;
    QString barWidth;

    QRegExp optionPort("-port");
    QRegExp optionIP("-ip");
    QRegExp optionClientId("-clientId");
    QRegExp optionBarWidth("-barWidth");


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
        else if (optionClientId.indexIn(args.at(i)) != -1 ) {
            clientId = args.at(i+1);
            ++i;
            qDebug() << "clientId is " << clientId;
        }
        else if (optionBarWidth.indexIn(args.at(i)) != -1 ) {
        	barWidth = args.at(i+1);
            ++i;
            qDebug() << "barWidth is " << barWidth;
        }
        else {
        }
    }

    if (portNumber < 0 || portNumber > 65535 || hostAddressStr.isEmpty()) {
    	printf("\r\n%s: <ip address> <port number> <clientName> <barWidth> \r\n", args.at(0).toUtf8().data());
    	exit(0);
    }

    if (clientId.isEmpty()) clientId = "FFFFF001";
    if (barWidth.isEmpty()) barWidth = "3600";

	TRMMonitorGUI monitorGUI(QHostAddress(hostAddressStr), portNumber, clientId, barWidth);
	monitorGUI.show();
	return app.exec();
}


/** @} */
/** @} */
