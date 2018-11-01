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
#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([TRMMgr], [1.0], [shivajisarangi@tataelxsi.co.in])
AC_CONFIG_SRCDIR([TRMMgr.cpp])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([foreign])
# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
#AC_PROG_INSTALL

# Checks for libraries.
# FIXME: Replace `main' with a function in `-lIARMBus':
AC_CHECK_LIB([IARMBus], [IARM_Bus_Init])
# FIXME: Replace `main' with a function in `-ljansson':
#AC_CHECK_LIB([jansson], [main])
# FIXME: Replace `main' with a function in `-lpthread':
#AC_CHECK_LIB([pthread], [main])
# FIXME: Replace `main' with a function in `-ltrm':
#AC_CHECK_LIB([trm], [main])
# FIXME: Replace `main' with a function in `-luuid':
#AC_CHECK_LIB([uuid], [main])


# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h stddef.h stdint.h stdlib.h string.h sys/socket.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([gettimeofday localtime_r memset socket])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
