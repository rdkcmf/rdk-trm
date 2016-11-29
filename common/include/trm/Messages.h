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
 * @file Messages.h
 */

/**
 * @defgroup TRM_MESSAGE TRM Messages
 * @ingroup TRM_MAIN
 * Certain information will be shared among messages, as described below.
 *
 * @par Custom Attributes
 * @n
 * Tuner State object represents state of the tuner.
 * @n
 * @code
 * CustomAttributes :=
 * {
 *       // JSON Entities defined by application
 * }
 * @endcode
 * @n
 * The CustomAttribute contains JSON Entities defined by the application. It is sent by the application when it requests for a
 * tuner reservation. After the reservation is granted, and as long as the content of the reservation is not modified, the customAttributes
 * will be present in any messages, including the asynchronous notifications, that contains the corresponding TunerRevervation.
 * If a renewed TunerReservation also contains CustomAttribute, the new attributes object will take the place.
 * See more on this in @b NotifyTunerReservationUpdate message.
 * @n
 * The lifetime of the object is then same as the lifetime of the @b TunerReversation itself.
 *
 * @defgroup TRM_GENERAL_MESSAGE TRM General Messages
 * The messages will follow the request-response message exchange pattern.
 * The general format of the request payload is:
 * @code
 * {
 *     RequestName : {
 *          "requestId" : [String] requestId,
 *          "device" : [String] device,
 *     }
 * }
 * @endcode
 * The fields are defined as follows:
 * - @b RequestName: the name of the request, it will vary for the operations.
 * This generally should be the device where the request message originates.
 * - @b requestId: a GUID used to match requests with responses.
 * For every request, the client supplies a requestId that it can use to match the corresponding response.
 * For every notification, the sender supplies a requestId.
 * - @b device: the remote device making the request.
 *
 * The general format of the response payload is
 *
 * @code
 * {
 *      response: {
 *          "requestId" : [String] requestId,
 *          <ResponseStatus>
 *      }
 * }
 * @endcode
 *
 * The fields are defined as follows:
 *
 * - @b requestId: matches the response to a request.
 *
 * @ingroup TRM_MESSAGE
 *
 * @defgroup TRM_NOTIFY_MSG TRM Notification Messages
 * Notifications are asynchronous messages sent by TRM to token owners with regard to status update of an existing reservation.
 * @ingroup TRM_MESSAGE
 *
 * @defgroup TRM_UTILITY_MSG TRM Utility Messages
 * Utility Messages provide useful information about the status of the TRM.
 * @ingroup TRM_MESSAGE
 */

/**
 * @defgroup getVersion Get Version Number
 * @ingroup TRM_UTILITY_MSG
 *
 * @par Get Version Number Message
 * @n
 * To get the TRM Server Version Number.
 * @n
 * TRM clients query for the server Version Number. 
 * The TRM server and client shall maintain backward compatibility.
 * @n
 * The message format:
 * @n
 * @code
 * {
 *     "getVersion" : {
 *          "requestId" : [String] requestId,
 *     }
 * }
 * @endcode
 * @n
 * The details about the fields,
 * @n
 * - @b requestId : a GUID used to match requests with responses.
 * For every request, the client supplies a requestId corresponding to the response.
 *
 * @par Get Version Number Response Message
 * @n
 * The Message Format,
 * @n
 * @code
 * {
 *      getVersionResponse: {
 *          "requestId"       : [String] requestId,
 *          <ResponseStatus>
 *          "version"         : [String] version number
 *      }
 * }
 * @endcode
 * @n
 * - @b requestId : a GUID used to match requests with responses.
 * For every request, the client supplies a requestId corresponding to the response.
 * - @b version : Version number for which protocol is defined.
 *
 * @defgroup getAllTunerIds Get All Tuner Id's
 * @ingroup TRM_UTILITY_MSG
 *
 * @par Get All Tuner Id's Message
 * Requests for the unique ID that system has assigned to each tuner.
 * The ID for each tuner is guaranteed to be unique within a same target host.
 * @n
 * The Message format,
 * @n
 * @code
 * {
 *     "getAllTunerIds" : {
 *           "requestId" : [String] requestId,
 *     }
 * }
 * @endcode
 * @n
 * - @b requestId : a GUID used to match requests with responses.
 * For every request, the client supplies a requestId corresponding to the response.
 *
 * @par Get All Tuner Id's Response Message
 * @n
 * Response for the unique ID that system has assigned to each tuner.
 * The ID for each tuner is guaranteed to be unique within a same target host.
 * @n
 * The message format,
 * @n
 * @code
 * {
 *      "getAllTunerIdsResponse" : {
 *          "requestId"          : [String] requestId,
 *          <ResponseStatus>
 *          "tunerIds.           : [String,...]
 *      }
 * }
 * @endcode
 * @n
 * - @b requestId : a GUID used to match requests with responses.
 * For every request, the client supplies a requestId corresponding to the response.
 * - @b tunerIds: An array of String. Each element is an ID of a tuner.
 *
 * @defgroup getAllTunerStates Get All Tuner States
 * @ingroup TRM_UTILITY_MSG
 *
 * @par Get All Tuner States Message
 * @n
 * Requests for the State of all tuners on the system.
 * Each tuner can be in any of the following state specified in 0.
 * @n
 * The message format,
 * @n
 * @code
 * {
 *      "getAllTunerStates" : {
 *           "requestId"    : [String] requestId,
 *      }
 * }
 * @endcode
 * @n
 * - @b requestId : a GUID used to match requests with responses.
 * For every request, the client supplies a requestId corresponding to the response.
 *
 * @par Get All Tuner States Response Message
 * @n
 * Requests for all tuner reservations that are valid at the time of the request.
 * @n
 * The message format,
 * @n
 * @code
 * {
 *       "getAllTunerStatesResponse" : {
 *           "requestId"             : [String] requestId,
 *           <ResponseStatus>
 *           "allStates"             : <AllTunerStates>
 *           "detailedStates"        : <AllTunerDetailedStates>
 *       }
 * }
 * @endcode
 * @n
 * - @b tunerId: A string returned from getAllTunerIdsResponse.
 * - @b AllTunerStates: An enumeration of tuner states at time of request.
 * This is deprecated and replaced by "AllTunerDetailedStates" since 1.0.5.
 * For backward compatibility, the 1.0.5 or newer TRM client should ignore this field if it is present in the response message.
 *- @b AllTunerDetailedStates: An enumeration of detailed tuner states at time of request.
 *
 * @defgroup getAllReservations Get All Reservation
 * @ingroup TRM_UTILITY_MSG
 *
 * @par Get All Reservation Message
 * @n
 * Requests for all tuner reservations that are valid at the time of the request.
 * @n
 * @code
 * Filter :=
 * {
 *       "device" (optional)     : [String] device,
 *       "activity" (optional)   : <Activity>,
 *       "tunerState" (optional) : <State>,
 * }
 * @endcode
 * @n
 * @code
 * {
 *     "getAllReservations" : {
 *         "requestId" : [String] requestId,
 *         "filters" : [<Filter>,...]
 *     }
 * }
 * @endcode
 * @n
 * The fields are defined as follows:
 * @n
 * - @b filters: An array of filtering values, each filter can contain any number of conditions.
 * Valid condition include device, Activity, State. If a filter has multiple conditions,
 * these conditioned are AND.d to generate the final result. If there are multiple filters,
 * these filters are OR.d to generate the final result.
 *
 * @par Get All Reservation Response Message
 * @n
 * @b getAllReservationsResponse requests for all tuner reservations that are valid at the time of the request.
 * @n
 * @code
 * AllTunerReservations := {
 *     <tunerId> : [<TunerReservation>, ...]
 * }
 * @endcode
 * @n
 * @code
 * {
 *      "getAllReservationsResponse" : {
 *          "requestId"              : [String] requestId,
 *          "allTunerReservations"   : <AllTunerReservations> ...
 *      }
 * }
 * @endcode
 * @n
 * The fields are defined as follows,
 * @n
 * - @b tunerId: A string returned from getAllTunerIdsResponse.
 * Each tunerId maps to an array of tunerReservation currently active on the tuner.
 */

/**
 * @defgroup notifyTunerReservationConflict Notify Tuner Reservation Conflict
 * @ingroup TRM_NOTIFY_MSG
 *
 * @b notifyTunerReservationConflict is an asynchronous notification from TRM to the owner of a token
 * that a tuner reservation is about to be terminated, unless the owner initiates to resolve the conflict.
 * If no conflict resolution is provided, the current reservation will be terminated automatically at the
 * startTime of the new reservation. The owner of current reservation will receive
 * notifyTunerReservationRelease notification when its reservation is released by TRM.
 * If any value of the tunerReservation does not match those held by the owner, the notifyTunerConflict
 * should be discarded. This can happen if user has changed tuner activity after the notifyTunerConflict is sent.
 *
 * The message format,
 *
 * @code
 * {
 *      "notifyTunerReservationConflict" : {
 *          "requestId"                  : [String] requestId,
 *          "tunerReservation"           :
 *          <TunerReservation>
 *          "conflicts" : [<TunerReservation>]
 *      }
 * }
 * @endcode
 *
 * The fields are defined as follows:
 *
 * - @b requestId : a GUID used to match requests with responses.
 * For every request, the client supplies a requestId corresponding to the response.
 * - @b tunerReservation: details of the current reservation that has to be cancelled as a result of the conflict.
 * This reservation is in conflict with those listed in conflicts. Upon notification, the owner must initialize
 * conflict resolution if it wishes to retain its reservation.
 * - @b conflicts: one or more reservations that are going to override current reservation.
 * The reservations listed in most conflicts will be cancelled if the owner wishes to retain the reservation in tunerReservation.
 *
 * @defgroup NotifyTunerReservationRelease Notify Tuner Reservation Release
 * @ingroup TRM_NOTIFY_MSG
 * @b NotifyTunerReservationRelease is an Asynchronous Notification from TRM to the owner of a token that its
 * tuner reservation has been terminated. The token is no longer valid after receiving this message.
 *
 * The message format,
 *
 * @code
 * {
 *      "notifyTunerReservationRelease" : {
 *          "requestId"           : [String] requestId,
 *          "reservationToken"    : [String] reservationToken
 *          "reason"              : [String] reason
 *      }
 * }
 * @endcode
 *
 * The fields are defined as follows:
 *
 * - @b requestId : a GUID used to match requests with responses.
 * For every request, the client supplies a requestId corresponding to the response.
 * - @b reservationToken: the tuner reservation that is terminated.
 * - @b reason: a message explaining why the reservation is terminated.
 *
 * @defgroup NotifyTunerStatesUpdate Notify Tuner State Update
 * @ingroup TRM_NOTIFY_MSG
 * @b NotifyTunerStatesUpdate is an Asynchronous Notification from TRM whenever a tuner has changed it state.
 * The TRM client can listen for this notification and keep a local cache of the tuner states.
 * Note the payload represents the states when the message is generated.
 * Tuner states could change after the message is sent.
 *
 * The message format
 *
 * @code
 * {
 *        "notifyTunerStatesUpdate" : {
 *            "requestId"        : [String] requestId,
 *            "detailedState"    : <AllTunerDetailedStates>
 *        }
 * }
 * @endcode
 *
 * The fields are defined as follows:
 *
 * - @b requestId : a GUID used to match requests with responses.
 * For every request, the client supplies a requestId corresponding to the response.
 * - @b detailedStates: state Information about each tuner.
 *
 * @defgroup NotifyTuneReservationUpdate Notify Tuner Reservation Update
 * @ingroup TRM_NOTIFY_MSG
 *
 * @b NotifyTuneReservationUpdate is an Asynchronous Notification from TRM whenever a reservation has changed
 * its usage by its owner. The Server can listen for this notification and synchronize its cached value with the
 * new reservation. In the update message, these fields are guaranteed to be the original value associated with the reservation.
 * - tunerReservation::reservationToken
 * - tunerReservation::device
 * - tunerReservation::activity
 * - tunerReservation::customAttributes
 *
 * The message format,
 *
 * @code
 * {
 *      "notifyTunerReservationUpdate" : {
 *          "requestId" : [String] requestId,
 *          "tunerReservation" : <TunerReservation>
 *      }
 * }
 * @endcode
 *
 * The fields are defined as follows,
 *
 * - @b requestId : a GUID used to match requests with responses.
 * For every request, the client supplies a requestId corresponding to the response.
 * - @b tunerReservation: the reservation that has changed.
 */

/**
 * @defgroup reserveTuner Reserve Tuner
 * @ingroup TRM_GENERAL_MESSAGE
 *
 * @par Reserve Tuner Message
 * @n
 * The client uses this message to request, update or renew a reservation.
 * When renewing a reservation, the tunerReservation contains an existing reservationToken
 * and matching values of {device, activity}.
 * @n
 * Message format for @b reserveTuner
 * @code
 * {
 *      "reserveTuner" : {
 *         "requestId"        : [String] requestId,
 *         "device"           : [String] device,
 *         "tunerReservation" : <TunerReservation> 
 *      }
 * }
 * @endcode
 * @n
 * Fields are defined as follows: @n
 * - @b requestId : a GUID used to match requests with responses.
 * For every request, the client supplies a requestId corresponding to the response.
 * - @b device : the remote device making the request.
 * - @b reservationToken : If present and valid in <tunerReservation>, the existing reservation will be updated.
 * - @b tunerReservation : details of the requested, renewed or updated reservation. If the tunerReservation
 * contains reservationToken entity, the request is to renew or update an existing
 * reservation identified by the token. Only certain values of tokenReservation can be updated.
 * The field should not be present in the initial ReserveTuner message. The value can only be created by TRM.
 * - @b resurrect : If resurrect flag is on, then the TRM will accept.
 *
 * @par Reserve Tuner Response Message
 * @n
 * The tuner reservations included in the response are granted reservations. The requesting component
 * (e.g Guide-App or Recorder) must comply with the granted reservations, or update the granted
 * reservations with the actual tuner usage.
 * @n
 * Message format:
 * @n
 * @code
 * "reserveTunerResponse" =:
 * {
 *   "requestId"                : [String] requestId,
 *   "status"                   : [String] status,
 *   "statusMessage" (optional) : [String] statusMessage
 *   "tunerReservation"         : <TunerReservation>
 *   "conflicts"                : [ [<TunerReservation>, ...], ...]
 * }
 * @endcode
 * @n
 * Attribute details:
 * @n
 * - @b requestId : a GUID (Globally Unique Identifier) used to match requests with responses. For every request,
 * the client supplies a requestId that it can use to match the corresponding response.
 * For every notification, the sender supplies a requestId.
 * - @b status : is an enumeration of strings indicating the status of the request
 * - @b statusMessage : is a string containing additional information about the status.
 * - @b tunerReservation : the returned/granted reservation. This may or may not exist in the response message
 * depending on the conflict resolution rules used by TRM. If present, its values may be different
 * (in serviceLocator, or in startTime and such) from what are requested.
 * When this field is present and there is no conflicts in the response, the owner of the token must
 * comply with the activity in the response message. When this field is present and there is conflicts in the
 * response, the granted tunerReservation is compatible to those listed in conflicts. The owner of the granted
 * token must either accept the granted resolution as is or resolve the conflict without acting on the granted
 * reservation. After conflict is resolved, and the resolution results in different from the granted reservation,
 * the owner must update the granted token with the new values. When this field is not present, the owner of the
 * token must initiate resolution and then try reservation again.
 * - @b conflicts : an array of existing tuner reservations that is in conflict with the requested reservation.
 *
 * @defgroup releaseTunerReservation Release Tuner Reservation
 * @ingroup TRM_GENERAL_MESSAGE
 *
 * @par Release Tuner Reservation Message
 * @n
 * The client can release an existing tuner reservation. Normally,
 * a token need not be released if its owner device and activity have not changed.
 * In this case a next TunerReservation request will update the same token with new values of
 * { servicelocator, startTime, duration}.
 * @n
 * The format of releasing a reservation is,
 * @n
 * @code
 * {
 *     "releaseTunerReservation" : {
 *          "requestId"        : [String] requestId,
 *          "device"           : [String] device,
 *          "reservationToken" : [String] reservationToken
 *     }
 * }
 * @endcode
 * @n
 * The fields are defined as follows:
 * @n
 * - @b requestId : a GUID (Globally Unique Identifier) used to match requests with responses. For every request,
 * the client supplies a requestId that it can use to match the corresponding response.
 * For every notification, the sender supplies a requestId.
 * - @b device : the remote device making the request.
 * - @b reservationToken: the reservation to be released.
 * After successful release, the token is no longer valid and cannot be used in future messages.
 * It is clients responsibility to stop the associated activity first before releasing the tuner reservation with TRM.
 *
 * @par Release Tuner Reservation Response Message
 * @n
 * The general format of the response payload is:
 * @n
 * @code
 * {
 *       releaseTunerReservationResponse: {
 *           "requestId"         : [String] requestId,
 *           <ResponseStatus>
 *           "reservationToken"  : [String] reservationToken
 *           "released"          : [String] released
 *       }
 * }
 * @endcode
 * @n
 * The fields are defined as follows:
 * @n
 * - @b reservationToken : the reservation released. After successful release, the token is no longer valid
 * and cannot be used in future messages.
 * - @b Released : a string of "true" or "false", indicating if an valid token is successfully released.
 * If the token is not valid, the ResponseStatus will carry "MALFORMED_REQUEST"
 *
 * @defgroup validateTunerReservation Validate Tuner Reservation
 * @ingroup TRM_GENERAL_MESSAGE
 *
 * @par Validate Tuner Reservation Message
 * @n
 * The client can validate an existing tuner reservation. A reservation is valid after it is created,
 * and invalid after it is released either per client's request or upon receiving asynchronous notification from TRM.
 * @n
 * The format of validating a reservation is:
 * @n
 * @code
 * {
 *       "validateTunerReservation" : {
 *            "requestId" [required]        : [String] requestId,
 *            "device" [required]           : [String] device,
 *            "reservationToken" [required] : [String] reservationToken
 *       }
 * }
 * @endcode
 * @n
 * The fields are defined as follows:
 * @n
 * - @b requestId : a GUID to be used for every request.
 * - @b device : the remote device making the request.
 * - @b reservationToken: The reservation to be validated.
 *
 * @par Validate Tuner Reservation Response Message
 * @n
 * The general format of the response payload is:
 * @n
 * @code
 * {
 *        validateTunerReservationResponse: {
 *            "requestId"        : [String] requestId,
 *            <ResponseStatus>
 *            "reservationToken" : [String] reservationToken
 *            "valid"            : [String] valid
 *        }
 * }
 * @endcode
 * @n
 * The fields are defined below,
 * @n
 * - @b reservationToken: the reservation validated.
 * - @b valid: a string of "true" or "false" indicates if the token is valid or not.
 * The token is valid only if the reservationToken and device both match the record maintained by TRM.
 *
 * @defgroup cancelRecording Cancel Recording
 * @ingroup TRM_GENERAL_MESSAGE
 *
 * @par Cancel Recording Message
 * @n
 * The client uses this message to request TRM to cancel the recording associated with the supplied
 * reservationToken. After successful cancellation of the recording, the associated token is no longer valid.
 * @n
 * The message format,
 * @n
 * @code
 * {
 *       "cancelRecording" : {
 *            "requestId"        : [String] requestId,
 *            "reservationToken" : [String] reservationToken,
 *       }
 * }
 * @endcode
 * @n
 * The fields are defined as follows:
 * @n
 * - @b requestId : a GUID to be used for every request.
 * - @b reservationToken: the token whose recording is to be cancelled. The activity of the token must be Record.
 *
 * @par Cancel Recording Response Message
 * @n
 * The response message to a CancelRecording request.
 * @n
 * @code
 * {
 *       "cancelRecordingResponse" : {
 *           "requestId"          : [String] requestId,
 *           <ResponseStatus>
 *           "reservationToken "  : [String] reservationToken
 *           "cancelled"          : cancelled
 *       }
 * }
 * @endcode
 * @n
 * The fields are defined as follows:
 * @n
 * - @b reservationToken: The reservation that is cancelled.
 * - @b state: state of the tuner after reservation is cancelled.
 * - @b cancelled: if the recording is cancelled. "false" is also returned if the recording does not exist.
 *
 * @defgroup TRM_MESSAGE_IFACE TRM Interface Classes
 * Described the details about the classes used in Messages.
 * @ingroup TRM_MESSAGE
 */

/**
* @defgroup trm
* @{
* @defgroup common
* @{
**/


#ifndef TRM_MESSAGES_H_
#define TRM_MESSAGES_H_

#include <list>
#include <stdint.h>

#include "TRM.h"
#include "Activity.h"
#include "TunerReservation.h"
#include "ResponseStatus.h"
#include "Klass.h"

TRM_BEGIN_NAMESPACE

class ServiceAttributes
{
private:
    static std::map<std::string, ServiceAttributes> attributes;

public:
    static const ServiceAttributes & get(const std::string &serviceLocator) {   
        return attributes[serviceLocator]; 
    }   
    static void set(const std::string &serviceLocator, const ServiceAttributes &attribute) {
        attributes[serviceLocator] = attribute;
    }   

    static bool exist(const std::string &serviceLocator) {
        return attributes.find(serviceLocator) != attributes.end();
    }

public: 
    ServiceAttributes(const std::string &shortName = "", const std::string &providerImageURL = "", const std::string &vcn = "") 
    : shortName(shortName), providerImageURL(providerImageURL), vcn(vcn) {
    }   

    std::string shortName;
    std::string providerImageURL;
    std::string vcn;
};

enum MessageType {
	Request = 0x1234,
	Response = 0x1800,
	Notification = 0x1400,
	Unknown,
};

class MessageBase
{
public:
	static const char *klassName(void) { return Klass::kMessageBase; }

	const std::string & getClassName(void) const
	{
		return className;
	}

	const MessageType & getType(void) const
	{
		return type;
	}

	const std::string & getUUID(void) const
	{
		return uuid;
	}

	virtual void print(void) const
	{
		std::cout << "[OBJ][" << klassName() << "] className = " << className << std::endl;
	}
protected:

	MessageBase(const std::string &className, MessageType type, const std::string &uuid)
	: className(className),  type(type), uuid(uuid) {}
	virtual ~MessageBase(void) {};

private:
	std::string className;
	MessageType type;
	std::string uuid;

};

class NoResponse : public MessageBase
{
public:
	static const char *klassName(void) { return Klass::kNoResponse; }

	NoResponse(const std::string &uuid/*uuid*/)
	: MessageBase(klassName(), Response, uuid)
	{
	}
};


class RequestBase : public MessageBase
{
public:
	static const char *klassName(void) { return Klass::kRequestBase; }

	RequestBase(const std::string &className, const std::string &uuid, const std::string &device)
	: MessageBase(className, Request, uuid), device(device)
	{
	};

	RequestBase(const std::string &className, const std::string &uuid)
	: MessageBase(className, Request, uuid), device("")
	{
	};

	virtual void print(void) const
	{
		MessageBase::print();
		std::cout << "[OBJ][" << klassName() << "] uuid = " << getUUID() << std::endl;
		if (!device.empty())
		std::cout << "[OBJ][" << klassName() << "] device = " << getDevice() << std::endl;

	};

	const std::string &getDevice(void) const {
		return device;
	}
protected:
	std::string device;

};


class ResponseBase : public MessageBase
{
public:

	typedef  NoResponse ResponseType;

	static const char *klassName(void) { return Klass::kResponseBase; }

	ResponseBase(const std::string    & className,
			     const std::string    & uuid,
			     const ResponseStatus & status)
	: MessageBase(className, Response, uuid),
	  status(status)
	{
	};

	const ResponseStatus & getStatus(void) const {
		return status;
	}

	ResponseStatus & getStatus(void) {
		return status;
	}

	virtual void print(void) const
	{
		MessageBase::print();
		status.print();
	};

protected:
	ResponseStatus status;
};

class NotificationBase : public MessageBase
{
public:
	typedef  NoResponse ResponseType;

	static const char *klassName(void) { return Klass::kNotificationBase; }

	NotificationBase(const std::string &className, const std::string &uuid)
	: MessageBase(className, Notification, uuid)
	{
	};

	virtual void print(void) const
	{
		MessageBase::print();
		std::cout << "[OBJ][" << klassName() << "] uuid = " << getUUID() << std::endl;
	};
};

typedef MessageBase  Message;

class SimpleTRMRequest : public RequestBase
{
public:
	static const char *klassName(void) { return Klass::kSimpleTRMRequest; }

	virtual void print(void) const
	{
		RequestBase::print();
		if (!reservationToken.empty())
		std::cout << "[OBJ][" << klassName() << "]reservationToken = " << reservationToken<< std::endl;
	}

	const std::string & getReservationToken(void) const
	{
		return reservationToken;
	}

public:
	SimpleTRMRequest(const std::string &className, const std::string &uuid, const std::string &device, const std::string & reservationToken)
	: RequestBase( className, uuid, device),
	  reservationToken(reservationToken)
	{
	}

	SimpleTRMRequest(const std::string &className, const std::string &uuid, const std::string & reservationToken)
	: RequestBase( className, uuid, ""),
	  reservationToken(reservationToken)
	{
	}

	virtual ~SimpleTRMRequest(void)
	{
	}


private:
	std::string reservationToken;
};

class SimpleTRMResponse : public ResponseBase
{
public:
	static const char *klassName(void) { return Klass::kSimpleTRMResponse; }

	virtual void print(void) const
	{
		ResponseBase::print();
		if (!reservationToken.empty())
		std::cout << "reservationToken = " << reservationToken<< std::endl;
	}

	const std::string & getReservationToken(void) const
	{
		return reservationToken;
	}

public:
	SimpleTRMResponse(const std::string    &className, const std::string &uuid,
					  const ResponseStatus &status,   const std::string & reservationToken)
	: ResponseBase( className, uuid, status),
	  reservationToken(reservationToken)
	{
	}

	virtual ~SimpleTRMResponse(void)
	{
	}

private:
	std::string reservationToken;
};

/**
 * @brief Class implementing the response message to a ReserveTuner request.
 *
 * The tuner reservations included in the response are granted reservations.
 * The requesting component (e.g. Guide or Recorder) must comply with the granted reservations,
 * or update the granted reservations with the actual tuner usage
 * @ingroup TRM_MESSAGE_IFACE
 */
class ReserveTunerResponse : public ResponseBase
{
public:
	typedef std::list<TunerReservation> ReservationCT;
	typedef std::list<TunerReservation> ConflictCT;


	static const char *klassName(void) { return Klass::kReserveTunerResponse; }

	ReserveTunerResponse(const std::string &uuid = "")
	: ResponseBase(klassName(), uuid, ResponseStatus::kOk) {}

	ReserveTunerResponse(const std::string 		&uuid,
						 const ResponseStatus 	&status,
			             const TunerReservation &tunerReservation)
	: ResponseBase(klassName(), uuid, status),
	  tunerReservation(tunerReservation)
	{
	}

	ReserveTunerResponse(const std::string 		&uuid,
						 const ResponseStatus 	&status,
			             const ConflictCT &conflicts = ConflictCT ())
	: ResponseBase(klassName(), uuid, status),
	  conflicts(conflicts)
	{
	}

/**
 * @brief This function is used to return the unique token generated when a reservation
 * is created.
 * This token will be used with the response message payload.
 *
 * @return Returns the already granted reservation token.
 */
	const TunerReservation & getTunerReservation(void) const {
		return tunerReservation;
	}

/**
 * @brief This function is used to set the input token which will
 * be used to uniquely identify the response message.
 *
 * @param[in] tunerReservation A unique token that the requesting device can use to make the remote tuning request.
 * @return None.
 */
	void setTunerReservation(const TunerReservation &tunerReservation) {
		this->tunerReservation = tunerReservation;
	}

/**
 * @brief This function returns an array of existing tuner reservations that is in
 * conflict with the requested reservation
 *
 * @return Array of conflict reservations.
 */
	const ConflictCT & getConflicts(void) const {
		return conflicts;
	}

/**
 * @brief This function returns an array of existing tuner reservations that is in
 * conflict with the requested reservation
 *
 * @return Array of conflict reservations.
 */
	ConflictCT & getConflicts(void) {
		return conflicts;
	}

/**
 * @brief This function adds a tuner reservation request to the array of existing tuner reservations that is in
 * conflict with the requested reservation
 *
 * @param[in] reservation A unique token that the requesting device can use to make the remote tuning request.
 * @return None.
 */
	void addConflict(const TunerReservation &reservation) {
		conflicts.push_back(reservation);
	}

/**
 * @brief This function prints the list of tuner reservations with all the conflicts if exist.
 * It is used for debugging purpose.
 *
 * @return None.
 */
	virtual void print(void) const
	{
		ResponseBase::print();
		if (conflicts.size() == 0) {
			tunerReservation.print();
		}
		else {
			ConflictCT::const_iterator itm;
			for (itm = conflicts.begin(); itm != conflicts.end(); itm++) {
				const TunerReservation & reservation = *itm;
				reservation.print();
			}
		}
	}
private:
	TunerReservation tunerReservation;
	ConflictCT conflicts;
};

/**
 * @brief Class implementing a Tuner reservation request, the client uses this message
 * to request, update or renew a reservation.
 *
 * When renewing a reservation, the tunerReservation contains an existing reservationToken
 * and matching values of {device, activity}
 * @ingroup TRM_MESSAGE_IFACE
 */
class ReserveTuner : public RequestBase
{
public:
	typedef ReserveTunerResponse ResponseType;

	static const char *klassName(void) { return Klass::kReserveTuner; }

	ReserveTuner(void)
	: RequestBase(klassName(), "", "") {}

	ReserveTuner(const std::string &uuid, const std::string &device, const TunerReservation &tunerReservation, const std::string &resurrect ="false")
	: RequestBase(klassName(), uuid, device), tunerReservation(tunerReservation),resurrect(resurrect)
	{
	}

	virtual ~ReserveTuner(void)
	{
	}

	virtual void print(void) const
	{
		RequestBase::print();
		tunerReservation.print();
	}

/**
 * @brief This function is used to return the unique token generated when a reservation
 * is created. This token will be used when creating a reservation request message payload.
 *
 * If present and valid in <tunerReservation>, the existing reservation will be updated.
 * The field should not be present in the initial ReserveTuner message.
 *
 * @return Returns the existing reservation token.
 */
    TunerReservation & getTunerReservation(void)
	{
		return tunerReservation;
	}

/**
 * @brief This function is used to return the unique token generated when a reservation
 * is created. This token will be used when creating a reservation request message payload.
 *
 * If present and valid in <tunerReservation>, the existing reservation will be updated.
 * The field should not be present in the initial ReserveTuner message.
 *
 * @return Returns the existing reservation token.
 */
	const TunerReservation & getTunerReservation(void) const
	{
		return tunerReservation;
	}

/**
 * @brief This function is used to return the list of resurrect (dead) reservation tokens.
 *
 * A resurrect token is the one which the TRM server has lost due to re-set.
 *
 * @return Returns the array of resurrect reservation token.
 */
	const std::string & getResurrect(void) const
	{
		return resurrect;
	}

private:
	TunerReservation tunerReservation;
	 std::string resurrect;
};

/**
 * @brief Class implementing the response message payload for releasing Tuner reservation.
 * @ingroup TRM_MESSAGE_IFACE
 */
class ReleaseTunerReservationResponse : public SimpleTRMResponse
{
public:
	static const char *klassName(void) { return Klass::kReleaseTunerReservationResponse; }

	ReleaseTunerReservationResponse(const std::string &uuid = "", const std::string &reservationToken = "")
	: SimpleTRMResponse(klassName(), uuid, ResponseStatus(ResponseStatus::kOk), reservationToken) {}

	ReleaseTunerReservationResponse(const std::string    &uuid,
									const ResponseStatus &status,
									const std::string    &reservationToken,
									const bool           &released)
	: SimpleTRMResponse(klassName(), uuid, status, reservationToken),
	  released(released)
	{
	}

/**
 * @brief This function is used to a return the value of released flag which may be
 * “true” or “false”, indicating if a valid token is successfully released or not.
 *
 * If the token is not valid, the ResponseStatus will carry “MALFORMED_REQUEST”
 *
 * @return Returns the value of released flag.
 * @retval true if token is released, false otherwise.
 */
	bool isReleased(void) const {return released;}

/**
 * @brief This function is used to set the value of released flag with input value.
 *
 * A true value will be supplied in response message when a valid token is released successfully.
 *
 * @param [in] result Boolean value used for setting the released flag.
 */
	void setReleased(bool result) {released = result;}

	~ReleaseTunerReservationResponse(void){}
private:
	bool released;
};

/**
 * @brief Implements the message payload for releasing tuner reservation.
 *
 * Generally a release request is needed if the device owner or activity has changed.
 * @ingroup TRM_MESSAGE_IFACE
 */
class ReleaseTunerReservation : public SimpleTRMRequest
{
public:

	typedef ReleaseTunerReservationResponse ResponseType;

	static const char *klassName(void) { return Klass::kReleaseTunerReservation; }

	ReleaseTunerReservation(void)
	: SimpleTRMRequest(klassName(), "", ""){}

	ReleaseTunerReservation(const std::string &uuid, const std::string &device, const std::string & reservationToken)
	: SimpleTRMRequest(klassName(), uuid, device, reservationToken)
	{
	}

	~ReleaseTunerReservation(void){}
private:
};

/**
 * @brief Implements the response message payload for Tuner reservation validation requests.
 * @ingroup TRM_MESSAGE_IFACE
 */
class ValidateTunerReservationResponse : public SimpleTRMResponse
{
public:
	static const char *klassName(void) { return Klass::kValidateTunerReservationResponse; }

	ValidateTunerReservationResponse(const std::string &uuid = "", const std::string &reservationToken = "")
	: SimpleTRMResponse(klassName(), uuid, ResponseStatus(ResponseStatus::kOk), reservationToken) {}

	ValidateTunerReservationResponse(const std::string    &uuid,
									 const ResponseStatus &status,
									 const std::string    &reservationToken,
									 const bool           &valid)
	: SimpleTRMResponse(klassName(), uuid, status, reservationToken),
	  valid(valid)
	{
	}

/**
 * @brief This function returns whether the token requested is valid or not.
 *
 * A string of “true” or “false” indicates if the token is valid or not.
 * The token is valid only if the reservationToken and device both match the record maintained by TRM
 *
 * @return Returns a boolean value indicating the token is valid or not.
 */
	bool isValid(void) const {return valid;}

/**
 * @brief This function sets the value of 'valid' flag for a validate tuner reservation response.
 *
 * A true value will be present in the "valid" field of response message if the token and device
 * both match in the record maintained by TRM.
 *
 * @param [in] result Boolean value used for setting the valid flag.
 */
	void setValid(bool result) {valid = result;}

	~ValidateTunerReservationResponse(void){}

private:
	bool valid;
};

/**
 * @brief Implements the message format for client to validate an existing tuner reservation.
 *
 * A reservation is valid after it is created, and invalid after it is released either
 * per client’s request or upon receiving asynchronous notification from TRM.
 * @ingroup TRM_MESSAGE_IFACE
 */
class ValidateTunerReservation : public SimpleTRMRequest
{
public:

	typedef ValidateTunerReservationResponse ResponseType;

	static const char *klassName(void) { return Klass::kValidateTunerReservation; }

	ValidateTunerReservation(void)
	: SimpleTRMRequest(klassName(), "", ""){}

	ValidateTunerReservation(const std::string &uuid,
							 const std::string &device,
							 const std::string &reservationToken)
	: SimpleTRMRequest(klassName(), uuid, device, reservationToken)
	{
	}

	~ValidateTunerReservation(void){}
private:
};

/**
 * @brief Implements payload for a response message against cancel recording request.
 * @ingroup TRM_MESSAGE_IFACE
 */
class CancelRecordingResponse : public SimpleTRMResponse
{
public:
	static const char *klassName(void) { return Klass::kCancelRecordingResponse; }

	CancelRecordingResponse(const std::string &uuid = "", const std::string &reservationToken = "")
	: SimpleTRMResponse(klassName(), uuid, ResponseStatus(ResponseStatus::kOk), reservationToken)
	  {}

	CancelRecordingResponse(const std::string    &uuid,
			 	 	 	 	const ResponseStatus &status,
			 	 	 	 	const std::string    &reservationToken,
			 	 	 	 	const bool           &canceled)
	: SimpleTRMResponse(klassName(), uuid, status, reservationToken),
	  canceled(canceled)
	{
	}

/**
 * @brief This function returns whether a recording is cancelled, this is used when preparing
 * the response message against a cancel recording request.
 *
 * @return Returns "true" if the recording is cancelled. “false” if the recording does not exist.
 */
	bool isCanceled(void) const {return canceled;}

/**
 * @brief This function sets the value of "cancelled" field for the response message
 * of a cancel recording request.
 *
 * @param [in] result Boolean value used for setting the 'cancelled' flag.
 */
	void setCanceled(bool result) {canceled = result;}

	~CancelRecordingResponse(void){}
private:
	bool canceled;
};

/**
 * @brief Implements a message to cancel the recording.
 *
 * The client uses this message to request TRM to cancel the recording associated with
 * the supplied reservationToken. After successful cancellation of the recording,
 * the associated token is no longer valid.
 * @ingroup TRM_MESSAGE_IFACE
 */
class CancelRecording : public SimpleTRMRequest
{
public:

	typedef CancelRecordingResponse ResponseType;

	static const char *klassName(void) { return Klass::kCancelRecording; }

	CancelRecording(void)
	: SimpleTRMRequest(klassName(), "", ""){}

	CancelRecording(const std::string &uuid, const std::string & reservationToken)
	: SimpleTRMRequest(klassName(), uuid, reservationToken)
	{
	}

	~CancelRecording(void){}
private:
};

/**
 * @brief Implements payload for a response message against cancel live streaming request.
 * @ingroup TRM_MESSAGE_IFACE
 */
class CancelLiveResponse : public SimpleTRMResponse
{
public:
	static const char *klassName(void) { return Klass::kCancelLiveResponse; }

	CancelLiveResponse(const std::string &uuid = "", const std::string &reservationToken = "")
	: SimpleTRMResponse(klassName(), uuid, ResponseStatus(ResponseStatus::kOk), reservationToken),
	  canceled(false), serviceLocator()
	  {}

	CancelLiveResponse(const std::string    &uuid,
			 	 	 	 	const ResponseStatus &status,
			 	 	 	 	const std::string    &reservationToken,
			 	 	 	 	const std::string    &serviceLocator,
			 	 	 	 	const bool           &canceled)
	: SimpleTRMResponse(klassName(), uuid, status, reservationToken),
	  canceled(canceled), serviceLocator(serviceLocator)
	{
	}

/**
 * @brief This will return whether the Live session has been cancelled, this information
 * will be used in the response message of a cancel live streaming request.
 *
 * @return Returns true if the live streaming is cancelled, false otherwise.
 */
	bool isCanceled(void) const {return canceled;}

/**
 * @brief This will update value of the  "cancelled" flag which will be used in the response message
 * of a cancel live streaming request.
 *
 * @param [in] result Boolean value used for setting the 'cancelled' flag.
 */
	void setCanceled(bool result) {canceled = result;}

/**
 * @brief This function returns a string containing the service Locator which will be used
 * while preparing response for a cancel live streaming request.
 *
 * @return Returns the service locator string.
 */
	const std::string &getServiceLocator() const{
		return serviceLocator;
	}

/**
 * @brief This function sets value of the service Locator field which will be used in the
 * response message of a cancel live streaming request.
 *
 * @param [in] serviceLocator A string value representing the service locator (source locator).
 */
	void setServiceLocator(const std::string &serviceLocator) {
		this->serviceLocator = serviceLocator;
	}
	~CancelLiveResponse(void){}
private:
	bool canceled;
	std::string serviceLocator;
};

/**
 * @brief Implements a message to cancel the live streaming.
 *
 * The client uses this message to request TRM to cancel the live streaming session associated with
 * the supplied reservationToken. After successful cancellation of the session,
 * the associated token is no longer valid.
 * @ingroup TRM_MESSAGE_IFACE
 */
class CancelLive : public SimpleTRMRequest
{
public:

	typedef CancelLiveResponse ResponseType;

	static const char *klassName(void) { return Klass::kCancelLive; }

	CancelLive(void)
	: SimpleTRMRequest(klassName(), "", ""){}

	CancelLive(const std::string &uuid, const std::string & serviceLocator, const std::string & reservationToken = "")
	: SimpleTRMRequest(klassName(), uuid, reservationToken), serviceLocator(serviceLocator)
	{
	}

/**
 * @brief This function returns a string containing the service Locator which will be used
 * while preparing the cancel live streaming request.
 *
 * @return Returns the service locator string.
 */
	const std::string & getServiceLocator(void) const {
		return serviceLocator;
	}

/**
 * @brief This function sets value of the service Locator field which will be used while
 * preparing the cancel live streaming request.
 *
 * @param[in] serviceLocator A string value representing the service locator (source locator).
 */
	void setServiceLocator(const std::string &serviceLocator) {
		this->serviceLocator = serviceLocator;
	}
	~CancelLive(void){}
private:
	std::string serviceLocator;
};

/**
 * @brief Implements the response message payload against a Tuner Id request.
 * @ingroup TRM_MESSAGE_IFACE
 */
class GetAllTunerIdsResponse : public SimpleTRMResponse
{
public:
	static const char *klassName(void) { return Klass::kGetAllTunerIdsResponse; }

	GetAllTunerIdsResponse(const std::string &uuid = "")
	: SimpleTRMResponse(klassName(), uuid, ResponseStatus(ResponseStatus::kOk), "") {}

	GetAllTunerIdsResponse(const std::string    &uuid,
						   const ResponseStatus &status,
						   const std::list<std::string> &  tunerIds = std::list<std::string>())
	: SimpleTRMResponse(klassName(), uuid, status, ""),
	  tunerIds(tunerIds)
	{
	}

/**
 * @brief This function will return an array of tuner id.
 *
 * @return Returns an array of tuner Identifiers.
 */
	const std::list<std::string> & getTunerIds(void) const {
		return tunerIds;
	}

/**
 * @brief This function will add a new tuner identifier to the list of tuner ID.
 *
 * @param [in] tid An unique tuner identifier.
 */
	void addTunerId(const std::string &tid) {
		tunerIds.push_back(tid);
	}

/**
 * @brief This function will add a list of new tuner identifiers to existing array of tuner ID.
 *
 * @param [in] tid List of unique tuner identifiers.
 */
	void addTunerId(const std::list<std::string> &tid) {
		tunerIds.insert(tunerIds.end(), tid.begin(), tid.end());
	}

	void print(void) const {
		SimpleTRMResponse::print();
		std::list<std::string>::const_iterator it;
		for (it = tunerIds.begin(); it != tunerIds.end(); it++) {
			std::cout << "[OBJ][" << klassName() << "]tunerId = " << *it << std::endl;
		}
	}
	~GetAllTunerIdsResponse(void){}

private:
	std::list<std::string> tunerIds;
};

/**
 * @brief Implements the message payload format for requesting the system allocated Unique Id of tuner.
 * The ID for each tuner is guaranteed to be unique within the same target host.
 * @ingroup TRM_MESSAGE_IFACE
 */
class GetAllTunerIds : public SimpleTRMRequest
{
public:

	typedef GetAllTunerIdsResponse ResponseType;

	static const char *klassName(void) { return Klass::kGetAllTunerIds; }

	GetAllTunerIds(void)
	: SimpleTRMRequest(klassName(), "", ""){}

	GetAllTunerIds(const std::string &uuid, const std::string &device="")
	: SimpleTRMRequest(klassName(), uuid, device, "")
	{
	}

	~GetAllTunerIds(void){}
private:
};

/**
 * @brief Class for implementing the detail state information of a single tuner.
 * @ingroup TRM_MESSAGE_IFACE
 */
class DetailedTunerState {
public:
	DetailedTunerState(const std::string &state="Free", const std::string &serviceLocator="", const ServiceAttributes &serviceAttributes = ServiceAttributes())
	: state(state), serviceLocator(serviceLocator), reservedDeviceId("") , serviceAttributes(serviceAttributes) {

	}

/**
 * @brief This function will return the detailed state for the tuner.
 * @return Returns a string value containing the detailed tuner state.
 */
	const std::string getState() const {
		return state;
	}

/**
 * @brief This function will Service Locator that has to be used in the Detailed State message.
 * @return Returns the Service Locator string.
 */
	const std::string getServiceLocator() const {
		return serviceLocator;
	}

/**
 * @brief This function will return the Owners list in the form of activity and device
 * @return  Returns the Owners list string.
 */
	const std::map<std::string, std::string> & getOwners(void) const {
		return owners;
	}

/**
 * @brief This function will return the ReservedDeviceId for the tuner.
 *
 * If this value is present in the detailedStates, the corresponding tuner is can only be used in the following cases.
 * - LIVE streaming for the device represented by the reservedDeviceId.
 * - RECORD for any recording request, regardless of the recording’s originating device
 *
 * @return Returns the remote Device name making the request.
 */
	const std::string getReservedDeviceId() const {
		return reservedDeviceId;
	}

/**
 * @brief This function will return the List of service Attributes.
 *
 * @return Returns the ServiceAttributes object.
 */
    const ServiceAttributes &getServiceAttributes() const {
        return serviceAttributes;
    }

/**
 * @brief This function will set the service attributes for DetailedTunerState message
 * such as Image URL and short name.
 *
 * @param [in] serviceAttribute The service attribute object.
 */
    void setServiceAttributes(const ServiceAttributes &serviceAttributes) {
        this->serviceAttributes = serviceAttributes;
    }

/**
 * @brief This function will set the reserved Device Id field while getting Detalied Tuner State message.
 *
 * @param [in] reservedDeviceId String representing Reserved Device Id.
 */
	void setReservedDeviceId(const std::string &reservedDeviceId) {
		this->reservedDeviceId = reservedDeviceId;
	}

/**
 * @brief This function sets the state attribute of the DetailedTunerState message
 * with the input parameters such as tuner state and service Locator.
 *
 * @param [in] state Detailed tuner state.
 * @param [in] serviceLocator Service Locator (Source Locator) string.
 */
	void setState(const std::string &state, const std::string &serviceLocator="") {
		this->state = state;
		this->serviceLocator = serviceLocator;
	}

/**
 * @brief This function will add a new device Id to the activity attribute of owners list.
 *
 * @param [in] activity Tuner Activity such as "Live" or "Record"
 * @param [in] device The unique device identifier.
 */
	void addTunerOwner(const std::string &activity, const std::string &device) {
		owners[activity] = device;
	}

	void print(const std::string &prefix="") const {
		std::cout << prefix << "[detailedStates][state] = " <<  state << std::endl;
		std::cout << prefix << "[detailedStates][serviceLocator] = " <<  serviceLocator << std::endl;
		std::map<std::string, std::string>::const_iterator owners_it = owners.begin();
		for (owners_it = owners.begin(); owners_it != owners.end(); owners_it++) {
			std::cout << prefix << "[detailedState][activity] = " << owners_it->first << std::endl;
			std::cout << prefix << "[detailedState][detice] = " << owners_it->second << std::endl;
		}
	}

	friend class GetAllTunerStatesResponse;
	friend class NotifyTunerStatesUpdate;
	friend std::ostream & operator << (std::ostream &, DetailedTunerState const &);
    private:
	std::string state;
	std::string serviceLocator;
	std::map<std::string, std::string> owners;
	std::string reservedDeviceId;
    ServiceAttributes serviceAttributes;
};

/**
 * @brief Implements the response payload against a Get tuner state request.
 *
 * This message will ask for all tuner reservations that are valid at the time of request.
 * @ingroup TRM_MESSAGE_IFACE
 */
class GetAllTunerStatesResponse : public SimpleTRMResponse
{
public:
	static const char *klassName(void) { return Klass::kGetAllTunerStatesResponse; }

	GetAllTunerStatesResponse(const std::string &uuid = "")
	: SimpleTRMResponse(klassName(), uuid, ResponseStatus(ResponseStatus::kOk), "") {}

	GetAllTunerStatesResponse(const std::string    &uuid,
						      const ResponseStatus &status,
						      const std::map<std::string, std::string> &  tunerStates = std::map<std::string, std::string>())
	: SimpleTRMResponse(klassName(), uuid, status, ""),
	  tunerStates(tunerStates), tunerDetailedStates()
	{
	}

/**
 * @brief This function will return the tuner states which is an enumeration
 * of detailed tuner states at time of request.
 *
 * @return Returns the list of detailed tuner states.
 */
	const std::map<std::string, std::string> & getTunerStates(void) const {
		return tunerStates;
	}

/**
 * @brief This function will add the new state value against the specified tuner id.
 *
 * @param [in] tid The unique tuner identifier.
 * @param [in] state The new state represented as a string value.
 */
	void addTunerState(const std::string &tid, const std::string &state) {
		tunerStates[tid] = state;
	}

/**
 * @brief This function is used to print the debug information for each tuner state.
 */
	void print(void) const {
		SimpleTRMResponse::print();
		{
			std::map<std::string, std::string>::const_iterator it;
			for (it = tunerStates.begin(); it != tunerStates.end(); it++) {
				std::cout << "[OBJ][" << klassName() << "]" << it->first << " = " <<  it->second << std::endl;
			}
		}
		{
			std::map<std::string, DetailedTunerState>::const_iterator it;
			for (it = tunerDetailedStates.begin(); it != tunerDetailedStates.end(); it++) {
				std::string prefix = std::string("[OBJ][") + klassName() + "][detailedStates][" + it->first + "] = ";
				it->second.print(prefix);
			}
		}

	}

	~GetAllTunerStatesResponse(void){}

	const std::map<std::string, DetailedTunerState>  & getTunerDetailedStates(void) const {
		return tunerDetailedStates;
	}

/**
 * @brief This function will add the detailed tuner state value against the specified tuner id.
 *
 * @param [in] tid The unique tuner identifier.
 * @param [in] state The new state represented as a string value.
 * @param [in] serviceLocator The Source Locator string.
 * @param [in] reservedDeciceId The remote Device making the request.
 */
	void addTunerState(const std::string &tid, const std::string &state, const std::string &serviceLocator, const std::string &reservedDeviceId) {
		tunerStates[tid] = state;
		tunerDetailedStates[tid].state = state;
		tunerDetailedStates[tid].serviceLocator = serviceLocator;
		tunerDetailedStates[tid].reservedDeviceId = reservedDeviceId;
	}

/**
 * @brief This function will add the detailed tuner state value against the specified tuner id.
 *
 * @param [in] tid The unique tuner identifier.
 * @param [in] serviceAttributes The Service Attributes such as URL and Short Name.
 */
	void addTunerState(const std::string &tid, const ServiceAttributes &serviceAttributes) {
        tunerDetailedStates[tid].serviceAttributes = serviceAttributes;
    }

/**
 * @brief This function will add the supplied device to the corresponding owners activity list.
 *
 * @param [in] tid The unique tuner identifier.
 * @param [in] activity Tuner activity string which may "Live", "Record", "Hybrid" etc.
 * @param [in] device The Remote device name.
 */
	void addTunerOwner(const std::string &tid, const std::string &activity, const std::string &device) {
		tunerDetailedStates[tid].owners[activity] = device;
	}

private:
	std::map<std::string, std::string> tunerStates;
	std::map<std::string, DetailedTunerState> tunerDetailedStates;
};

/**
 * @brief Implements a request message for getting the state of all tuners in the system.
 *
 * The state field indicates the activity state of a tuner, which can be one of following:
 * - Live: the tuner is reserved for Live activity (Streaming or Playback).
 * - Record: the tuner is reserved for Record activity.
 * - Hybrid: the tuner is reserved for Live and Record activity.
 * - EAS: the tuner is reserved for EAS.
 * - Free: the Tuner is not reserved.
 * @ingroup TRM_MESSAGE_IFACE
 */
class GetAllTunerStates : public SimpleTRMRequest
{
public:

	typedef GetAllTunerStatesResponse ResponseType;

	static const char *klassName(void) { return Klass::kGetAllTunerStates; }

	GetAllTunerStates(void)
	: SimpleTRMRequest(klassName(), "", ""){}

	GetAllTunerStates(const std::string &uuid, const std::string &device="")
	: SimpleTRMRequest(klassName(), uuid, device, "")
	{
	}

	~GetAllTunerStates(void){}
private:
};

/**
 * @brief Implements the response message for the request to get All tuner reservation details.
 * @ingroup TRM_MESSAGE_IFACE
 */
class GetAllReservationsResponse : public SimpleTRMResponse
{
public:
	static const char *klassName(void) { return Klass::kGetAllReservationsResponse; }

	GetAllReservationsResponse(const std::string &uuid = "")
	: SimpleTRMResponse(klassName(), uuid, ResponseStatus(ResponseStatus::kOk), "") {}

	GetAllReservationsResponse(const std::string &uuid,
						   const ResponseStatus &status,
						   const std::map<std::string, std::list<TunerReservation> > & reservations = std::map<std::string, std::list<TunerReservation> > ())
	: SimpleTRMResponse(klassName(), uuid, status, ""),
	  reservations(reservations)
	{
	}

	const std::map<std::string, std::list<TunerReservation> > & getAllReservations(void) const {
		return reservations;
	}

	void addTunerReservation(const std::string &tid, const TunerReservation &reservation) {
		reservations[tid].push_back(reservation);
	}

	~GetAllReservationsResponse(void){}

private:
	std::map<std::string, std::list<TunerReservation> > reservations;
};

/**
 * @brief Implements a request message to get reservation detail of all the tuners
 * that are valid at that time.
 * @ingroup TRM_MESSAGE_IFACE
 */
class GetAllReservations : public SimpleTRMRequest
{
public:

	typedef GetAllReservationsResponse ResponseType;

	static const char *klassName(void) { return Klass::kGetAllReservations; }

	GetAllReservations(void)
	: SimpleTRMRequest(klassName(), "", ""){}

	GetAllReservations(const std::string &uuid, const std::string &filterDevice)
	: SimpleTRMRequest(klassName(), uuid, "", ""),
	  filterDevice(filterDevice)
	{
			filters["device"] = filterDevice;
	}

	GetAllReservations(const std::string &uuid, const std::map<std::string, std::string> &filters)
	: SimpleTRMRequest(klassName(), uuid, "", ""),
	  filterDevice(filterDevice), filters(filters)
	{
	}

	const std::string  getFilterDevice(void) const {
		return filterDevice;
	}

	void setFilterDevice(const std::string &filterDevice) {
		this->filterDevice = filterDevice;
		filters["device"] = filterDevice;
	}

	const std::string getFilter(const std::string filterId) const {
		std::map<std::string, std::string>::const_iterator it = filters.find(filterId);
		return it == filters.end() ? "" : it->second;
	}

	void addFilter(const std::string &filterId, const std::string &filterValue) {
		filters[filterId] = filterValue;
	}

	const std::map<std::string, std::string> getFilters(void) const {
		return filters;
	}

	~GetAllReservations(void){}
private:
	std::string filterDevice;
	std::map<std::string, std::string> filters;
};

/**
 * @brief Implements the response message for the queries that request TRM server version.
 * @ingroup TRM_MESSAGE_IFACE
 */
class GetVersionResponse : public SimpleTRMResponse
{
public:
	static const char *klassName(void) { return Klass::kGetVersionResponse; }

	GetVersionResponse(const std::string &uuid = "")
	: SimpleTRMResponse(klassName(), uuid, ResponseStatus(ResponseStatus::kOk), "") {}

	GetVersionResponse(const std::string    &uuid,
					   const ResponseStatus &status,
					   const std::string &version)
	: SimpleTRMResponse(klassName(), uuid, status, ""),
	  version(version)
	{
	}

	const std::string getVersion(void) const {
		return version;
	}

	void setVersion(const std::string &version) {
		this->version = version;
	}

	void print(void) const {
		SimpleTRMResponse::print();
	    //std::cout << "[OBJ][" << klassName() << "]Version = " << version << std::endl;
	}
	~GetVersionResponse(void){}

private:
	std::string version;
};

/**
 * @brief Implements a message to request for getting TRM server version.
 */
class GetVersion : public SimpleTRMRequest
{
public:

	typedef GetVersionResponse ResponseType;

	static const char *klassName(void) { return Klass::kGetVersion; }

	GetVersion(void)
	: SimpleTRMRequest(klassName(), "", ""){}

	GetVersion(const std::string &uuid, const std::string &device="")
	: SimpleTRMRequest(klassName(), uuid, device, "")
	{
	}

	~GetVersion(void){}
private:
};

/**
 * @brief Class to implement asynchronous Notification from TRM to the owner of a token that
 * its tuner reservation has been terminated. The token is no longer valid after receiving this message.
 * @ingroup TRM_MESSAGE_IFACE
 */
class NotifyTunerReservationRelease : public NotificationBase
{
public:
	static const char *klassName(void) { return Klass::kNotifyTunerReservationRelease; }

	NotifyTunerReservationRelease(const std::string &uuid = "")
	: NotificationBase(klassName(), uuid){}

	NotifyTunerReservationRelease(const std::string &uuid, const std::string &reservationToken, const std::string &reason)
	: NotificationBase(klassName(), uuid),
	  reservationToken(reservationToken), reason(reason)
	  {}

/**
 * @brief This function will return the reservation token that has been terminated.
 *
 * @return Returns the string value containing termination reservation token.
 */
	const std::string & getReservationToken(void) const
	{
		return reservationToken;
	}

/**
 * @brief This function will return the reason for the tuner reservation release.
 * Reason is a message explaining why the reservation is terminated
 *
 * @return Returns the string value containing termination reason.
 */
	const std::string & getReason(void) const
	{
		return reason;
	}

	~NotifyTunerReservationRelease(void){}
private:
	std::string reservationToken;
    std::string reason;
};

/**
 * @brief Implements the asynchronous Notification from TRM whenever a reservation has changed its usage by its owner.
 *
 * The Server can listen for this notification and synchronize its cached value with the new reservation.
 * In the update message, these fields are guaranteed to be the original value associated with the reservation
 * @ingroup TRM_MESSAGE_IFACE
 */
class NotifyTunerReservationUpdate : public NotificationBase
{
public:
	static const char *klassName(void) { return Klass::kNotifyTunerReservationUpdate; }

	NotifyTunerReservationUpdate(const std::string &uuid = "")
	: NotificationBase(klassName(), uuid){}

	NotifyTunerReservationUpdate(const std::string &uuid, const TunerReservation &updatedReservation)
	: NotificationBase(klassName(), uuid),
	  updatedReservation(updatedReservation)
	  {}

/**
 * @brief This function will return the details of the reservation token
 * that has been updated.
 *
 * @return Returns the changed reservation token.
 */
	const TunerReservation & getTunerReservation(void) const {
		return updatedReservation;
	}

/**
 * @brief This function will return the details of the reservation token
 * that has been updated.
 *
 * @return Returns the changed reservation token.
 */
	TunerReservation & getTunerReservation(void) {
		return updatedReservation;
	}

/**
 * @brief This function will set the value for the updated reservation token to current
 * message instance for notification.
 *
 * @param [in] tunerReservation The tuner reservation token that has changed.
 */
	void setTunerReservation(const TunerReservation &tunerReservation) {
		this->updatedReservation = tunerReservation;
	}

	~NotifyTunerReservationUpdate(void){}
private:
	TunerReservation updatedReservation;
};

/**
 * @brief Class for implementing asynchronous notification from TRM to the owner of a token
 * that a tuner reservation is about to be terminated, unless the owner initiates to resolve the conflict.
 * @ingroup TRM_MESSAGE_IFACE
 */
class NotifyTunerReservationConflicts : public NotificationBase
{
public:
	typedef std::list<TunerReservation> ConflictCT;

	static const char *klassName(void) { return Klass::kNotifyTunerReservationConflicts; }

	NotifyTunerReservationConflicts(const std::string &uuid = "")
	: NotificationBase(klassName(), uuid){}

	NotifyTunerReservationConflicts(const std::string &uuid, const TunerReservation &requestedReservation, const ConflictCT &conflicts = ConflictCT())
	: NotificationBase(klassName(), uuid),
	  requestedReservation(requestedReservation), conflicts(conflicts)
	  {}

	NotifyTunerReservationConflicts(const std::string &uuid, const TunerReservation &existingLiveReservation, const TunerReservation &conflictingRecordReservation)
	: NotificationBase(klassName(), uuid),
	  requestedReservation(existingLiveReservation) {
          addConflict(conflictingRecordReservation);
      }

/**
 * @brief This function will return the details of the current reservation
 * that has to be cancelled as a result of the conflict.
 *
 * @return Returns the requested reservation token.
 */
	const TunerReservation & getTunerReservation(void) const {
		return requestedReservation;
	}

/**
 * @brief This function will return the details of the current reservation
 * that has to be cancelled as a result of the conflict.
 *
 * @return Returns the requested reservation token.
 */
	TunerReservation & getTunerReservation(void) {
		return requestedReservation;
	}

/**
 * @brief This function will set the input reservation token to this instance which
 * has to be used for the notification.
 *
 * @param [in] tunerReservation The tuner reservation token.
 */
	void setTunerReservation(const TunerReservation &tunerReservation) {
		this->requestedReservation = tunerReservation;
	}

/**
 * @brief This function will return the array of tuner reservation conflicts.
 *
 * @return Returns the array of conflicts.
 */
	const ConflictCT & getConflicts(void) const {
		return conflicts;
	}

/**
 * @brief This function will return the array of tuner reservation conflicts.
 *
 * @return Returns the array of conflicts.
 */
	ConflictCT & getConflicts(void) {
		return conflicts;
	}

/**
 * @brief This function will add a tuner reservation request to the existing list of conflicts.
 *
 * @param [in] tunerReservation The tuner reservation token.
 */
	void addConflict(const TunerReservation &reservation) {
		conflicts.push_back(reservation);
	}

	~NotifyTunerReservationConflicts(void){}
private:
	//@TODO: Conflicts
	TunerReservation requestedReservation;
	ConflictCT conflicts;
};

/**
 * @brief Class for implementing an asynchronous Notification from TRM whenever a tuner has changed it state.
 *
 * The TRM client can listen for this notification and keep a local cache of the tuner states.
 * @note The payload represents the states when the message is generated. Tuner states could change after the message is sent.
 * @ingroup TRM_MESSAGE_IFACE
 */
class NotifyTunerStatesUpdate : public NotificationBase
{
public:
	static const char *klassName(void) { return Klass::kNotifyTunerStatesUpdate; }

	NotifyTunerStatesUpdate(const std::string &uuid = "")
	: NotificationBase(klassName(), uuid), tunerDetailedStates(){}

	~NotifyTunerStatesUpdate(void){}

/**
 * @brief This function adds the detail state of the tuner specified by it's unique id when a notification
 * has to be issued after tuner has changed it's state.
 *
 * @param [in] tid Unique identification number of tuner.
 * @param [out] detailedState Detailed state of the tuner.
 */
	void addTunerDetailedState(const std::string & tid, const DetailedTunerState &detailedState) {
		tunerDetailedStates[tid] = detailedState;
	}

/**
 * @brief This function returns the detailed tuner state and is used when a notification has to be
 * sent after tuner has changed state.
 *
 * @return Returns the Detailed tuner state.
 */
	const std::map<std::string, DetailedTunerState>  & getTunerDetailedStates(void) const {
		return tunerDetailedStates;
	}

private:
	std::map<std::string, DetailedTunerState> tunerDetailedStates;
};

/**
 * @brief Implements a Notification message when a pre tune has performed.
 * @ingroup TRM_MESSAGE_IFACE
 */
class NotifyTunerPretune : public NotificationBase
{
public:
	static const char *klassName(void) { return Klass::kNotifyTunerPretune; }

	NotifyTunerPretune(const std::string &uuid = "")
	: NotificationBase(klassName(), uuid), serviceLocator(""){}

	NotifyTunerPretune(const std::string &uuid, const std::string &serviceLocator)
	: NotificationBase(klassName(), uuid),
	  serviceLocator(serviceLocator)
	  {}

	const std::string & getServiceLocator(void) const
	{
		return serviceLocator;
	}

	~NotifyTunerPretune(void){}
private:
	std::string serviceLocator;
};

class NotifyClientConnectionEvent : public NotificationBase
{

public:
	typedef NotifyClientConnectionEvent ConnectionEvent;
	static const char *klassName(void) { return Klass::kNotifyClientConnectionEvent; }

	NotifyClientConnectionEvent(const std::string &uuid = "")
	: NotificationBase(klassName(), uuid){}

	NotifyClientConnectionEvent(const std::string &uuid,const std::string &eventName, const std::string &clientIP, uint64_t timeStamp)
	: NotificationBase(klassName(), uuid),
	  eventName(eventName),clientIP(clientIP),timeStamp(timeStamp)
	  {}

	const std::string & getEventName(void) const
	{
		return eventName;
	}

	const std::string & getClientIP(void) const
	{
		return clientIP;
	}

	const uint64_t getEventTimeStamp(void) const
	{
		return timeStamp;
	}

	~NotifyClientConnectionEvent(void){}

private:
	std::string eventName;
    std::string clientIP;
    uint64_t 	timeStamp;
    
};


class GetTRMConnectionEvents : public SimpleTRMResponse
{

public:
	typedef std::list<NotifyClientConnectionEvent> ConnectionEventList;

	static const char *klassName(void) { return Klass::kGetTRMConnectionEvents; }

	GetTRMConnectionEvents(const std::string &uuid = "")
	: SimpleTRMResponse(klassName(), uuid, ResponseStatus(ResponseStatus::kOk), "") {}

	GetTRMConnectionEvents(const std::string    &uuid,
						   const ResponseStatus &status,
						   const ConnectionEventList &conEventList = ConnectionEventList())
	: SimpleTRMResponse(klassName(), uuid, status, ""),
	  conEventList(conEventList)
	{
	}

	const ConnectionEventList & getConEvents(void) const {
		return conEventList;
	}

	ConnectionEventList & getConEvents(void) {
		return conEventList;
	}

	void addConEvents(const NotifyClientConnectionEvent &event) {
		conEventList.push_back(event);
	}
		
	~GetTRMConnectionEvents(void){}

private:
	ConnectionEventList conEventList;
};



class GetAllConnectedDeviceIdsResponse : public SimpleTRMResponse
{

public:
	static const char *klassName(void) { return Klass::kGetAllConnectedDeviceIdsResponse; }

	GetAllConnectedDeviceIdsResponse(const std::string &uuid = "")
	: SimpleTRMResponse(klassName(), uuid, ResponseStatus(ResponseStatus::kOk), "") {}

	GetAllConnectedDeviceIdsResponse(const std::string    &uuid,
						   const ResponseStatus &status,
						   const std::list<std::string> &  conDeviceIDs = std::list<std::string>())
	: SimpleTRMResponse(klassName(), uuid, status, ""),
	  conDeviceIDs(conDeviceIDs)
	{
	}

	const std::list<std::string> & getDeviceIds(void) const {
		return conDeviceIDs;
	}

	void addDeviceId(const std::string &deviceId) {
		conDeviceIDs.push_back(deviceId);
	}

	void addDeviceId(const std::list<std::string> &deviceId) {
		conDeviceIDs.insert(conDeviceIDs.end(), deviceId.begin(), deviceId.end());
	}

	void print(void) const {
		SimpleTRMResponse::print();
		std::list<std::string>::const_iterator it;
		for (it = conDeviceIDs.begin(); it != conDeviceIDs.end(); it++) {
			std::cout << "[OBJ][" << klassName() << "]conDeviceIDs = " << *it << std::endl;
		}
	}
	~GetAllConnectedDeviceIdsResponse(void){}

private:
	std::list<std::string> conDeviceIDs;
};



class GenerateAuthTokenResponse {
public:
	std::string deviceId;
	std::string authToken;
};

class GenerateAuthTokenResponseFromAuthService {
public:
	std::string deviceId;
	std::string partnerId;
};

TRM_END_NAMESPACE

#endif


/** @} */
/** @} */
