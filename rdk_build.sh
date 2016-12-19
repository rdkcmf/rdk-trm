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

#######################################
#
# Build Framework standard script for
#
# TRM component

# use -e to fail on any shell issue
# -e is the requirement from Build Framework
set -e


# default PATHs - use `man readlink` for more info
# the path to combined build
export RDK_PROJECT_ROOT_PATH=${RDK_PROJECT_ROOT_PATH-`readlink -m ..`}
export COMBINED_ROOT=$RDK_PROJECT_ROOT_PATH

# path to build script (this script)
export RDK_SCRIPTS_PATH=${RDK_SCRIPTS_PATH-`readlink -m $0 | xargs dirname`}

# path to components sources and target
export RDK_SOURCE_PATH=${RDK_SOURCE_PATH-`readlink -m .`}
export RDK_TARGET_PATH=${RDK_TARGET_PATH-$RDK_SOURCE_PATH}

# fsroot and toolchain (valid for all devices)
export RDK_FSROOT_PATH=${RDK_FSROOT_PATH-`readlink -m $RDK_PROJECT_ROOT_PATH/sdk/fsroot/ramdisk`}
export RDK_TOOLCHAIN_PATH=${RDK_TOOLCHAIN_PATH-`readlink -m $RDK_PROJECT_ROOT_PATH/sdk/toolchain/staging_dir`}


# default component name
export RDK_COMPONENT_NAME=${RDK_COMPONENT_NAME-`basename $RDK_SOURCE_PATH`}

# import some helper functions
if [[ "$RDK_PLATFORM_SOC" == "broadcom" ]] ; then
source $RDK_PROJECT_ROOT_PATH/opensource/qt/apps_helpers.sh
export BUILDS_DIR=$RDK_PROJECT_ROOT_PATH
else
source $RDK_PROJECT_ROOT_PATH/opensource/src/qt/apps_helpers.sh
fi

# parse arguments
INITIAL_ARGS=$@

function usage()
{
    set +x
    echo "Usage: `basename $0` [-h|--help] [-v|--verbose] [action]"
    echo "    -h    --help                  : this help"
    echo "    -v    --verbose               : verbose output"
    echo "    -p    --platform  =PLATFORM   : specify platform for TRM"
    echo
    echo "Supported actions:"
    echo "      configure, clean, build (DEFAULT), rebuild, install"
}

# options may be followed by one colon to indicate they have a required argument
if ! GETOPT=$(getopt -n "build.sh" -o hvp: -l help,verbose,platform: -- "$@")
then
    usage
    exit 1
fi

eval set -- "$GETOPT"

while true; do
  case "$1" in
    -h | --help ) usage; exit 0 ;;
    -v | --verbose ) set -x ;;
    -p | --platform ) CC_PLATFORM="$2" ; shift ;;
    -- ) shift; break;;
    * ) break;;
  esac
  shift
done

ARGS=$@


# component-specific vars
export FSROOT=${RDK_FSROOT_PATH}
export TOOLCHAIN_DIR=${RDK_TOOLCHAIN_PATH}
export OPENSRC=$RDK_PROJECT_ROOT_PATH/opensource

#Set the Path and Export Variables
if [ $RDK_PLATFORM_SOC = "intel" ]; then
export JANSSON_DIR=$RDK_PROJECT_ROOT_PATH/opensource/src/jansson/
export CC=$RDK_PROJECT_ROOT_PATH/sdk/toolchain/staging_dir/bin/i686-cm-linux-gcc
export CXX=$RDK_PROJECT_ROOT_PATH/sdk/toolchain/staging_dir/bin/i686-cm-linux-g++
export CROSS_TOOLCHAIN=$TOOLCHAIN_DIR
export GLIB_INCLUDE_PATH=$CROSS_TOOLCHAIN/include/glib-2.0/
export GLIB_LIBRARY_PATH=$CROSS_TOOLCHAIN/lib/
export GLIB_CONFIG_INCLUDE_PATH=$GLIB_LIBRARY_PATH/glib-2.0/
export GLIBS='-lglib-2.0 -lz -lgio-2.0 -lgthread-2.0 -lgobject-2.0 -lgmodule-2.0'
elif [ $RDK_PLATFORM_SOC = "broadcom" ]; then
echo "building for ${RDK_PLATFORM_DEVICE}..."
        export WORK_DIR=$BUILDS_DIR/work${RDK_PLATFORM_DEVICE^^}
        . $WORK_DIR/../build_scripts/setBCMenv.sh
CROSS_COMPILE=mipsel-linux
export CC=$CROSS_COMPILE-gcc
export CXX=$CROSS_COMPILE-g++
export JANSSON_DIR=$RDK_PROJECT_ROOT_PATH/opensource/jansson/
export GLIB_INCLUDE_PATH=$FSROOT/usr/local/include/glib-2.0/
export GLIB_LIBRARY_PATH=$FSROOT/usr/local/lib/
export GLIBS='-lglib-2.0 -lintl -lz -lgio-2.0 -lgthread-2.0 -lgobject-2.0 -lgmodule-2.0'
fi
# functional modules

function configure()
{
    true #use this function to perform any pre-build configuration
}

function clean()
{
    if [ $RDK_PLATFORM_SOC = "intel" ]; then
        if [ $RDK_PLATFORM_DEVICE = "xg1" ]; then
            rm -r $RDK_FSROOT_PATH/etc/ssl/trmserver/*
        fi
    fi
    true #use this function to provide instructions to clean workspace
}

function build()
{
    set_qmakespec
    cd ${RDK_SOURCE_PATH}
    TRM_NUMBER_OF_TUNERS=5
    if [ $RDK_PLATFORM_SOC = "intel" ]; then
       if [ $RDK_PLATFORM_DEVICE = "xg1" ]; then
           TRM_NUMBER_OF_TUNERS=5 #pace xg1
           export TRM_SNIFFER_ENABLED_=1
       elif [ $RDK_PLATFORM_DEVICE = "xg5" ]; then
           TRM_NUMBER_OF_TUNERS=6 #arris xg5
           export TRM_SNIFFER_ENABLED_=1
       fi
    elif [ $RDK_PLATFORM_SOC = "broadcom" ]; then
       if [ "$RDK_COMCAST_PLATFORM" = "XG2" ]; then
           TRM_NUMBER_OF_TUNERS=4 #pace and Samsung XG2v2
           export TRM_SNIFFER_ENABLED_=1
       elif [ $RDK_PLATFORM_DEVICE = "xg1" ]; then
           TRM_NUMBER_OF_TUNERS=6 #pace xg1v3 and samsung xg1v3
           export TRM_SNIFFER_ENABLED_=1
       elif [ $RDK_PLATFORM_DEVICE = "rng150" ]; then
           TRM_NUMBER_OF_TUNERS=1
           export TRM_SNIFFER_ENABLED_=0
       fi
    fi

    # Always build for HYBRID Gateway. No more ocap-ri
    export TRM_NUMBER_OF_TUNERS=$TRM_NUMBER_OF_TUNERS

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
	$RDK_FSROOT_PATH/usr/local/Qt/bin/qmake ./srv.pro
	make clean
	make
	cd ../../  
	cp -rf ./qtapp/srv/srv ./install/bin/trmsrv
	echo "$PWD..."
	
	echo "Building fcgi-trm-proxy..."
	make -C ./proxy/ all

	echo "Building websocket-trm-proxy..."
	cd ./wsproxy
	rm -rf Makefile
	$RDK_FSROOT_PATH/usr/local/Qt/bin/qmake ./wsproxy.pro
	make clean
	make
    cd ../
	cp -rf ./wsproxy/websocket-trm-proxy ./install/bin/websocket-trm-proxy

    if [ $TRM_SNIFFER_ENABLED_ -eq 1 ]; then
	echo "Building websocket-trm-sniffer..."
	cd ./sniffer
	rm -rf Makefile
	$RDK_FSROOT_PATH/usr/local/Qt/bin/qmake ./sniffer.pro
	make clean
	make
    cd ../
	cp -rf ./sniffer/websocket-trm-sniffer ./install/bin/websocket-trm-sniffer
    else
	echo "Not Building websocket-trm-sniffer..."
    fi

   echo "Building TRM Diag Manager..."
   make -C ./diag all


    mkdir -p $RDK_FSROOT_PATH/usr/local/bin
	mkdir -p $RDK_FSROOT_PATH/usr/local/lib
}

function rebuild()
{
    clean
    build
}

function install()
{
    cd ${RDK_SOURCE_PATH}
    if [ $RDK_PLATFORM_SOC = "intel" ]; then
        if [ $RDK_PLATFORM_DEVICE = "xg1" ]; then
            mkdir -p $RDK_FSROOT_PATH/etc/ssl/trmserver/
            cp wsproxy/sslcerts/* $RDK_FSROOT_PATH/etc/ssl/trmserver/
       fi
    fi
    cp -f install/lib/*.so $RDK_FSROOT_PATH/usr/local/lib 
	cp -f install/bin/* $RDK_FSROOT_PATH/usr/local/bin

}


# run the logic

#these args are what left untouched after parse_args
HIT=false

for i in "$ARGS"; do
    case $i in
        configure)  HIT=true; configure ;;
        clean)      HIT=true; clean ;;
        build)      HIT=true; build ;;
        rebuild)    HIT=true; rebuild ;;
        install)    HIT=true; install ;;
        *)
            #skip unknown
        ;;
    esac
done

# if not HIT do build by default
if ! $HIT; then
  build
fi
