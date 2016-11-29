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
DIST_SUBDIRS = cfg
EXTRA_DIST = $(top_srcdir)/../include/trm/Activity.h \
             $(top_srcdir)/../include/trm/AsyncNotification.h \
             $(top_srcdir)/../include/trm/Enum.h \
             $(top_srcdir)/../include/trm/Header.h \
             $(top_srcdir)/../include/trm/JsonDecoder.h \
             $(top_srcdir)/../include/trm/JsonEncoder.h \
             $(top_srcdir)/../include/trm/Klass.h \
             $(top_srcdir)/../include/trm/MessageProcessor.h \
             $(top_srcdir)/../include/trm/Messages.h \
             $(top_srcdir)/../include/trm/ResponseStatus.h \
             $(top_srcdir)/../include/trm/TRM.h \
             $(top_srcdir)/../include/trm/Timer.h \
             $(top_srcdir)/../include/trm/TunerReservation.h \
             $(top_srcdir)/../include/trm/TunerState.h
lib_LTLIBRARIES = libtrm.la
libtrm_la_SOURCES = Activity.cpp Header.cpp JanssonParser.cpp \
                    JsonDecoder.cpp JsonEncoder.cpp  Klass.cpp  \
                    ResponseStatus.cpp Tuner.cpp TunerReservation.cpp \
                    TunerState.cpp  Util.cpp

libtrm_la_CXXFLAGS = -I$(top_srcdir)/../include -fpermissive
libtrm_la_LDFLAGS = -lrdkloggers -ljansson -luuid

libtrm_la_includedir = $(includedir)/trm
libtrm_la_include_HEADERS = $(EXTRA_DIST)

IPK_OUTDIR = $(abs_top_builddir)/ipk
IPK_STAGING_DIR=$(abs_top_builddir)/staging_dir
IPK_TMP_DIR=$(IPK_STAGING_DIR)/ipk

CONTROL_FILES = control postinst
IPK_FILES  = debian-binary control.tar.gz data.tar.gz
IPK_OUTPUT = libtrm0.ipk

package_ipk:
	@echo "Generating IPK Package"
	$(MAKE) prefix=$(IPK_STAGING_DIR) install
	$(mkdir_p) $(IPK_TMP_DIR)/usr/lib
	cp -af $(IPK_STAGING_DIR)/lib/*.so* $(IPK_TMP_DIR)/usr/lib/
	tar -czvf $(IPK_OUTDIR)/data.tar.gz -C $(IPK_TMP_DIR) usr
	tar -czvf $(IPK_OUTDIR)/control.tar.gz -C $(IPK_OUTDIR) $(CONTROL_FILES)
	pushd $(IPK_OUTDIR) && ar cr $(IPK_OUTPUT) $(IPK_FILES); popd
	rm -rf $(IPK_TMP_DIR)
	rm -f $(IPK_OUTDIR)/*.tar.gz

clean_package_ipk:
	rm -rf $(IPK_STAGING_DIR)
	rm -f $(IPK_OUTDIR)/*.tar.gz
	rm -f $(IPK_OUTDIR)/*.ipk
