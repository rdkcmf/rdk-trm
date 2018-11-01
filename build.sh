#!/bin/bash
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
set -x
SCRIPT=$(readlink -f "$0")
SCRIPTS_DIR=`dirname "$SCRIPT"`
export COMBINED_ROOT=$SCRIPTS_DIR/../

#Build Log
buildReport=$COMBINED_ROOT/../Logs/buildTrmReport.txt

#Set the Path and Export Variables
export OPENSRC=$COMBINED_ROOT/opensource
export JANSSON_DIR=$COMBINED_ROOT/opensource/src/jansson/
export CC=$COMBINED_ROOT/sdk/toolchain/staging_dir/bin/i686-cm-linux-gcc
export CXX=$COMBINED_ROOT/sdk/toolchain/staging_dir/bin/i686-cm-linux-g++
export SDK_FSROOT=$COMBINED_ROOT/sdk/fsroot/ramdisk

#Install the libs and bin to FS ROOT
if [ "X$1" == "Xinstall" ]; then
mkdir -p $FSROOT/usr/local/lib
mkdir -p $FSROOT/usr/local/bin
cp -f install/lib/*.so $FSROOT/usr/local/lib
cp -f install/bin/* $FSROOT/usr/local/bin
exit 0
fi

#Creat Local install Directory
rm -rf ./install
mkdir -p ./install/bin
mkdir -p ./install/lib

#Build Trm Lib
echo "Building TRM Libs..."
make -C ./common/src/ all 


#Build Trm TRM App
echo "Building TRM Server..."
cd ./qtapp/srv
rm -rf Makefile
$SDK_FSROOT/usr/local/Qt/bin/qmake ./srv.pro
make clean
make
cd ../../  
cp -rf ./qtapp/srv/srv ./install/bin/trmsrv
echo "$PWD..."

echo "Building fcgi-trm-proxy..."
make -C ./proxy/ all

echo "Building websocket-trm-proxy..."
make -C ./wsproxy/ clean
make -C ./wsproxy/ all

#>> $buildReport 2>> $buildReport

if [ $? -ne 0 ] ; then
  echo "TRM  Build Failed..."
  exit 1
else
echo "TRM Build Success.."
mkdir -p $FSROOT/usr/local/lib
mkdir -p $FSROOT/usr/local/bin
cp -f install/lib/*.so $SDK_FSROOT/usr/local/lib 
cp -f install/bin/* $SDK_FSROOT/usr/local/bin
exit 0
fi


