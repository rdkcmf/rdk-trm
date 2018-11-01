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

SUBDIRS = 
DIST_SUBDIRS =

AM_LDFLAGS =  -ltrm -lpthread -ljansson -luuid -lIARMBus

EXTRA_DIST = $(top_srcdir)/include/TRMMgr.h

INCLUDE_FILES = -I.\
         -I$(PKG_CONFIG_SYSROOT_DIR)/usr/include/uuid\
         -I$(PKG_CONFIG_SYSROOT_DIR)/usr/include/rdk/iarmbus
CC = $(CXX)

AM_CPPFLAGS = $(INCLUDE_FILES)
AM_CPPFLAGS += -DNUM_OF_TUNERS=$(TRM_NUMBER_OF_TUNERS)

bin_PROGRAMS = TRMMgr
TRMMgr_SOURCES = TRMMgr.cpp Helper.cpp

TRMMgr_CPPFLAGS = $(AM_CPPFLAGS) -I $(top_srcdir)/include


IPK_OUTDIR = $(abs_top_builddir)/ipk
IPK_STAGING_DIR = $(abs_top_builddir)/staging_dir
IPK_TMP_DIR = $(IPK_STAGING_DIR)/ipk

CONTROL_FILES = control postinst
IPK_FILES  = debian-binary control.tar.gz data.tar.gz
IPK_OUTPUT = trmmgr.ipk

package_ipk:
	@echo "Generating IPK Package ..."
	$(MAKE) prefix=$(IPK_STAGING_DIR) install
	$(mkdir_p) $(IPK_TMP_DIR)/usr/bin
	$(mkdir_p) $(IPK_TMP_DIR)/lib/systemd/system/
	cp -af $(abs_top_builddir)/conf/trmmgr.service $(IPK_TMP_DIR)/lib/systemd/system/
	cp -af $(IPK_STAGING_DIR)/bin/TRMMgr $(IPK_TMP_DIR)/usr/bin
	tar -czvf $(IPK_OUTDIR)/data.tar.gz -C $(IPK_TMP_DIR) usr lib
	tar -czvf $(IPK_OUTDIR)/control.tar.gz -C $(IPK_OUTDIR) $(CONTROL_FILES)
	pushd $(IPK_OUTDIR) && ar cr $(IPK_OUTPUT) $(IPK_FILES); popd
