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
 * @defgroup TRM_RESERVATION Tuner Reservation
 * @ingroup TRM_MAIN
 * @par Tuner Reservation
 * The TunerReservation object represents a requested or granted tuner reservation.
 * The reservation has a validity window that is indicated by startTime and duration.
 * The requesting device is required to renew a reservation before its validity window disappears.
 * If it is not renewed, the token will be released by TRM and all messages that follow bearing the
 * token will be considered as MalformedRequest . For Record reservations, the requested startTime
 * should be N seconds ahead of the actual recording start time
 * (or should be left out so that the granted reservation starts at the time when it is granted by TRM),
 * to give room in case a conflict needs to be resolved.
 * @n @n
 * A same reservationToken can be reused if and only if values of {device, activity} are the same.
 * In the case, the {serviceLocator, startTime, duration} of the reused token can be updated.
 * This is useful during a channel change, where the TRM client can reuse a same Live tuner reservation by
 * just updating the serviceLocator of it.
 * @n @n
 * A reservation is renewed by requesting tunerRreservation with a same reservationToken.
 * @n
 * @code
 * TunerReservation :=
 * {
 *     "reservationToken" (optional) : [String] reservationToken,
 *     "device"                      : [String] device,
 *     "serviceLocator"              : [String] sourceLocator,
 *     "startTime" (optional)        : [long] startTime,
 *     "duration" (optional)         : [long] duration
 *     "activity"                    : <Activity>
 *     "customAttributes" (optional) : <CustomAttributes>
 * }
 * @endcode
 * @n
 * - @b token: a unique token generated when a reservation is created.
 * After a reservation is created, this token is used in all messages to uniquely identify an existing reservation within TRM.
 * - @b device: the remote device requesting the reservation. For a hot recording, this
 * should be the receiverId of the originating device.
 * - @b serviceLocator: locator of the service that the tuner will tune to.
 * - @b startTime:start time of the reservation in milliseconds from the epoch. If not present in a request message,
 * this is set to when the reservation is granted or renewed. startTime is always included in a response message.
 * - @b duration: the reservation period measured from the start in milliseconds.
 *  If not present in a request message, the token is valid for a default duration. duration is always included in a response message.
 * - @b activity: the granted activity. Granted activity may or may not be the same as the requested.
 * In the latter case the owner of the reservation will need to comply with the returned activity,
 * or initiate conflict resolution.
 * @n
 * For example, when a client requests Live activity when EAS is in progress,
 * the returned reservation will have the EAS activity.
 * - @b customAttributes: a set of attributes assigned by the application.
 * These attributes are associated with the reservation token for the lifetime of the reservation.
 *
 * @par Response Status
 * All response messages from TRM will provide information regarding the status of the response.
 * Responses to recognized requests may contain additional information, as described in later sections of this document.
 * Responses to unrecognized requests will contain only this status data, consisting of a status code
 * and message signifying the request was unrecognized.
 * @n @n
 * @code
 * ResponseStatus := {
 *    "status"                   : [String] status,
 *    "statusMessage" [optional] : [String] statusMessage
 * }
 * @endcode
 * @n
 * Where the fields are defined as follows:
 * @n @b status: is an enumeration of strings indicating the status of the request.
 * - @b Ok Request was successful.
 * - @b GeneralError Request was unsuccessful.
 * - @b MalFormedRequest Unexpected/Invalid request data.
 * - @b UnRecognizedRequest Unrecognized request.
 * - @b InsufficientResource: there is no tuner available.
 * - @b UserCancellation: Token is released as result of user cancellation.
 * - @b InvalidToken: Token included in the message is invalid.
 * - @b InvalidState: Token is in invalid state.
 * - @b statusMessage: is a string containing additional information about the status.
 *
 * @defgroup TRM_RESERVATION_CLASSES Tuner Reservation Classes
 * @ingroup TRM_RESERVATION
 * Described the details about Tuner Reservation Specifications.
 */

/**
* @defgroup trm
* @{
* @defgroup common
* @{
**/


#ifndef TRM_TUNER_RESERVATION_H_
#define TRM_TUNER_RESERVATION_H_

#include <stdint.h>

#include <string>
#include <iostream>
#include <list>

#include "TRM.h"
#include "Activity.h"
#include "Klass.h"

namespace TRM {

class ReservationCustomAttributes;
/**
 * @brief The TunerReservation class is used to set the requested or granted tuner reservation from client.
 * The reservation has a validity window that is indicated by startTime and duration.
 * The requesting device is required to renew a reservation before its validity window disappears.
 * If it is not renewed, the token will be released by TRM and all messages that follow bearing the
 * token will be considered as MalformedRequest. For Record reservations, the requested startTime should
 * be N seconds ahead of the actual recording start time (or should be left out so that the granted reservation
 * starts at the time when it is granted by TRM), to give room in case a conflict needs to be resolved.
 * @n @n A same reservationToken can be reused if and only if values of {device, activity} are the same.
 * In the case, the {serviceLocator, startTime, duration} of the reused token can be updated.
 * This is useful during a channel change, where the TRM client can reuse a same @b Live tuner reservation
 * by just updating the serviceLocator of it.
 * @ingroup TRM_RESERVATION_CLASSES
 */
class TunerReservationBase
{
public:
	typedef std::list<std::string> TokenList;
	enum {
		IDLE = 0,
		ACTIVE,
		EXPIRED,
	};

	static const char *klassName(void) { return Klass::kTunerReservation; }

	const std::string  & getReservationToken(void) 	const;
	const std::string  & getDevice(void) 			const;
	const std::string  & getServiceLocator(void) 	const;
	uint64_t       		 getStartTime(void) 		const;
	uint64_t       		 getDuration(void) 			const;
	const Activity     & getActivity(void) 		    const;

	void setReservationToken(const std::string &token);
	void setServiceLocator(const std::string &_serviceLocator);
	void setStartTime(const uint64_t &_startTime);
	void setDuration(const uint64_t &_duratioin);
	uint64_t getExpirationTime(void) const;
	void setActivity(const Activity &activity);
	void addDetail(const std::string &key, const std::string &value);
	const ReservationCustomAttributes *getCustomAttributes(void) const;
	void setCustomAttributes(ReservationCustomAttributes *);
	virtual void print(void) const;


public:
	TunerReservationBase()
	: reservationToken(""), device(""), serviceLocator(""),
	  startTime(0), duration(0),
	  activity(Activity::kNone),
	  customAttributes(0),state(0) {};  //CID:18524-Initialize  state

	TunerReservationBase(const std::string &device,
	         	 	     const std::string &serviceLocator,
					     const uint64_t    startTime,
			             const uint64_t    duration,
			             const Activity    &activity,
			             const std::string &reservationToken = "",
			             ReservationCustomAttributes *customAttributes = 0);

	virtual ~TunerReservationBase(void);

private:
	std::string 	reservationToken;
	std::string 	device;
	std::string 	serviceLocator;
	//@TODO: should startTime defaults to "Now" and duration to "Infinity"?
	uint64_t 		startTime;
	uint64_t 		duration;
	Activity 	    activity;
	//@TODO: safe copy constructor
	ReservationCustomAttributes* customAttributes;
public:
	int state;
};

typedef TunerReservationBase TunerReservation;

}
#endif


/** @} */
/** @} */
