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
RM          := rm -rf 
INSTALL_DIR := $(COMBINED_ROOT)/trm/install/lib
CFLAGS      := -g -fPIC -D_REENTRANT -Wall 
LIBNAME     := trm
LIBNAMEFULL := lib$(LIBNAME).so
OBJS        = Activity.o Header.o JanssonParser.o JsonDecoder.o JsonEncoder.o  
OBJS       += Klass.o  ResponseStatus.o Tuner.o TunerReservation.o TunerState.o  Util.o


INCLUDE = -I.\
	 -I../include	\
	 -I$(JANSSON_DIR)/src \
	 -I$(OPENSRC)/include/ \
	 -I$(COMBINED_ROOT)/rdklogger/include/ \
	 -I$(OPENSRC)/include/uuid/
	

CFLAGS      += $(INCLUDE)
CFLAGS      += -DTRM_VERSION_MAJOR=2 -DTRM_VERSION_MINOR=3

LDFLAGS +=  -L$(RDK_FSROOT_PATH)/usr/lib -lrdkloggers

all: clean library install
	@echo "Build Finished...."

library: $(OBJS)
	@echo "Building $(LIBNAMEFULL) ...."
	$(CXX) $(OBJS) $(CFLAGS) $(LDFLAGS) -shared -Wl,-rpath $(COMBINED_ROOT)/sdk/fsroot/ramdisk/usr/local/lib/ -o $(LIBNAMEFULL)

%.o:%.cpp
	@echo "Building $@ ...."
	$(CXX) -c $<  $(CFLAGS) -o $@

clean:
	@echo "Cleaning the directory..."
	@$(RM) $(OBJS) $(LIBNAMEFULL)

install:
	@echo "Copy the $(LIBNAMEFULL)..."
	cp lib$(LIBNAME).so $(INSTALL_DIR)
