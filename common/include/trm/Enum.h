/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2016 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/


/**
* @defgroup trm
* @{
* @defgroup common
* @{
**/


#ifndef TRM_ENUM_H_
#define TRM_ENUM_H_

#include <string>
#include <iostream>
#include <vector>
#include <limits>

#include "TRM.h"
TRM_BEGIN_NAMESPACE


template<class T>
class Enum {
public:
    typedef const char * CString;
    typedef typename T::EnumType eT;

    static const Enum<T> & at(eT t) {
    	//@TODO: use array indexing
    	typedef const std::vector<const Enum<T> * > CT;
    	CT & enums = T::getEnums();
    	for (typename CT::const_iterator it = enums.begin(); it != enums.end(); it++) {
    		if (((eT)t) == ((eT)(*it))) {
    			return *(*it);
    		}
    	}
    	return *(enums[0]);
    }

   static  const Enum<T> & at(const char * t)  {
	   //@TODO: use std::map RB tree
    	typedef const std::vector<const Enum<T> * > CT;
    	CT & enums = T::getEnums();
    	for (typename CT::const_iterator it = enums.begin(); it != enums.end(); it++) {
    		 if ((*it)->name.compare(t) == 0) {
    			return *(*it);
    		}
    	}
    	return *(enums[0]);
    }

   Enum(const eT &value, const char *name) : value(value), name(name) {}

    operator eT (void) const {
        return value;
    }

    operator CString(void) const {
        return name.c_str();
    }

    bool operator == (const Enum<T> &that) const {
    	return this->value == that.value;
    }

    bool operator != (const Enum<T> &that) const {
    	return this->value != that.value;
    }

    void print(void) const {
    	std::cout << "[OBJ]Enum<" << T::klassName() << ">" << "[k" << name << "] = " << value << std::endl;
    }
private:
    eT value;
    std::string name;
};


TRM_END_NAMESPACE
#endif



/** @} */
/** @} */
