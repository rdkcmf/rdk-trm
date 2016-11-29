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
INSTALL_DIR := $(COMBINED_ROOT)/trm/install/bin
CFLAGS      := -g -fPIC -D_REENTRANT -Wall
BINARIES    := fcgi-trm-proxy
OBJS        = fcgiproxy.o  

INCLUDE = -I.\
	 -I$(OPENSRC)/include  \
	 -I$(COMBINED_ROOT)/rdklogger/include  \


CFLAGS      += $(INCLUDE)

LDFLAGS	= -L$(OPENSRC)/lib -lfcgi -Wl,-rpath-link=$(COMBINED_ROOT)/sdk/fsroot/ramdisk/usr/local/lib/ -L$(COMBINED_ROOT)/rdklogger/build/lib -L$(COMBINED_ROOT)/sdk/fsroot/ramdisk/usr/lib  -lrdkloggers -llog4c -lrt

all: clean exe install
	@echo "Build Finished...."

exe: $(OBJS)
	@echo "Building $(BINARIES) ...."
	$(CC) $(OBJS) $(CFLAGS) $(LDFLAGS) -o $(BINARIES)

%.o:%.c
	@echo "Building $@ ...."
	$(CC) -c $<  $(CFLAGS) -o $@

clean:
	@echo "Cleaning the directory..."
	@$(RM) $(OBJS) $(BINARIES)

install:
	@echo "Copy the $(BINARIES)..."
	cp -f $(BINARIES) $(INSTALL_DIR)

