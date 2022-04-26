##########################################################################
# If not stated otherwise in this file or this component's Licenses.txt
# file the following copyright and licenses apply:
#
# Copyright 2016 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
##########################################################################

######################################################################
# Automatically generated by qmake (3.0) Mon Jan 13 17:55:53 2014
######################################################################

TEMPLATE = app
TARGET = websocket-trm-proxy
USE_DELIA_GW=$$(USE_DELIA_GATEWAY)

greaterThan(QT_MAJOR_VERSION, 4) {
    isQtModuleAvailable(websockets): DEFINES += ENABLE_WEBSOCKET_SERVICE
}

contains(DEFINES, ENABLE_WEBSOCKET_SERVICE) {
    QT += websockets
}

equals(USE_DELIA_GW, yes){
	DEFINES += USE_DELIA_GATEWAY=$(USE_DELIA_GATEWAY)
}

QT += websockets
INCLUDEPATH += ../../../../opensource/src/qt/build/qtbase/mkspecs/devices/linux-platfrom-rdk-g++ 
INCLUDEPATH += ../../rdklogger/include
INCLUDEPATH += =/usr/include/wdmp-c
INCLUDEPATH += =/usr/include

!contains(DEFINES, USE_TRM_YOCTO_BUILD) {
LIBS += -L../../rdklogger/build/lib/
LIBS += -L${RDK_FSROOT_PATH}/usr/lib
}
LIBS += -lrdkloggers -llog4c
LIBS += -lrfcapi

contains(DEFINES, USE_SAFEC_API) {
CONFIG += link_pkgconfig
PKGCONFIG += libsafec
}

!contains(DEFINES, USE_SAFEC_API) {
QMAKE_CXXFLAGS += -DSAFEC_DUMMY_API
}

QT += network
QT -= gui    
QT -= widget    

# Input
HEADERS += qt_websocketproxy.h \
           tcpOpensslProxyServer.h

SOURCES += qt_websocketproxy.cpp \
           tcpOpensslProxyServer.cpp \
           main.cpp

LIBS += -lcrypto -lssl -lpthread
