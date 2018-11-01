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


#include <string>
#include <iostream>

#include "trm/TRM.h"
#include "trm/TunerReservation.h"
#include "ReservationCustomAttributes.h"

#include "Util.h"

using namespace TRM;

TunerReservationBase::TunerReservationBase(
				 const std::string &device,
				 const std::string &serviceLocator,
				 const uint64_t     startTime,
				 const uint64_t     duration,
				 const Activity    &activity,
				 const std::string &reservationToken,
				 ReservationCustomAttributes *customAttributes)
: reservationToken(reservationToken),
  device(device),
  serviceLocator(serviceLocator),
  startTime(startTime),
  duration(duration),
  activity(activity),
  customAttributes(customAttributes)
{
	state = IDLE;
};

TunerReservationBase::~TunerReservationBase(void)
{
}

/**
 * @brief This function is used to set a unique token generated when a reservation
 * is created. After a reservation is created, this token is used in all messages to uniquely
 * identify an existing reservation within TRM.
 *
 * @param[in] token A unique token that the requesting device can use to make the remote tuning request.
 * @return None.
 */
void TunerReservationBase::setReservationToken(const std::string &token)
{
	reservationToken = token;
}


/**
 * @brief This function is used to return the unique token generated when a reservation
 * is created.
 *
 * @return Returns a token that the requesting device can use to make the remote tuning request.
 */
const std::string & TunerReservationBase::getReservationToken(void) const
{
	return reservationToken;
}


/**
 * @brief This function is used to return the locator of the service that the tuner is tuned to.
 *
 * @return Returns a service locator in string format.
 */
const std::string & TunerReservationBase::getServiceLocator(void) const
{
	return serviceLocator;
};


/**
 * @brief This function is used to get the start time of the reservation in milliseconds from the epoch.
 *
 * @return Returns a time in terms of milliseconds.
 */
uint64_t TunerReservationBase::getStartTime(void) const
{
	return startTime;
}


/**
 * @brief This function is used to get the reservation period measured from the start in milliseconds.
 *
 * @return Returns a time in terms of millisecond.
 */
uint64_t TunerReservationBase::getDuration(void) const
{
	return duration;
}


/**
 * @brief This function is used to get the remote device id requesting for tuner reservation.
 *
 * @return Returns the device Id in string format.
 */
const std::string  &TunerReservationBase::getDevice(void) const
{
	return device;
};


/**
 * @brief This function is used to return the granted @b activity. Granted @b activity may or may not
 * be the same as the requested. In the later case the owner of the reservation will need to comply with
 * the returned activity, or initiate conflict resolution.
 * @n For example, a client requests for @b Live activity when EAS is in progress, the returned
 * reservation will have the EAS activity.
 *
 * @return Returns a pointer to an object of class Activity.
 */
const Activity & TunerReservationBase::getActivity(void) const
{
	return activity;
}


/**
 * @brief This function is used to set the locator of the service that the tuner will tune to.
 * The service locator is a URL containing tune parameters of the remote device.
 *
 * @param[in] serviceLocator Locator of the service that the tuner will tune to.
 * @return None.
 */
void TunerReservationBase::setServiceLocator(const std::string &_serviceLocator)
{
	serviceLocator = _serviceLocator;
};


/**
 * @brief This function is used to set the start time of the reservation in milliseconds from the epoch.
 * If the @b startTime not present a requested message, this is set to when the reservation is granted or renewed.
 * @note The @b startTime is always included in a reservation response message.
 *
 * @param[in] startTime Time in milliseconds from the epoch.
 * @return None.
 */
void TunerReservationBase::setStartTime(const uint64_t &startTime)
{
	this->startTime = startTime;
}


/**
 * @brief This function is used to set the reservation duration measured from the start in milliseconds.
 * If the duration field not present in a request message, the token is valid for a default duration.
 * @note The @b duration is always included in a reservation response message.
 *
 * @param[in] duration A time period measured from the start in milliseconds.
 * @return None.
 */
void TunerReservationBase::setDuration(const uint64_t &duration)
{
	this->duration = duration;
}


/**
 * @brief This function is used to return remaining tuner reservation time.
 *
 * @return Returns a positive value means the reservation time is not yet expired.
 * Returns 0 when tuner reservation time has expired.
 */
uint64_t TunerReservationBase::getExpirationTime(void) const
{
	/*
	 * Expiration = (Start - Current) + Duration;
	 */
	int64_t currentEpochMillis = GetCurrentEpoch();

	int64_t expirationTime =  static_cast<int64_t>((((int64_t)startTime) - ((int64_t)currentEpochMillis)) + ((int64_t)duration));
	if (expirationTime < 0) expirationTime = 0;

	return expirationTime;
}

/**
 * @brief This function is used to add the details describing the tuner reservation activity in to the activity list.
 *
 * The field specified here are required for the associated activity. The requestor is
 * allowed to insert unspecified fields in the details. These unspecified fields are ignored by TRM,
 * and echoed back in response message that have the activity field.
 * @n The defined fields are:
 * - recordingId : required when requesting Record activity for a tuner.
 * - hot : flag (true or false) indicating of the recording is scheduled or hot.
 *
 * @param[in] key it could be "recordingId" or "hot" when requesting Record activity for a tuner.
 * @param[in] value value associated for key.
 * @return None.
 */
void TunerReservationBase::addDetail(const std::string &key, const std::string &value)
{
	activity.addDetail(key, value);
}


/**
 * @brief This function is used to return the custom attributes assigned by the application. These attributes are
 * associated with the reservation token for the lifetime of the reservation.
 *
 * @return Returns a pointer to an object of class ReservationCustomAttributes.
 */
const ReservationCustomAttributes *TunerReservationBase::getCustomAttributes(void) const
{
	return customAttributes;
}


/**
 * @brief This function is used to set the attributes assigned by the application. These attributes are
 * associated with the reservation token for the lifetime of the reservation.
 *
 * The @b customAttributes contains JSON entities defined by the application. It is sent by the application
 * when it requests for a tuner reservation. After the reservation is granted, and as long as the content
 * of the reservation is not modified, the @b customAttributes will be present in any messages, including
 * the asynchronous notifications, that contains the corresponding TunerReservation.
 * @n If a renewd TunerReservation also contains @b customAttributes, the new attributes object will take
 * the place.
 * @see notifyTunerReservationUpdate()
 *
 * @param[in] customAttributes A set of attributes assigned by the application.
 * @return None.
 */
void TunerReservationBase::setCustomAttributes(ReservationCustomAttributes *customAttributes)
{
	this->customAttributes = customAttributes;
}


/**
 * @brief This function is used to print the following attributes of tunerReservation token.
 * - @b reservationToken = A unique token generated when a reservation is created.
 * - @b device = The remote device requesting the reservation.
 * - @b serviceLocator = The service locator that the tuner will tune to.
 * - @b duration = The reservation period measured from the start in milliseconds.
 *
 * @return None.
 */
void TunerReservationBase::print(void) const
{
	std::cout << "[OBJ][" << klassName() << "] reservationToken = "<< reservationToken << std::endl;
	std::cout << "[OBJ][" << klassName() << "] device = " 			<< device 			<< std::endl;
	std::cout << "[OBJ][" << klassName() << "] serviceLocator = " 	<< serviceLocator 	<< std::endl;
	std::cout << "[OBJ][" << klassName() << "] startTime = " 		<< startTime 		<< std::endl;
	std::cout << "[OBJ][" << klassName() << "] duration = " 		<< duration 		<< std::endl;
	activity.print();
}


/** @} */
/** @} */
