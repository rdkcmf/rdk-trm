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
export JASSON_PATH=/home/hongli/Projects/transformer/jansson/jansson-2.4/
#export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/hongli/Projects/transformer/jansson/jansson-2.4/src/.libs
rm -rf a.out
g++ -g *.cpp -I./ -I../include/ -I$JASSON_PATH/src/ -L$JASSON_PATH/src/.libs/ -ljansson -Wnon-template-friend
