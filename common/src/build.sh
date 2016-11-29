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
rm -rf a.out
rm -rf libtrm.so
rm -rf libtrm.so.1
rm -rf *.o

export JANSSON_DIR=/home/hongli/Projects/transformer/jansson/jansson-2.4

g++ -fPIC -g -c -I../include -I$JANSSON_DIR/src  -L$JANSSON_DIR/src/.libs -ljansson Activity.cpp ResponseStatus.cpp  TunerReservation.cpp Klass.cpp Json.cpp JsonParser.cpp TunerState.cpp
g++ -L$JANSSON_DIR/src/.libs -shared -Wl,-soname,libtrm.so.1 -o libtrm.so.1.0.1 Activity.o ResponseStatus.o  TunerReservation.o  Klass.o  Json.o JsonParser.o TunerState.o -ljansson
ln -s libtrm.so.1.0.1 libtrm.so
ln -s libtrm.so.1.0.1 libtrm.so.1
#g++ main.cpp -Wl,-rpath,. -I../include -I$JANSSON_DIR/src -L./ -L$JANSSON_DIR/src/.libs -ljansson -ltrm
g++ main.cpp -I../include -I$JANSSON_DIR/src -L./ -L$JANSSON_DIR/src/.libs -ltrm -ljansson

