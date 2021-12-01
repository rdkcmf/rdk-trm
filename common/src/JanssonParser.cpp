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


#include <stdint.h>
#include <stdlib.h>
#include <vector>
#include <memory>
#include <limits>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "jansson.h"

#include "trm/TRM.h"
#include "trm/Klass.h"
#include "trm/Messages.h"

#include "ReservationCustomAttributes.h"
#include "Util.h"
#include "safec_lib.h"

#define DEVICEID_SCRIPT_PATH "/lib/rdk/getDeviceId.sh"
#define SCRIPT_OUTPUT_BUFFER_SIZE 512

TRM_BEGIN_NAMESPACE

static void json_dump_error(const std::string & title, const json_error_t &error);
static int  vector_dump_callback(const char *buffer, size_t size, void *data);
static int  vector_load_callback(const char *buffer, size_t size, void *data);

static void JsonEncode(const Activity &r, json_t *parent);
static void JsonDecode(json_t * parent, Activity& r);

static void JsonEncode(const TunerReservation &r, json_t *parent);
static void JsonDecode(json_t * parent, TunerReservation & reservation);

static void JsonEncode(const DetailedTunerState &r, json_t *parent);
static void JsonDecode(json_t * parent, DetailedTunerState & reservation);


static void JsonEncode(const NotifyClientConnectionEvent &r, json_t *parent);
static void JsonDecode(json_t * parent, NotifyClientConnectionEvent & notification);


/*
 * Classes MsgT that are identical to SimpleTRMRequest will invoke
 * JsonEncode(MsgT&r, out)     -- > JsonEncode(r,out, int=0)     --> JsonEncode(r, parent);
 * JsonDecode(handle, MsgT&r)  -- > JsonEncode(handle, r, int=0);
 */
static void JsonEncode(const SimpleTRMRequest & r, std::vector<uint8_t> &out, int);
static void JsonEncode(const SimpleTRMRequest &r, json_t *parent);

static void JsonDecode(int handle, SimpleTRMRequest & message, int);
static void JsonDecode(json_t *parent, SimpleTRMRequest & message);

static void JsonEncode(const SimpleTRMResponse &r, json_t *parent);
static void JsonDecode(json_t *parent, SimpleTRMResponse & message);

static void json_dump_error(const std::string & title, const json_error_t &error)
{
    std::cout << title << "============================" << std::endl;
    std::cout << "Line : " << error.line << "Column : " << error.column << "Position : " << error.position << std::endl;
    std::cout << "Source : " <<  error.source << std::endl;
    std::cout << "Text : " <<  error.text << std::endl;
}


static int vector_dump_callback(const char *buffer, size_t size, void *data)
{
	std::vector<uint8_t> *out = (std::vector<uint8_t> *)data;
	out->insert(out->end(), buffer, (buffer+size));
	return 0;
}

static int vector_load_callback(const char *buffer, size_t size, void *data)
{
	std::vector<uint8_t> *in = (std::vector<uint8_t> *)data;
	in->insert(in->end(), buffer, (buffer+size));
	return 0;
}

/*
 *
 * Activity :=
 * {
 *   "name"             : [String] name,
 *   "details"          : <Details>,
 * }
 *
 * Details :=
 * {
 *    "recordingId"     : [long] recordingId,
 * }
 *
 */

static void JsonEncode(const Activity &r, json_t *parent)
{
	json_t *JT_activity = parent;

	//Add Parent
	{
		json_object_set_new(JT_activity, "name",             json_string(r.getActivity()));
		if (r.hasDetails()) {
			json_object_set_new(JT_activity, "details",          json_object());

			json_t *JT_details = json_object_get(JT_activity, "details");
			//Encode Child
			{
#if 1
				/* Iterate through details and echo back the (key, value) pair */
				const std::map<Activity::KeyT,Activity::ValT> details = r.getDetails();
				std::map<Activity::KeyT,Activity::ValT>::const_iterator it = details.begin();
				while(it != details.end()) {
					std::cout << "[ENC]Adding Activity(" << it->first << ", " << it->second << ") " << std::endl;
					json_object_set_new(JT_details, (it->first).c_str(), json_string((it->second).c_str()));
					it++;
				}
#else
				if (r == Activity::kRecord) {
					//uint32_t recordingId = atol(r.getDetail("recordingId").c_str());
					json_object_set_new(JT_details, "recordingId", json_string(r.getDetail("recordingId").c_str()));
				}
				else if (r == Activity::kLive) {

				}
				else if (r == Activity::kEAS) {

				}
#endif
			}
		}
	}
}

static void JsonDecode(json_t * parent, Activity& r)
{
	//Decode parent
	{
		json_t *JT_activity = parent;
		{
			json_t *JT_name 	 = json_object_get(JT_activity, "name");
			const char * name    = json_string_value(JT_name);
			std::cout << "[DEC][Activity] name = "<< name << std::endl;

			r = Activity(name);

			json_t *JT_details 	= json_object_get(JT_activity, "details");
			//Decode child.
			{
				const char *key;
				json_t *value;
				json_object_foreach(JT_details, key, value) {
				r.addDetail(key, json_string_value(value));
				std::cout << "[DEC][Details]"<< key << " = " << json_string_value(value) << std::endl;
				}
			}
#if 1
				/* Enable to test EAS feature */
				struct stat buf;
				errno_t safec_rc = -1;
				int ind = -1;
				if (stat("/tmp/testTRMEAS", &buf) == 0) {
					safec_rc = strcmp_s("Live", strlen("Live"), name, &ind);
					ERR_CHK(safec_rc);
					if((safec_rc == EOK) && (ind == 0)) {
						std::cout << "Test Live Tune with EAS flag" << std::endl;
						r.addDetail("eas", "true");
					}
				}
#endif

		}
	}
}

/*
 * TunerReservation :=
 * {
 *   "reservationToken"      : [String] 	 reservationToken,
 *   "device"    		     : [String] 	 device,
 *   "serviceLocator"        : [String]      sourceLocator,
 *   "startTime" (optional)  : [long long]   startTime,
 *   "duration"  (optional)  : [long long]   duration
 *   "activity"              : <Activity>
 *   "customAttributes(optional): <CustomAttributes>
 * }
 */
static void JsonEncode(const TunerReservation &r, json_t *parent)
{
	json_t *JT_tunerReservation = parent;

	//Add child
	{
		if (!r.getReservationToken().empty())
		json_object_set_new(JT_tunerReservation, "reservationToken", json_string(r.getReservationToken().c_str()));
		json_object_set_new(JT_tunerReservation, "device",    		 json_string(r.getDevice().c_str()));
		json_object_set_new(JT_tunerReservation, "serviceLocator",   json_string(r.getServiceLocator().c_str()));
		json_object_set_new(JT_tunerReservation, "startTime",    	 json_integer(r.getStartTime()));
		json_object_set_new(JT_tunerReservation, "duration",    	 json_integer(r.getDuration()));
		json_object_set_new(JT_tunerReservation, "activity",         json_object());
		if (r.getCustomAttributes()) {
			json_object_set(JT_tunerReservation, "customAttributes", r.getCustomAttributes()->getObject());
		}
        else {
            /* Setting to json null if customAttributes is NULL*/
			json_object_set(JT_tunerReservation, "customAttributes", json_null());
        }
		//@TODO: Asser sizeof(json_int_t) == sizeof(uint64_t)

		//Encode grandchild
		{
			json_t *JT_activity 	= json_object_get(JT_tunerReservation, "activity");
			JsonEncode(r.getActivity(), JT_activity);
		}
	}
}

static void JsonDecode(json_t * parent, TunerReservation & reservation)
{
	//Decode parent
	{
		json_t *JT_reservation = parent;
		{
			json_t *JT_reservationToken 	= json_object_get(JT_reservation, "reservationToken");
			json_t *JT_device 				= json_object_get(JT_reservation, "device");
			json_t *JT_serviceLocator 		= json_object_get(JT_reservation, "serviceLocator");
			json_t *JT_startTime 			= json_object_get(JT_reservation, "startTime");
			json_t *JT_duration 			= json_object_get(JT_reservation, "duration");
			json_t *JT_activity 			= json_object_get(JT_reservation, "activity");
			json_t *JT_customAttributes		= json_object_get(JT_reservation, "customAttributes");

			const char* reservationToken    = json_string_value (JT_reservationToken);
			const char* device     			= json_string_value (JT_device);
			const char* serviceLocator      = json_string_value (JT_serviceLocator);
			uint64_t startTime    		    = (JT_startTime ? json_integer_value(JT_startTime) : GetCurrentEpoch());
			uint64_t duration     			= (JT_duration  ? json_integer_value(JT_duration ) : std::numeric_limits<uint32_t>::min());

			ReservationCustomAttributes *
			      customAttributes          = (JT_customAttributes ? ((!json_is_null(JT_customAttributes)) ? (new ReservationCustomAttributes(JT_customAttributes)) : 0) : 0);

			if (reservationToken)
			std::cout << "[DEC][TunerReservation] reservationToken = "	<< reservationToken << std::endl;
			std::cout << "[DEC][TunerReservation] device = "			<< device 			<< std::endl;
			std::cout << "[DEC][TunerReservation] serviceLocator = "	<< serviceLocator	<< std::endl;
			std::cout << "[DEC][TunerReservation] startTime = "			<< startTime 		<< std::endl;
			if (JT_duration)
			std::cout << "[DEC][TunerReservation] duration = "			<< duration 		<< std::endl;
			else
			std::cout << "[DEC][TunerReservation] duration = "			<< "[To be Adjusted]"		<< std::endl;
			std::cout << "[DEC][TunerReservation] customAttriburtes = "	<< (customAttributes ? "Present" : "Not Present") << std::endl;

			//Decode child.
			{
				Activity activity;
				JsonDecode(JT_activity, activity);

				if (JT_duration == NULL && GetSpecVersion() > SpecVersion(2, 1)) {
					/* duration is optional for Live */
					if (activity == Activity::kLive) {
						/* Leave enough room to avoid overflow handling */
						Assert(std::numeric_limits<int64_t>::max() > 2 * GetCurrentEpoch());
						duration = std::numeric_limits<int64_t>::max() - 2 * GetCurrentEpoch();
					}
				}

				std::cout << "[DEC][TunerReservation] duration [Adjusted] = " << duration << std::endl;

				reservation = TunerReservation(device ? device : "",
						                         serviceLocator,
						                         startTime,
						                         duration,
						                         activity,
						                         reservationToken ? reservationToken : "",
						                         customAttributes);
			}

		}
	}
}

/*
 *   DetailedTunerState := {
 *       "state" : <State>
 *       “serviceLocator" : [String] sourceLocator
 *       "owners" : {
 *            <Activity> : {
 *               "device" : [String] device
 *            }
 *            ...
 *       }
 *   }
 */
static void JsonEncode(const DetailedTunerState &r, json_t *parent)
{
	json_t *JT_detailedTunerState = parent;

	//Add child
	{
		json_object_set_new(JT_detailedTunerState, "state", json_string(r.getState().c_str()));
		if (!r.getServiceLocator().empty()) {
			json_object_set_new(JT_detailedTunerState, "serviceLocator", json_string(r.getServiceLocator().c_str()));
			json_object_set_new(JT_detailedTunerState, "owners", json_object());
			{
				json_t *JT_owners = json_object_get(JT_detailedTunerState, "owners");
				const std::map<std::string, std::string> & owners = r.getOwners();
				std::map<std::string, std::string>::const_iterator ito;
				for (ito = owners.begin(); ito != owners.end(); ito++) {
                    json_object_set_new(JT_owners, ito->first.c_str(), json_object());
                    {
                        json_t *JT_ownersActivity = json_object_get(JT_owners, ito->first.c_str());
                        json_object_set_new(JT_ownersActivity, "device", json_string(ito->second.c_str()));
                    }
				}
			}
		}
		if (!r.getReservedDeviceId().empty()) {
		    json_object_set_new(JT_detailedTunerState, "reservedDeviceId", json_string(r.getReservedDeviceId().c_str()));
		}
		else {
        }
	}
}

static void JsonDecode(json_t * parent, DetailedTunerState & detailedState)
{
	//Decode parent
        json_t *JT_reservation = parent;
	json_t *JT_state 				= json_object_get(JT_reservation, "state");
	json_t *JT_serviceLocator 		= json_object_get(JT_reservation, "serviceLocator");
	json_t *JT_owners 			    = json_object_get(JT_reservation, "owners");
	json_t *JT_reservedDeviceId   	= json_object_get(JT_reservation, "reservedDeviceId");

	const char* state    			= json_string_value (JT_state);
	const char* serviceLocator     	= JT_serviceLocator ? json_string_value (JT_serviceLocator) : "";

	std::cout << "[DEC][DetailedTunerState] state = "			<< state << std::endl;
	if (JT_serviceLocator) {
		std::cout << "[DEC][DetailedTunerState] serviceLocator = "	<< serviceLocator	<< std::endl;
	}
	detailedState.setState(state, serviceLocator);

	//Decode child.
	if (JT_owners)
	{
		//Assert(json_object_size(JT_owners) >= 1);
		const char *key;
		json_t *value;
		json_object_foreach(JT_owners, key, value){
			const char *activity = key;
			json_t *JT_ownersActivity = value;
                        json_t *JT_ownerActivityDevice = json_object_get(JT_ownersActivity, "device");
			const char *device      = json_string_value(JT_ownerActivityDevice);
			if(device == 0) {
				std::cout << "[ERROR][JsonDecode : ReserveTuner] NULL Device "<< std::endl;  //CID:18250 - Forward null issue
				return;
			}
			std::cout << "[DEC][DetailedTunerState][owners][" << activity << "]"
						"[device] = " << device << std::endl;
			detailedState.addTunerOwner(activity, device);
		}
	}

	if (JT_reservedDeviceId) {
		const char* reservedDeviceId    = json_string_value (JT_reservedDeviceId);
		detailedState.setReservedDeviceId(reservedDeviceId);
		std::cout << "[DEC][DetailedTunerState] reservedDeviceId = "	<< reservedDeviceId	<< std::endl;
	}
	else {
		std::cout << "[DEC][DetailedTunerState] reservedDeviceId = "	<< "NONE" << std::endl;
	}
}

int JsonDecode(const std::vector<uint8_t> &in, Enum<Klass> &klass)
{

	json_error_t error;
	json_t * parent = json_loadb((const char *)&in[0], in.size(), JSON_DISABLE_EOF_CHECK, &error);

	if (parent == 0) {
		json_dump_error("Load Error", error);
	}
	else
	//Decode parent
	{
		const char *key;
		json_t *value;
		json_object_foreach(parent, key, value){
			//First key is "className", first value is json object.
			Assert(json_object_size(parent) == 1);
			klass = Klass(key).getClass();
		}
	}

	return (int)parent;
}

/*
 * "reserveTuner" =:
 * {
 *   "requestId"        : [String] requestId,
 *   "device"           : [String] device,
 *   "tunerReservation" : <TunerReservation>
 * }
 */
void JsonEncode(const ReserveTuner &r, std::vector<uint8_t> &out)
{
	json_t * parent = json_object();

	//Add parent
	{
		json_object_set_new(parent, r.getClassName().c_str(), json_object());

		//Add child
		{
			json_t *JT_reserveTuner = json_object_get(parent,  r.getClassName().c_str());

			json_object_set_new(JT_reserveTuner, "requestId", 			   json_string(r.getUUID().c_str()));
			json_object_set_new(JT_reserveTuner, "device",    			   json_string(r.getDevice().c_str()));
			json_object_set_new(JT_reserveTuner, "tunerReservation",       json_object());
			json_object_set_new(JT_reserveTuner,  "resurrect",   			json_string(r.getResurrect().c_str()));

			//Encode grandchild
			{
				JsonEncode(r.getTunerReservation(), json_object_get(JT_reserveTuner, "tunerReservation"));
			}
		}
	}

	//@TODO: only enable JSON_PRESERVE_ORDER for debug builds.
	json_dump_callback(parent, vector_dump_callback, &out, JSON_INDENT(4) | JSON_PRESERVE_ORDER);
	json_decref(parent);
}

void JsonDecode(int handle, ReserveTuner & message)
{
	json_t * parent = (json_t *)handle;
	
	if (parent == 0) {
		std::cout << "[ERROR][JsonDecode : ReserveTuner] NULL Handle "<< std::endl;
	}
	else
	//Decode parent
	{
		const char *key;
		json_t *value;
		json_object_foreach(parent, key, value){
			Assert(json_object_size(parent) == 1);
			json_t *JT_reserveTuner = value;
			{
				json_t *JT_requestId 			= json_object_get(JT_reserveTuner, "requestId");
				json_t *JT_device 				= json_object_get(JT_reserveTuner, "device");
				json_t *JT_tunerReservation		= json_object_get(JT_reserveTuner, "tunerReservation");
				json_t *JT_resurrect 			= json_object_get(JT_reserveTuner, "resurrect");

				const char *requestId = json_string_value(JT_requestId);
				const char *device    = json_string_value(JT_device);
				const char *resurrect = (JT_resurrect ? json_string_value(JT_resurrect) : "false");

				Assert(resurrect != 0);
				if(device == 0) {
					std::cout << "[ERROR][JsonDecode : ReserveTuner] NULL Device "<< std::endl;  //CID:18576 - Forward null issue
					return;
				}
				std::cout << "[DEC][ReserveTuner] requestId = "<< requestId 					<< std::endl;
				std::cout << "[DEC][ReserveTuner] device = "	<< device << std::endl;
				std::cout << "[DEC][ReserveTuner] resurrect = "<< resurrect 					<< std::endl;
				
				{
					TunerReservation tunerReservation;
					JsonDecode(JT_tunerReservation, tunerReservation);
					message = ReserveTuner(requestId, device, tunerReservation,resurrect);
				}
			}
		}
		json_decref(parent);
	}
}

/*
 * "reserveTunerResponse" =:
 * {
 *   "requestId"        			: [String] requestId,
 *   "status" 					 	: [String] status,
 *   "statusMessage"  (optional)	: [String] statusMessage
 *   "tunerReservation" 			: <TunerReservation>
 *   "conflicts"					: [ [<TunerReservation>, ...], ...]
 * }
 */
void JsonEncode(const ReserveTunerResponse &r, std::vector<uint8_t> &out)
{
	json_t * parent = json_object();

	//Add parent
	{
		json_object_set_new(parent, r.getClassName().c_str(), json_object());

		//Add child
		{
			json_t *JT_reserveTunerResponse = json_object_get(parent,  r.getClassName().c_str());

			json_object_set_new(JT_reserveTunerResponse, "requestId", 		json_string(r.getUUID().c_str()));
			json_object_set_new(JT_reserveTunerResponse, "status",    		json_string(r.getStatus().getStatusCode()));
			json_object_set_new(JT_reserveTunerResponse, "statusMessage",   json_string(r.getStatus().getDetails().c_str()));
			if(!r.getTunerReservation().getReservationToken().empty())
			json_object_set_new(JT_reserveTunerResponse, "tunerReservation",json_object());
			if(r.getConflicts().size())
			json_object_set_new(JT_reserveTunerResponse, "conflicts",json_array());

			//Encode grandchild
			{
				json_t *JT_tunerReservation = json_object_get(JT_reserveTunerResponse, "tunerReservation");
				if (JT_tunerReservation) {
				    JsonEncode(r.getTunerReservation(), JT_tunerReservation);
				}

				//Conflicts is a List of List of Reservations.
				//Each sublist represents all reservations from a same tuner.

				json_t *JT_conflicts = json_object_get(JT_reserveTunerResponse, "conflicts");
				if (JT_conflicts)
				{ //
					typedef ReserveTunerResponse::ConflictCT ConflictsCT;
					typedef ReserveTunerResponse::ReservationCT ReservationCT;

					const ConflictsCT & conflicts = r.getConflicts();
					ConflictsCT::const_iterator itc;
					for (itc = conflicts.begin(); itc != conflicts.end(); itc++) {
						json_t *JT_singleReservation = json_object();
						JsonEncode(*itc, JT_singleReservation);
						json_array_append_new(JT_conflicts, JT_singleReservation);
					}
				}
			}
		}
	}

	//@TODO: only enable JSON_PRESERVE_ORDER for debug builds.
	json_dump_callback(parent, vector_dump_callback, &out, JSON_INDENT(4) | JSON_PRESERVE_ORDER);
	json_decref(parent);
}

void JsonDecode(int handle, ReserveTunerResponse & message)
{
	json_t * parent = (json_t *)handle;
	
	if (parent == 0) {
		std::cout << "[ERROR][JsonDecode : ReserveTunerResponse] NULL Handle "<< std::endl;
	}
	else
	//Decode parent
	{
		const char *key;
		json_t *value;
		json_object_foreach(parent, key, value){
			//first key is "className", first value is json object.
			Assert(json_object_size(parent) == 1);
			json_t *JT_reserveTunerResponse = value;
			{
				json_t *JT_requestId 			= json_object_get(JT_reserveTunerResponse, "requestId");
				json_t *JT_status 				= json_object_get(JT_reserveTunerResponse, "status");
				json_t *JT_statusMessage		= json_object_get(JT_reserveTunerResponse, "statusMessage");
				json_t *JT_tunerReservation		= json_object_get(JT_reserveTunerResponse, "tunerReservation");
				json_t *JT_conflicts    		= json_object_get(JT_reserveTunerResponse, "conflicts");

				const char *requestId 		= json_string_value(JT_requestId);
				const char *status 			= json_string_value(JT_status);
				const char *statusMessage 	= json_string_value(JT_statusMessage);

				if (/***/JT_requestId) {
					std::cout << "[DEC][ReserveTunerResponse] requestId = "		<< requestId	<< std::endl;
				}
				if (/***/JT_status) {
					std::cout << "[DEC][ReserveTunerResponse] status = "		<< status		<< std::endl;
				}
				if (/***/JT_statusMessage) {
					std::cout << "[DEC][ReserveTunerResponse] statusMessage = "	<< statusMessage<< std::endl;
				}
				if (/***/JT_tunerReservation){
					TunerReservation tunerReservation;
					JsonDecode(JT_tunerReservation, tunerReservation);
					//TODO: message is reinit'd by assignment here, what happens if JT_tunerReservation is 0 but JT_conflicts
					message = ReserveTunerResponse(requestId, ResponseStatus(status, statusMessage), tunerReservation);
				}
				else {
					message = ReserveTunerResponse(requestId, ResponseStatus(status, statusMessage), TunerReservation());
				}
				if (/***/JT_conflicts) {
					//conflicts is a list of list.
					int numOfConflicts = json_array_size(JT_conflicts);

					for (int i = 0; i < numOfConflicts; i++) {
						json_t *JT_singleReservation = json_array_get(JT_conflicts, i);
						if (/***/JT_singleReservation) {
							TunerReservation tunerReservation;
							JsonDecode(JT_singleReservation, tunerReservation);
							message.addConflict(tunerReservation);
						}
					}
				}
			}
		}
		json_decref(parent);
	}
}
/*
 * Messages with the following format are Simple TRM Requests.
 *
 * <Message Name> =:
 * {
 *   "requestId"         : [String] requestId,
 *   "device" (optional) : [String] device,
 *   "reservationToken"  : [String] reservationToken,
 * }
 *
 * Known TRM Requests:
 *
 * - releaseTunerReservation
 * - validateTunerReservation
 * - cancelRecording
 *
 */
static void JsonEncode(const SimpleTRMRequest &r, json_t *parent)
{
	json_t *JT_simpleRequest = parent;

	//Add parent
	{
		json_object_set_new(JT_simpleRequest, "requestId", 			json_string(r.getUUID().c_str()));
		if (!r.getDevice().empty())
		json_object_set_new(JT_simpleRequest, "device",    			json_string(r.getDevice().c_str()));
		if (!r.getReservationToken().empty())
		json_object_set_new(JT_simpleRequest, "reservationToken",   json_string(r.getReservationToken().c_str()));
	}
}

static void JsonEncode(const SimpleTRMRequest & r, std::vector<uint8_t> &out, int)
{
	json_t * parent = json_object();

	if (parent != 0)
	{//Add parent
		json_object_set_new(parent, r.getClassName().c_str(), json_object());
		json_t *JT_request = json_object_get(parent,  r.getClassName().c_str());

		//Add child
		{
			JsonEncode(r, JT_request);
		}

		//@TODO: only enable JSON_PRESERVE_ORDER for debug builds.
		json_dump_callback(parent, vector_dump_callback, &out, JSON_INDENT(4) | JSON_PRESERVE_ORDER);
		json_decref(parent);
	}
}

static void JsonDecode(json_t *parent, SimpleTRMRequest & message)
{
	if (parent == 0) {
	}
	else
	//Decode parent
	{
		{
			json_t *JT_simpleRequest = parent;

			json_t *JT_requestId 			= json_object_get(JT_simpleRequest, "requestId");
			json_t *JT_device 				= json_object_get(JT_simpleRequest, "device");
			json_t *JT_reservationToken		= json_object_get(JT_simpleRequest, "reservationToken");

			const char *requestId = json_string_value(JT_requestId);
			const char *device    = json_string_value(JT_device);
			const char *reservationToken = json_string_value(JT_reservationToken);
			std::cout << "[DEC][" << SimpleTRMRequest::klassName() << "]requestId = "		<< requestId 					<< std::endl;
			std::cout << "[DEC][" << SimpleTRMRequest::klassName() << "]device : "			<< (device != 0 ? device : "") 	<< std::endl;
			if (reservationToken != 0)
			std::cout << "[DEC][" << SimpleTRMRequest::klassName() << "]reservationToken = "	<< reservationToken 			<< std::endl;

			message = SimpleTRMRequest(message.getClassName(), requestId, (device != 0 ? device : ""), (reservationToken != 0 ? reservationToken : ""));
		}
	}
}

static void JsonDecode(int handle, SimpleTRMRequest & message, int)
{
	json_t * parent = (json_t *)handle;
	if (parent == 0) {

	}
	else {
		const char *key;
		json_t *value;

		json_object_foreach(parent, key, value) {
			//First key is "className", first value is json object.
			Assert(json_object_size(parent) == 1);
			json_t *JT_simpleRequest = value;
			{
				JsonDecode(JT_simpleRequest, message);
			}
		}
		json_decref(parent);
	}
}

void JsonEncode(const ReleaseTunerReservation & r, std::vector<uint8_t> &out)
{
	JsonEncode(r, out, 0/*Dummy*/);
}

void JsonDecode(int handle, ReleaseTunerReservation & message)
{
	JsonDecode(handle, message, 0/*Dummy*/);
}

void JsonEncode(const ValidateTunerReservation & r, std::vector<uint8_t> &out)
{
	JsonEncode(r, out, 0/*Dummy*/);
}

void JsonDecode(int handle, ValidateTunerReservation & message)
{
	JsonDecode(handle, message, 0/*Dummy*/);
}

void JsonEncode(const CancelRecording & r, std::vector<uint8_t> &out)
{
	JsonEncode(r, out, 0/*Dummy*/);
}

void JsonDecode(int handle, CancelRecording & message)
{
	JsonDecode(handle, message, 0/*Dummy*/);
}

/*
 * Messages with the following format are Simple TRM Responses.
 *
 * <Message Name> =:
 * {
 *   "requestId"         		 : [String] requestId,
      "status" 					 : [String] status,
      "statusMessage"  (optional): [String] statusMessage
 *   "reservationToken"  	     : [String] reservationToken,
 * }
 *
 * Known TRM Responses:
 *
 * - releaseTunerReservationResponse
 * - validateTunerReservationResponse
 * - cancelRecordingResponse
 *
 */
static void JsonEncode(const SimpleTRMResponse &r, json_t *parent)
{
	json_t *JT_simpleResponse = parent;
	//Add parent
	{
			json_object_set_new(JT_simpleResponse,   "requestId",  		json_string(r.getUUID().c_str()));
			json_object_set_new(JT_simpleResponse,   "status",    		json_string(r.getStatus().getStatusCode()));
			if (!r.getStatus().getDetails().empty())
			json_object_set_new(JT_simpleResponse,   "statusMessage",   json_string(r.getStatus().getDetails().c_str()));
			if (!r.getReservationToken().empty())
			json_object_set_new(JT_simpleResponse,   "reservationToken",json_string(r.getReservationToken().c_str()));
	}
}

static void JsonDecode(json_t *parent, SimpleTRMResponse & message)
{
	if (parent == 0) {
	}
	else
	//Decode parent
	{
		json_t *JT_simpleResponse = parent;
		{
			json_t *JT_requestId 			= json_object_get(JT_simpleResponse, "requestId");
			json_t *JT_status 				= json_object_get(JT_simpleResponse, "status");
			json_t *JT_statusMessage 		= json_object_get(JT_simpleResponse, "statusMessage");
			json_t *JT_reservationToken		= json_object_get(JT_simpleResponse, "reservationToken");


			const char *requestId 			= json_string_value(JT_requestId);
			const char *status    			= json_string_value(JT_status);
			const char *statusMessage   	= json_string_value(JT_statusMessage);
			const char *reservationToken   	= json_string_value(JT_reservationToken);

			std::cout << "[DEC][" << SimpleTRMResponse::klassName() << "]requestId = "		<< requestId 		<< std::endl;
			std::cout << "[DEC][" << SimpleTRMResponse::klassName() << "]status = "			<< status 			<< std::endl;
			if (statusMessage)
			std::cout << "[DEC][" << SimpleTRMResponse::klassName() << "]statusMessage = "	<< statusMessage 	<< std::endl;
			if (reservationToken)
			std::cout << "[DEC][" << SimpleTRMResponse::klassName() << "]reservationToken = "	<< reservationToken << std::endl;

			message = SimpleTRMResponse(message.getClassName(), requestId, ResponseStatus(status, statusMessage == 0 ? "" : statusMessage), reservationToken != 0 ? reservationToken : "");
		}
	}
}

void JsonEncode(const ReleaseTunerReservationResponse &r, std::vector<uint8_t> &out)
{
	json_t * parent = json_object();
	//Add parent
	if (parent == 0) {

	}
	else
	{//Add parent
		json_object_set_new(parent, r.getClassName().c_str(), json_object());
		//Add child
		{
			json_t *JT_response = json_object_get(parent,  r.getClassName().c_str());
			JsonEncode(r, JT_response);
			json_object_set_new(JT_response, "released",json_string(r.isReleased() ? "true" : "false"));
		}

		json_dump_callback(parent, vector_dump_callback, &out, JSON_INDENT(4) | JSON_PRESERVE_ORDER);
		json_decref(parent);
	}

}

void JsonDecode(int handle, ReleaseTunerReservationResponse & message)
{
	json_t * parent = (json_t *)handle;
	if (parent == 0) {
	}
	else
	//Decode parent
	{
		const char *key;
		json_t *value;
		json_object_foreach(parent, key, value){
			//first key is "className", first value is json object.'
			Assert(json_object_size(parent) == 1);
			json_t *JT_simpleResponse = value;
			{
				JsonDecode(JT_simpleResponse, message);
				json_t *JT_released 			= json_object_get(JT_simpleResponse, "released");
				const char *released 			= json_string_value(JT_released);
				bool isReleased = (std::string(released).compare("true") == 0);
				message.setReleased(isReleased);
			}
		}
		json_decref(parent);
	}
}

void JsonEncode(const ValidateTunerReservationResponse &r, std::vector<uint8_t> &out)
{
	json_t * parent = json_object();
	//Add parent
	if (parent == 0) {

	}
	else
	{//Add parent
		json_object_set_new(parent, r.getClassName().c_str(), json_object());
		//Add child
		{
			json_t *JT_response = json_object_get(parent,  r.getClassName().c_str());
			JsonEncode(r, JT_response);
			json_object_set_new(JT_response, "valid",json_string(r.isValid() ? "true" : "false"));
		}

		json_dump_callback(parent, vector_dump_callback, &out, JSON_INDENT(4) | JSON_PRESERVE_ORDER);
		json_decref(parent);
	}

}

void JsonDecode(int handle, ValidateTunerReservationResponse & message)
{
	json_t * parent = (json_t *)handle;
	if (parent == 0) {
	}
	else
	//Decode parent
	{
		const char *key;
		json_t *value;
		json_object_foreach(parent, key, value){
			//first key is "className", first value is json object.
			Assert(json_object_size(parent) == 1);
			json_t *JT_simpleResponse = value;
			{
				JsonDecode(JT_simpleResponse, message);
				json_t *JT_valid 			= json_object_get(JT_simpleResponse, "valid");
				const char *valid 			= json_string_value(JT_valid);
				bool isValid = (std::string(valid).compare("true") == 0);
				message.setValid(isValid);
			}
		}
		json_decref(parent);
	}
}


void JsonEncode(const CancelRecordingResponse &r, std::vector<uint8_t> &out)
{
	json_t * parent = json_object();
	//Add parent
	if (parent == 0) {

	}
	else
	{//Add parent
		json_object_set_new(parent, r.getClassName().c_str(), json_object());
		//Add child
		{
			json_t *JT_response = json_object_get(parent,  r.getClassName().c_str());
			JsonEncode(r, JT_response);
			json_object_set_new(JT_response, "cancelled",json_string(r.isCanceled() ? "true" : "false"));
		}

		json_dump_callback(parent, vector_dump_callback, &out, JSON_INDENT(4) | JSON_PRESERVE_ORDER);
		json_decref(parent);
	}

}

void JsonDecode(int handle, CancelRecordingResponse & message)
{
	json_t * parent = (json_t *)handle;
	if (parent == 0) {
	}
	else
	//Decode parent
	{
		const char *key;
		json_t *value;
		json_object_foreach(parent, key, value){
			//first key is "className", first value is json object.
			Assert(json_object_size(parent) == 1);
			json_t *JT_simpleResponse = value;
			{
				JsonDecode(JT_simpleResponse, message);
				json_t *JT_canceled 			= json_object_get(JT_simpleResponse, "cancelled");
				const char *canceled 			= JT_canceled ? json_string_value(JT_canceled) : "";
				bool isCanceled = (std::string(canceled).compare("true") == 0);
				message.setCanceled(isCanceled);
			}
		}
		json_decref(parent);
	}
}

/*
 * "getAllTunerIds" =:
 * {
 *   "requestId"        : [String] requestId,
 *   "device"           : [String] device,
 * }
 */
void JsonEncode(const GetAllTunerIds &r, std::vector<uint8_t> &out)
{
	JsonEncode(r, out, 0/*Dummy*/);
}

void JsonDecode(int handle, GetAllTunerIds & message)
{
	JsonDecode(handle, message, 0/*Dummy*/);
}

/*
 * "getAllTunerIdsResponse" =:
 * {
 *   "requestId"         		 : [String] requestId,
 *    "status" 					 : [String] status,
 *    "statusMessage"  (optional): [String] statusMessage
 *    "tunerIds"                 ; [<TunerId>, ... ]
 * }
 */
void JsonEncode(const GetAllTunerIdsResponse &r, std::vector<uint8_t> &out)
{
	json_t * parent = json_object();
	//Add parent
	if (parent == 0) {

	}
	else
	{//Add parent
		json_object_set_new(parent, r.getClassName().c_str(), json_object());

		{//Add child
			json_t *JT_response = json_object_get(parent,  r.getClassName().c_str());
			JsonEncode(r, JT_response);
			json_object_set_new(JT_response, "tunerIds", json_array());

			{//Add Array Elements
				json_t *JT_tunerIds = json_object_get(JT_response,  "tunerIds");
				const std::list<std::string> & tunerIds = r.getTunerIds();
				std::list<std::string>::const_iterator it;
				for (it = tunerIds.begin(); it != tunerIds.end(); it++) {
					json_array_append_new(JT_tunerIds, json_string(it->c_str()));
				}
			}
		}

		json_dump_callback(parent, vector_dump_callback, &out, JSON_INDENT(4) | JSON_PRESERVE_ORDER);
		json_decref(parent);
	}

}

void JsonDecode(int handle, GetAllTunerIdsResponse & message)
{
	json_t * parent = (json_t *)handle;
	if (parent == 0) {
	}
	else
	//Decode parent
	{
		const char *key;
		json_t *value;
		json_object_foreach(parent, key, value){
			//first key is "className", first value is json object.
			Assert(json_object_size(parent) == 1);
			json_t *JT_simpleResponse = value;
			{
				JsonDecode(JT_simpleResponse, message);
				json_t *JT_tunerIds 			= json_object_get(JT_simpleResponse, "tunerIds");
				int numOfTuners = json_array_size(JT_tunerIds);
				for (int i = 0; i < numOfTuners; i++) {
					json_t *JT_id = json_array_get(JT_tunerIds, i);
					message.addTunerId(json_string_value(JT_id));
				}
			}
		}
		json_decref(parent);
	}
}

void JsonEncode(const GetAllTunerStates &r, std::vector<uint8_t> &out)
{
	JsonEncode(r, out, 0/*Dummy*/);
}

void JsonDecode(int handle, GetAllTunerStates & message)
{
	JsonDecode(handle, message, 0/*Dummy*/);
}

/*
 * "TunerState" =:
 * {
 *    <tunerId>                  : <State>
 *    ...
 * }
 *
 * "getAllTunerStatesResponse" =:
 * {
 *   "requestId"         		 : [String] requestId,
 *    "status" 					 : [String] status,
 *    "statusMessage"  (optional): [String] statusMessage
 *    "allStates"                  : <TunerState>
 * }
 */
void JsonEncode(const GetAllTunerStatesResponse &r, std::vector<uint8_t> &out)
{
	json_t * parent = json_object();
	//Add parent
	if (parent == 0) {

	}
	else
	{//Add parent
		json_object_set_new(parent, r.getClassName().c_str(), json_object());

		{//Add child
			json_t *JT_response = json_object_get(parent,  r.getClassName().c_str());
			JsonEncode(r, JT_response);
			json_object_set_new(JT_response, "allStates", json_object());

			{//Add one <tunerId>: <state> tuple for each tuner
				json_t *JT_states = json_object_get(JT_response, "allStates");
				const std::map<std::string, std::string> & tunerStates = r.getTunerStates();
				std::map<std::string, std::string>::const_iterator it;
				for (it = tunerStates.begin(); it != tunerStates.end(); it++) {
					json_object_set_new(JT_states, it->first.c_str(), json_string(it->second.c_str()));
				}
			}

			json_object_set_new(JT_response, "detailedStates", json_object());
			{
				json_t *JT_detailedStates = json_object_get(JT_response, "detailedStates");
				const std::map<std::string, DetailedTunerState>  & tunerDetailedStates = r.getTunerDetailedStates();
				std::map<std::string, DetailedTunerState>::const_iterator it;
				for (it = tunerDetailedStates.begin(); it != tunerDetailedStates.end(); it++) {

					json_object_set_new(JT_detailedStates, it->first.c_str(), json_object());
					{
						json_t *JT_detailedTunerState = json_object_get(JT_detailedStates, it->first.c_str());
						{
							JsonEncode(it->second, JT_detailedTunerState);
						}
					}
				}
			}
		}

		json_dump_callback(parent, vector_dump_callback, &out, JSON_INDENT(4) | JSON_PRESERVE_ORDER);
		json_decref(parent);
	}

}

void JsonDecode(int handle, GetAllTunerStatesResponse & message)
{
	json_t * parent = (json_t *)handle;
	if (parent == 0) {
	}
	else
	//Decode parent
	{
		const char *key;
		json_t *value;
		json_object_foreach(parent, key, value){
			//first key is "className", first value is json object.
			Assert(json_object_size(parent) == 1);
			json_t *JT_simpleResponse = value;
			{
				JsonDecode(JT_simpleResponse, message);
				json_t *JT_states 			= json_object_get(JT_simpleResponse, "allStates");
				//size_t numOfTuners = json_object_size(JT_states);
				{//Iterate through all elements of the "states" object
					const char *stateKey;
					json_t *stateValue;
					json_object_foreach(JT_states, stateKey, stateValue){
						message.addTunerState(stateKey, json_string_value(stateValue));
					}
				}
				json_t *JT_detailedStates 	= json_object_get(JT_simpleResponse, "detailedStates");
				{//Iterate through all elements of the "detailedStates" object
					const char *key;
					json_t *value;
					json_object_foreach(JT_detailedStates, key, value){
						//first key is "className", first value is json object.
						const char *&tunerId = key;
						json_t *&JT_detailedState = value;
						{
							DetailedTunerState detailedState;
							JsonDecode(JT_detailedState, detailedState);
							message.addTunerState(tunerId, detailedState.getState(), detailedState.getServiceLocator(), detailedState.getReservedDeviceId());
						    std::map<std::string, std::string>::const_iterator owners_it = detailedState.getOwners().begin();
						    for (owners_it = detailedState.getOwners().begin(); owners_it != detailedState.getOwners().end(); owners_it++)
						    {
								message.addTunerOwner(tunerId, owners_it->first, owners_it->second);
						    }
						}
					}
				}
			}
		}
		json_decref(parent);
	}
}

/*
 *
 * Filter :=
 * {
 *   "device" (optional)     : [String] device,
 *   "activity" (optional)   : <Activity>,
 *   "tunerState" (optional) : <State>,
 * }
 *
 * "getAllReservations" =:
 * {
 *   "requestId"         		 : [String] requestId,
 *    "filters" 			     : [<Filter>,...],
 * }
 */

void JsonEncode(const GetAllReservations &r, std::vector<uint8_t> &out)
{
	json_t * parent = json_object();
	//Add parent
	if (parent == 0) {

	}
	else
	{//Add parent
		json_object_set_new(parent, r.getClassName().c_str(), json_object());

		{//Add child
			json_t *JT_request = json_object_get(parent,  r.getClassName().c_str());
			JsonEncode(r, JT_request);
			json_object_set_new(JT_request, "filters", json_array());
			{
				json_t *JT_filters = json_object_get(JT_request, "filters");
				{
					const std::map<std::string, std::string> filters = r.getFilters();
					std::map<std::string, std::string>::const_iterator it = filters.begin();
					for (it = filters.begin(); it != filters.end(); it++) {
						json_t *JT_singleFilter = json_object();
						json_object_set_new(JT_singleFilter, it->first.c_str(), json_string(it->second.c_str()));
						json_array_append_new(JT_filters, JT_singleFilter);
					}
				}
			}
		}

		json_dump_callback(parent, vector_dump_callback, &out, JSON_INDENT(4) | JSON_PRESERVE_ORDER);
		json_decref(parent);
	}
}

void JsonDecode(int handle, GetAllReservations & message)
{
	json_t * parent = (json_t *)handle;
	if (parent == 0) {
	}
	else
	//Decode parent
	{
		const char *key;
		json_t *value;
		json_object_foreach(parent, key, value){
			//first key is "className", first value is json object.
			Assert(json_object_size(parent) == 1);
			json_t *JT_simpleRequest = value;
			{
				JsonDecode(JT_simpleRequest, message);
				json_t *JT_filters = json_object_get(JT_simpleRequest, "filters");
				{//Iterate through all elements of the "filters" array
					size_t numOfFilters = json_array_size(JT_filters);
					for (int i = 0; i < (int)numOfFilters; i++) {
						json_t *JT_singleFilter = json_array_get(JT_filters, i);
						json_t *JT_deviceFilter = json_object_get(JT_singleFilter, "device");
						json_t *JT_activityFilter = json_object_get(JT_singleFilter, "activity");
						json_t *JT_stateFilter = json_object_get(JT_singleFilter, "state");

						if (JT_deviceFilter) {
							const char * device	= json_string_value(JT_deviceFilter);
							message.addFilter("device", device);
						}
						if (JT_activityFilter) {
							json_t *JT_activityFilter_name = json_object_get(JT_activityFilter, "name");
							if (JT_activityFilter_name) {
								const char * activity	= json_string_value(JT_activityFilter_name);
								message.addFilter("activity", activity);
							}
						}
						if (JT_stateFilter) {
							const char * state	= json_string_value(JT_stateFilter);
							message.addFilter("state", state);
						}

					}
				}
			}
		}
		json_decref(parent);
	}
}

/*
 * "AllReservations" =:
 * {
 *    <tunerId> : [<TunerReservation>,...]
 *    ...
 * }
 *
 * "GetAllReservationsResponse" =:
 * {
 *   "requestId"         		 : [String] requestId,
 *    "status" 					 : [String] status,
 *    "statusMessage"  (optional): [String] statusMessage
 *    "allReservations"          : <AllReservations>
 * }
 */
void JsonEncode(const GetAllReservationsResponse &r, std::vector<uint8_t> &out)
{
	json_t * parent = json_object();
	//Add parent
	if (parent == 0) {

	}
	else
	{//Add parent
		json_object_set_new(parent, r.getClassName().c_str(), json_object());

		{//Add child
			json_t *JT_response = json_object_get(parent,  r.getClassName().c_str());
			JsonEncode(r, JT_response);
			json_object_set_new(JT_response, "allReservations", json_object());

			{//Add one <tunerId>: [<TunerReservation>,...] tuple for each tuner
				json_t *JT_allReservations = json_object_get(JT_response, "allReservations");
				const std::map<std::string, std::list<TunerReservation> > & allReservations = r.getAllReservations();
				std::map<std::string, std::list<TunerReservation> >::const_iterator itm;
				for (itm = allReservations.begin(); itm != allReservations.end(); itm++) {
					json_object_set_new(JT_allReservations, itm->first.c_str(), json_array());
					{// Add each reservation to the array
						json_t *JT_reservations = json_object_get(JT_allReservations, itm->first.c_str());
						const std::list<TunerReservation> &reservations = itm->second;
						std::list<TunerReservation>::const_iterator itl;
						for (itl = reservations.begin(); itl != reservations.end(); itl++) {
							json_t *JT_singleReservation = json_object();
							JsonEncode(*itl, JT_singleReservation);
							json_array_append_new(JT_reservations, JT_singleReservation);
						}
					}
				}
			}
		}

		json_dump_callback(parent, vector_dump_callback, &out, JSON_INDENT(4) | JSON_PRESERVE_ORDER);
		json_decref(parent);
	}

}

void JsonDecode(int handle, GetAllReservationsResponse & message)
{
	json_t * parent = (json_t *)handle;
	if (parent == 0) {
	}
	else
	//Decode parent
	{
		const char *key;
		json_t *value;
		json_object_foreach(parent, key, value){
			//first key is "className", first value is json object.
			Assert(json_object_size(parent) == 1);
			json_t *JT_simpleResponse = value;
			{
				JsonDecode(JT_simpleResponse, message);
				json_t *JT_allReservations = json_object_get(JT_simpleResponse, "allReservations");
				//size_t numOfTuners = json_object_size(JT_allReservations);
				{//Iterate through all elements of the "allReservations" object
					const char *tunerId;
					json_t *JT_reservations;
					json_object_foreach(JT_allReservations, tunerId, JT_reservations){
						int numOfReservations = json_array_size(JT_reservations);
						for (int i = 0; i < numOfReservations; i++) {
							json_t *JT_singleReservation = json_array_get(JT_reservations, i);
							TunerReservation singleReservation;
							JsonDecode(JT_singleReservation, singleReservation);
							message.addTunerReservation(tunerId, singleReservation);
						}
					}
				}
			}
		}
		json_decref(parent);
	}
}

void JsonEncode(const GetVersion &r, std::vector<uint8_t> &out)
{
	JsonEncode(r, out, 0/*Dummy*/);
}

void JsonDecode(int handle, GetVersion & message)
{
	JsonDecode(handle, message, 0/*Dummy*/);
}

void JsonEncode(const GetVersionResponse &r, std::vector<uint8_t> &out)
{
	json_t * parent = json_object();
	//Add parent
	if (parent == 0) {

	}
	else
	{//Add parent
		json_object_set_new(parent, r.getClassName().c_str(), json_object());
		//Add child
		{
			json_t *JT_response = json_object_get(parent,  r.getClassName().c_str());
			JsonEncode(r, JT_response);
			json_object_set_new(JT_response, "version",json_string(r.getVersion().c_str()));
		}

		json_dump_callback(parent, vector_dump_callback, &out, JSON_INDENT(4) | JSON_PRESERVE_ORDER);
		json_decref(parent);
	}
}

void JsonDecode(int handle, GetVersionResponse & message)
{
	json_t * parent = (json_t *)handle;
	if (parent == 0) {
	}
	else
	//Decode parent
	{
		const char *key;
		json_t *value;
		json_object_foreach(parent, key, value){
			//first key is "className", first value is json object.
			Assert(json_object_size(parent) == 1);
			json_t *JT_simpleResponse = value;
			{
				JsonDecode(JT_simpleResponse, message);
				json_t *JT_version 			= json_object_get(JT_simpleResponse, "version");
				const char *version 			= json_string_value(JT_version);
				message.setVersion(version);
			}
		}
		json_decref(parent);
	}
}

void JsonEncode(const UpdateTunerActivityStatusResponse &r, std::vector<uint8_t> &out)
{
        json_t * parent = json_object();
        //Add parent
        if (parent == 0) {

        }
        else
        {//Add parent

                json_object_set_new(parent, r.getClassName().c_str(), json_object());

                {//Add child
                        json_t *JT_UpdateTunerActivityResponse = json_object_get(parent,  r.getClassName().c_str());
			json_object_set_new(JT_UpdateTunerActivityResponse,   "requestId",  		json_string(r.getUUID().c_str()));
			json_object_set_new(JT_UpdateTunerActivityResponse,   "status",    		json_string(r.getStatus().getStatusCode()));
			if (!r.getStatus().getDetails().empty())
			json_object_set_new(JT_UpdateTunerActivityResponse,   "statusMessage",   json_string(r.getStatus().getDetails().c_str()));
                }

                json_dump_callback(parent, vector_dump_callback, &out, JSON_INDENT(4) | JSON_PRESERVE_ORDER);
                json_decref(parent);
        }
}

void JsonEncode(const UpdateTunerActivityStatus &r, std::vector<uint8_t> &out)
{
        json_t * parent = json_object();
        //Add parent
        if (parent == 0) {

        }
        else
        {//Add parent

                json_object_set_new(parent, r.getClassName().c_str(), json_object());

                {//Add child
                        json_t *JT_notification = json_object_get(parent,  r.getClassName().c_str());
                        json_object_set_new(JT_notification,   "requestId",             json_string(r.getUUID().c_str()));
                        json_object_set_new(JT_notification,   "device",             json_string(r.getDevice().c_str()));
                        json_object_set_new(JT_notification,   "tunerActivityStatus",             json_string(r.getTunerActivityStatus().c_str()));
                        json_object_set_new(JT_notification,   "lastActivityTimeStamp",             json_string(r.getLastActivityTimeStamp().c_str()));
                        json_object_set_new(JT_notification,   "lastActivityAction",             json_string(r.getLastActivityAction().c_str()));
                }

                json_dump_callback(parent, vector_dump_callback, &out, JSON_INDENT(4) | JSON_PRESERVE_ORDER);
                json_decref(parent);
        }
}

void JsonDecode(int handle, UpdateTunerActivityStatus & message)
{
        json_t * parent = (json_t *)handle;
        if (parent == 0) {
        }
        else
        //Decode parent
        {
                const char *key;
                json_t *value;
                json_object_foreach(parent, key, value){
                        //first key is "className", first value is json object.
                        Assert(json_object_size(parent) == 1);
                        json_t *JT_notification = value;
                        {
                                json_t *JT_requestId                    = json_object_get(JT_notification, "requestId");
                                json_t *JT_device             = json_object_get(JT_notification, "device");
                                json_t *JT_activity                               = json_object_get(JT_notification, "tunerActivityStatus");
                                json_t *JT_lastActivityTS     = json_object_get(JT_notification, "lastActivityTimeStamp");
                                json_t *JT_lastActivityAction                               = json_object_get(JT_notification, "lastActivityAction");


                                const char *requestId                   = json_string_value(JT_requestId);
                                const char *device                      = json_string_value(JT_device);
                                const char *activity                      = json_string_value(JT_activity);
                                const char *lastActivityTS                      = json_string_value(JT_lastActivityTS);
                                const char *lastActivityAction                      = json_string_value(JT_lastActivityAction);

                                Assert(requestId != 0);
                                Assert(device != 0);
                                Assert(activity != 0);
                                if(lastActivityTS == NULL ) lastActivityTS = "";
                                if(lastActivityAction == NULL ) lastActivityAction = "";

                                std::cout << "[DEC][" << UpdateTunerActivityStatus::klassName() << "]requestId = "              << requestId                << std::endl;
                                std::cout << "[DEC][" << UpdateTunerActivityStatus::klassName() << "]device = "   << device << std::endl;

                                std::cout << "[DEC][" << UpdateTunerActivityStatus::klassName() << "]tunerActivityStatus = "                        <<  activity                   << std::endl;
                                std::cout << "[DEC][" << UpdateTunerActivityStatus::klassName() << "]lastActivityTimeStamp = "                        <<  lastActivityTS             << std::endl;
                                std::cout << "[DEC][" << UpdateTunerActivityStatus::klassName() << "]lastActivityAction = "                        <<  lastActivityAction         << std::endl;

                                message = UpdateTunerActivityStatus(requestId,device,activity,lastActivityTS,lastActivityAction);
                                message.print();
                        }
                }
                json_decref(parent);
        }

}


/*
 * notifyTunerReservationRelease" =:
 * {
 *    "requestId"              : [String] requestId,
 *    "reservationToken"       : [String] reservationToken
 *    "reason"                 : [String] reason
 * }
 */
void JsonEncode(const NotifyTunerReservationRelease &r, std::vector<uint8_t> &out)
{
	json_t * parent = json_object();
	//Add parent
	if (parent == 0) {

	}
	else
	{//Add parent
		json_object_set_new(parent, r.getClassName().c_str(), json_object());

		{//Add child
			json_t *JT_notification = json_object_get(parent,  r.getClassName().c_str());
			json_object_set_new(JT_notification,   "requestId",  		json_string(r.getUUID().c_str()));
			json_object_set_new(JT_notification,   "reservationToken",  json_string(r.getReservationToken().c_str()));
			json_object_set_new(JT_notification,   "reason",   			json_string(r.getReason().c_str()));
		}

		json_dump_callback(parent, vector_dump_callback, &out, JSON_INDENT(4) | JSON_PRESERVE_ORDER);
		json_decref(parent);
	}
}

void JsonDecode(int handle, NotifyTunerReservationRelease & message)
{
	json_t * parent = (json_t *)handle;
	if (parent == 0) {
	}
	else
	//Decode parent
	{
		const char *key;
		json_t *value;
		json_object_foreach(parent, key, value){
			//first key is "className", first value is json object.
			Assert(json_object_size(parent) == 1);
			json_t *JT_notification = value;
			{
				json_t *JT_requestId 			= json_object_get(JT_notification, "requestId");
				json_t *JT_reservationToken		= json_object_get(JT_notification, "reservationToken");
				json_t *JT_reason 				= json_object_get(JT_notification, "reason");


				const char *requestId 			= json_string_value(JT_requestId);
				const char *reservationToken    = json_string_value(JT_reservationToken);
				const char *reason    			= json_string_value(JT_reason);

				Assert(requestId != 0);
				Assert(reservationToken != 0);
				Assert(reason != 0);

				std::cout << "[DEC][" << NotifyTunerReservationRelease::klassName() << "]requestId = "		    << requestId 		<< std::endl;
				std::cout << "[DEC][" << NotifyTunerReservationRelease::klassName() << "]reservationToken = "	<< reservationToken << std::endl;
				std::cout << "[DEC][" << NotifyTunerReservationRelease::klassName() << "]reason = "			   <<  reason 			<< std::endl;

				message = NotifyTunerReservationRelease(requestId, reservationToken, reason);
			}
		}
		json_decref(parent);
	}
}

/*
 * notifyTunerReservationUpdate" =:
 * {
 *    "requestId"              : [String] requestId,
 *    "tunerReservation"       : <TunerReservation>
 * }
 */
void JsonEncode(const NotifyTunerReservationUpdate &r, std::vector<uint8_t> &out)
{
	json_t * parent = json_object();
	//Add parent
	if (parent == 0) {

	}
	else
	{//Add parent
		json_object_set_new(parent, r.getClassName().c_str(), json_object());

		{//Add child
			json_t *JT_notification = json_object_get(parent,  r.getClassName().c_str());
			json_object_set_new(JT_notification,   "requestId",  json_string(r.getUUID().c_str()));
			json_object_set_new(JT_notification,   "tunerReservation",  json_object());

			//Encode grandchild
			{
				JsonEncode(r.getTunerReservation(), json_object_get(JT_notification, "tunerReservation"));
			}
		}

		json_dump_callback(parent, vector_dump_callback, &out, JSON_INDENT(4) | JSON_PRESERVE_ORDER);
		json_decref(parent);
	}
}

void JsonDecode(int handle, NotifyTunerReservationUpdate & message)
{
	json_t * parent = (json_t *)handle;
	if (parent == 0) {
	}
	else
	//Decode parent
	{
		const char *key;
		json_t *value;
		json_object_foreach(parent, key, value){
			//first key is "className", first value is json object.
			Assert(json_object_size(parent) == 1);
			json_t *JT_notification = value;
			{
				json_t *JT_requestId 			= json_object_get(JT_notification, "requestId");
				json_t *JT_tunerReservation		= json_object_get(JT_notification, "tunerReservation");

				const char *requestId 			= json_string_value(JT_requestId);
				Assert(requestId != 0);

				if (/***/JT_requestId) {
					std::cout << "[DEC][" << NotifyTunerReservationUpdate::klassName() << "]requestId = "		    << requestId 		<< std::endl;
				}

				if (/***/JT_tunerReservation){
					TunerReservation tunerReservation;
					JsonDecode(JT_tunerReservation, tunerReservation);
					message = NotifyTunerReservationUpdate(requestId, tunerReservation);
				}
			}
		}
		json_decref(parent);
	}
}

/*
 * notifyTunerReservationConflict" =:
 * {
 *    "requestId"            : [String] requestId,
 *    "tunerReservation"     : <TunerReservation>
 *    "conflicts"    	     : [<TunerReservation>]
 * }
 */
void JsonEncode(const NotifyTunerReservationConflicts &r, std::vector<uint8_t> &out)
{
	json_t * parent = json_object();
	//Add parent
	if (parent == 0) {

	}
	else
	{//Add parent
		json_object_set_new(parent, r.getClassName().c_str(), json_object());

		{//Add child
			json_t *JT_notification = json_object_get(parent,  r.getClassName().c_str());
			json_object_set_new(JT_notification,   "requestId",  		json_string(r.getUUID().c_str()));
			json_object_set_new(JT_notification,   "tunerReservation",  json_object());
			if(r.getConflicts().size())
			json_object_set_new(JT_notification,   "conflicts",   	    json_array());

			//Encode grandchild
			{
				JsonEncode(r.getTunerReservation(), json_object_get(JT_notification, "tunerReservation"));

				json_t *JT_conflicts = json_object_get(JT_notification, "conflicts");
				if (JT_conflicts)
				{ //
					typedef NotifyTunerReservationConflicts::ConflictCT ConflictsCT;

					const ConflictsCT & conflicts = r.getConflicts();
					ConflictsCT::const_iterator itc;
					for (itc = conflicts.begin(); itc != conflicts.end(); itc++) {
						json_t *JT_singleReservation = json_object();
						JsonEncode(*itc, JT_singleReservation);
						json_array_append_new(JT_conflicts, JT_singleReservation);
					}
				}
			}
		}

		json_dump_callback(parent, vector_dump_callback, &out, JSON_INDENT(4) | JSON_PRESERVE_ORDER);
		json_decref(parent);
	}
}

void JsonDecode(int handle, NotifyTunerReservationConflicts & message)
{
	json_t * parent = (json_t *)handle;
	if (parent == 0) {
	}
	else
	//Decode parent
	{
		const char *key;
		json_t *value;
		json_object_foreach(parent, key, value){
			//first key is "className", first value is json object.

			Assert(json_object_size(parent) == 1);
			json_t *JT_notification = value;
			{
				json_t *JT_requestId 			= json_object_get(JT_notification, "requestId");
				json_t *JT_tunerReservation		= json_object_get(JT_notification, "tunerReservation");
				json_t *JT_conflicts    		= json_object_get(JT_notification, "conflicts");

				const char *requestId 			= json_string_value(JT_requestId);

				if (/***/JT_requestId) {
					std::cout << "[DEC][NotifyTunerReservationConflicts] requestId = "		<< requestId	<< std::endl;
				}

				if (/***/JT_tunerReservation){
					TunerReservation tunerReservation;
					JsonDecode(JT_tunerReservation, tunerReservation);
					message = NotifyTunerReservationConflicts(requestId, tunerReservation);
				}

				if (/***/JT_conflicts) {
					//conflicts is a list of list.
					int numOfConflicts = json_array_size(JT_conflicts);

					for (int i = 0; i < numOfConflicts; i++) {
						json_t *JT_singleReservation = json_array_get(JT_conflicts, i);
						if (/***/JT_singleReservation) {
							TunerReservation tunerReservation;
							JsonDecode(JT_singleReservation, tunerReservation);
							message.addConflict(tunerReservation);
						}
					}
				}
			}
		}
		json_decref(parent);
	}
}

/*
 * NotifyTunerStatesUpdate" =:
 * {
 *    "requestId"            : string
 *    "detailedStates"       : <AllTunerDetailedStates>,
 * }
 */
void JsonEncode(const NotifyTunerStatesUpdate &r, std::vector<uint8_t> &out)
{
	json_t * parent = json_object();
	//Add parent
	if (parent == 0) {

	}
	else
	{//Add parent
		json_object_set_new(parent, r.getClassName().c_str(), json_object());

		{//Add child
			json_t *JT_notification = json_object_get(parent,  r.getClassName().c_str());
			json_object_set_new(JT_notification,   "requestId",  json_string(r.getUUID().c_str()));
			json_object_set_new(JT_notification,   "detailedStates",  json_object());
			//Encode grandchild
			{
				json_t *JT_detailedStates = json_object_get(JT_notification, "detailedStates");
				const std::map<std::string, DetailedTunerState>  & tunerDetailedStates = r.getTunerDetailedStates();
				std::map<std::string, DetailedTunerState>::const_iterator it;
				for (it = tunerDetailedStates.begin(); it != tunerDetailedStates.end(); it++) {

					json_object_set_new(JT_detailedStates, it->first.c_str(), json_object());
					{
						json_t *JT_detailedTunerState = json_object_get(JT_detailedStates, it->first.c_str());
						{
							JsonEncode(it->second, JT_detailedTunerState);
						}
					}
				}

			}
		}

		json_dump_callback(parent, vector_dump_callback, &out, JSON_INDENT(4) | JSON_PRESERVE_ORDER);
		json_decref(parent);
	}
}

void JsonDecode(int handle, NotifyTunerStatesUpdate & message)
{
	json_t * parent = (json_t *)handle;
	if (parent == 0) {
	}
	else
	//Decode parent
	{
		const char *key;
		json_t *value;
		json_object_foreach(parent, key, value){
			//first key is "className", first value is json object.
			Assert(json_object_size(parent) == 1);
			json_t *JT_notification = value;
			{
				json_t *JT_requestId 			= json_object_get(JT_notification, "requestId");
				if (/***/JT_requestId) {
					const char *requestId 			= json_string_value(JT_requestId);
					std::cout << "[DEC][NotifyTunerStatesUpdate] requestId = "		<< requestId	<< std::endl;
				}

				json_t *JT_detailedStates = json_object_get(JT_notification, "detailedStates");
				const char *tunerId;
				json_t *JT_detailedState;
				json_object_foreach(JT_detailedStates, tunerId, JT_detailedState){
					DetailedTunerState detailedState;
					JsonDecode(JT_detailedState, detailedState);
					message.addTunerDetailedState(tunerId, detailedState);
				}
			}
		}
		json_decref(parent);
	}
}

/*
 * notifyTunerPretune" =:
 * {
 *    "requestId"            : [String] requestId,
 *    "serviceLocator"       : [String] serviceLocator
 * }
 */
void JsonEncode(const NotifyTunerPretune &r, std::vector<uint8_t> &out)
{
	json_t * parent = json_object();
	//Add parent
	if (parent == 0) {

	}
	else
	{//Add parent
		json_object_set_new(parent, r.getClassName().c_str(), json_object());

		{//Add child
			json_t *JT_notification = json_object_get(parent,  r.getClassName().c_str());
			json_object_set_new(JT_notification,   "requestId",  		json_string(r.getUUID().c_str()));
			json_object_set_new(JT_notification,   "serviceLocator",  json_string(r.getServiceLocator().c_str()));
		}

		json_dump_callback(parent, vector_dump_callback, &out, JSON_INDENT(4) | JSON_PRESERVE_ORDER);
		json_decref(parent);
	}
}

void JsonDecode(int handle, NotifyTunerPretune & message)
{
	json_t * parent = (json_t *)handle;
	if (parent == 0) {
	}
	else
	//Decode parent
	{
		const char *key;
		json_t *value;
		json_object_foreach(parent, key, value){
			//first key is "className", first value is json object.
			Assert(json_object_size(parent) == 1);
			json_t *JT_notification = value;
			{
				json_t *JT_requestId 			= json_object_get(JT_notification, "requestId");
				json_t *JT_serviceLocator		= json_object_get(JT_notification, "serviceLocator");
				json_t *JT_reason 				= json_object_get(JT_notification, "reason");


				const char *requestId 			= json_string_value(JT_requestId);
				const char *serviceLocator    = json_string_value(JT_serviceLocator);

				Assert(requestId != 0);
				Assert(serviceLocator != 0);

				std::cout << "[DEC][" << NotifyTunerPretune::klassName() << "]requestId = "		    << requestId 		<< std::endl;
				std::cout << "[DEC][" << NotifyTunerPretune::klassName() << "]serviceLocator = "	<< serviceLocator << std::endl;

				message = NotifyTunerPretune(requestId, serviceLocator);
			}
		}
		json_decref(parent);
	}
}

void JsonEncode(const CancelLiveResponse &r, std::vector<uint8_t> &out)
{
	json_t * parent = json_object();
	//Add parent
	if (parent == 0) {

	}
	else
	{//Add parent
		json_object_set_new(parent, r.getClassName().c_str(), json_object());
		//Add child
		{
			json_t *JT_cancelLiveResponse = json_object_get(parent,  r.getClassName().c_str());
			JsonEncode(r, JT_cancelLiveResponse);
			json_object_set_new(JT_cancelLiveResponse, "cancelled",json_string(r.isCanceled() ? "true" : "false"));
		}

		json_dump_callback(parent, vector_dump_callback, &out, JSON_INDENT(4) | JSON_PRESERVE_ORDER);
		json_decref(parent);
	}

}

void JsonDecode(int handle, CancelLiveResponse & message)
{
	json_t * parent = (json_t *)handle;
	if (parent == 0) {
	}
	else
	//Decode parent
	{
		const char *key;
		json_t *value;
		json_object_foreach(parent, key, value){
			//first key is "className", first value is json object.
			Assert(json_object_size(parent) == 1);
			json_t *JT_cancelLiveResponse = value;
			{
				JsonDecode(JT_cancelLiveResponse, message);
				json_t *JT_canceled 			= json_object_get(JT_cancelLiveResponse, "cancelled");
				const char *canceled 			= JT_canceled ? json_string_value(JT_canceled) : "";
				bool isCanceled = (std::string(canceled).compare("true") == 0);
				message.setCanceled(isCanceled);
			}
		}
		json_decref(parent);
	}
}

/*
 * "cancelLive" =:
 * {
 *   "requestId"        : [String] requestId,
 *   "device"           : [String] device,
 *   "reservationToken" : <reservationToken>
 *   "serviceLocator"   : [String] serviceLocator
 * }
 */
void JsonEncode(const CancelLive &r, std::vector<uint8_t> &out)
{
	json_t * parent = json_object();

	//Add parent
	{
		json_object_set_new(parent, r.getClassName().c_str(), json_object());

		//Add child
		{
			json_t *JT_cancelLive = json_object_get(parent,  r.getClassName().c_str());
			JsonEncode(r, JT_cancelLive);
			json_object_set_new(JT_cancelLive, "serviceLocator", 		    json_string(r.getServiceLocator().c_str()));
		}
	}

	//@TODO: only enable JSON_PRESERVE_ORDER for debug builds.
	json_dump_callback(parent, vector_dump_callback, &out, JSON_INDENT(4) | JSON_PRESERVE_ORDER);
	json_decref(parent);
}

void JsonDecode(int handle, CancelLive & message)
{
	json_t * parent = (json_t *)handle;
	
	
	if (parent == 0) {
		std::cout << "[ERROR][JsonDecode : CancelLive] NULL Handle "<< std::endl;
	}
	else
	//Decode parent
	{
		const char *key;
		json_t *value;
		json_object_foreach(parent, key, value){
			Assert(json_object_size(parent) == 1);
			json_t *JT_cancelLive = value;
			{
				JsonDecode(JT_cancelLive, message);
				json_t *JT_serviceLocator 			= json_object_get(JT_cancelLive, "serviceLocator");

				const char *serviceLocator = json_string_value(JT_serviceLocator);
				std::cout << "[DEC][CanceLive] serviceLocator = "<< serviceLocator 					<< std::endl;
				message.setServiceLocator(serviceLocator);
			}
		}
		json_decref(parent);
	}
}

/*
 * {"result": "OK", "data": {"deviceId": "P1234567890", "authToken":"abCDefGhijK"}}
 */
int JsonDecode(const std::vector<uint8_t> &in, GenerateAuthTokenResponse &response)
{
	json_error_t error;

    if (in.size() == 0) {
        std::cout <<"[DEC][GenerateAuthTokenResponse]: Parsing Error, size 0" << std::endl;
        return 0;
    }

	json_t * parent = json_loadb((const char *)&in[0], in.size(), JSON_DISABLE_EOF_CHECK, &error);

	if (parent == 0) {
		json_dump_error("Load Error", error);
	}
	else
	{
		json_t *JT_result 				= json_object_get(parent, "result");
		json_t *JT_data 				= json_object_get(parent, "data");
		if (JT_result) {
			const char* result    			= json_string_value (JT_result);
		}

		if (JT_data) {
			json_t *JT_data_deviceId  = json_object_get(JT_data, "deviceId");
			json_t *JT_data_authToken = json_object_get(JT_data, "authToken");

			if (JT_data_deviceId) {
				response.deviceId = (json_string_value (JT_data_deviceId));
			}
			if (JT_data_authToken) {
				response.authToken = (json_string_value (JT_data_authToken));
			}
		}
	}

	json_decref(parent);
    return 0;
}

int JsonDecode(const std::vector<uint8_t> &in, GenerateAuthTokenResponseFromAuthService &response)
{
	json_error_t error;

    if (in.size() == 0) {
        std::cout <<"[DEC][GenerateAuthTokenResponseFromAuthService]: Parsing Error, size 0" << std::endl;
        return 0;
    }

	json_t * parent = json_loadb((const char *)&in[0], in.size(), JSON_DISABLE_EOF_CHECK, &error);

	if (parent == 0) {
		json_dump_error("Load Error", error);
	}
	else
	{
		json_t *JT_deviceId = json_object_get(parent, "deviceId");
		json_t *JT_partnerId = json_object_get(parent, "partnerId");


        if (JT_deviceId) {
            response.deviceId = (json_string_value (JT_deviceId));
        }
        if (JT_partnerId) {
            response.partnerId = (json_string_value (JT_partnerId));
        }
	}

	json_decref(parent);
    return 0;
}

std::string GetDeviceId()
{
    static std::string deviceId;
    //DELIA-39464 - Getting the device ID from /lib/rdk/getDeviceId.sh
    FILE *deviceIdScript = NULL;
    char scriptoutput[SCRIPT_OUTPUT_BUFFER_SIZE] = {0};
    if (!deviceId.empty()) return deviceId;
    deviceIdScript = popen(DEVICEID_SCRIPT_PATH, "r"); // pipe open the script
    if( (NULL != deviceIdScript) && deviceId.empty() ){ //Read the output from script
        if(fgets(scriptoutput,SCRIPT_OUTPUT_BUFFER_SIZE,deviceIdScript)!=NULL){
            std::string jsonResponse_(scriptoutput); //convert the output to sting object.
            std::vector<uint8_t> jsonResponse(jsonResponse_.begin(), jsonResponse_.end());
            GenerateAuthTokenResponseFromAuthService response;
            JsonDecode(jsonResponse, response); //parse the jason format string to object

            Log() << "AuthService::DeviceId = [" << response.deviceId << "]" << std::endl;
            deviceId = response.deviceId;
        }
    }
    if (deviceId.empty()) {
        static char generateTokenRequest[] = "GET /device/generateAuthToken HTTP/1.0\r\n\r\n";
        std::string jsonResponse_ = GetAuthToken(generateTokenRequest);
        std::vector<uint8_t> jsonResponse(jsonResponse_.begin(), jsonResponse_.end());
        GenerateAuthTokenResponse response;
        JsonDecode(jsonResponse, response);

        Log() << "WhiteBox::DeviceId = [" << response.deviceId << "]" << std::endl;
        deviceId = response.deviceId;
    }
    if(NULL != deviceIdScript)
        pclose(deviceIdScript);

    return deviceId;
}



/*
 * NotifyClientConnectionEvent =:
 * {
 *    "eventName"              : [String] Event Type,
 *    "clientIP"       		   : [String] Client IP Address
 *    "timeStamp"              : [int] Event Time Stamp
 * }
 */
 
void JsonEncode(const NotifyClientConnectionEvent &r, std::vector<uint8_t> &out)
{
	json_t * parent = json_object();
	//Add parent
	if (parent == 0) {

	}
	else
	{//Add parent
		json_object_set_new(parent, r.getClassName().c_str(), json_object());

		{//Add child
			json_t *JT_notification = json_object_get(parent,  r.getClassName().c_str());
			json_object_set_new(JT_notification,   "eventName",  		json_string(r.getEventName().c_str()));
			json_object_set_new(JT_notification,   "clientIP",  		json_string(r.getClientIP().c_str()));
			json_object_set_new(JT_notification,   "timeStamp",   		json_integer(r.getEventTimeStamp()));
		}

		json_dump_callback(parent, vector_dump_callback, &out, JSON_INDENT(4) | JSON_PRESERVE_ORDER);
		json_decref(parent);
	}
}

void JsonDecode(int handle, NotifyClientConnectionEvent & message)
{
	json_t * parent = (json_t *)handle;
	if (parent == 0) {
	}
	else
	//Decode parent
	{
		const char *key;
		json_t *value;
		json_object_foreach(parent, key, value){
			//first key is "className", first value is json object.
			Assert(json_object_size(parent) == 1);
			json_t *JT_notification = value;
			{

				json_t *JT_eventName 			= json_object_get(JT_notification, "eventName");
				json_t *JT_clientIP		        = json_object_get(JT_notification, "clientIP");
				json_t *JT_timeStamp 			= json_object_get(JT_notification, "timeStamp");


				const char *requestID 			= "";
				const char *eventName 			= json_string_value(JT_eventName);
				const char *clientIP    		= json_string_value(JT_clientIP);
				uint64_t timeStamp    		    = json_integer_value(JT_timeStamp);

				
				std::cout << "[DEC][" << NotifyClientConnectionEvent::klassName() << "]eventName = " << eventName 		<< std::endl;
				std::cout << "[DEC][" << NotifyClientConnectionEvent::klassName() << "]clientIP = "  << clientIP << std::endl;
				std::cout << "[DEC][" << NotifyClientConnectionEvent::klassName() << "]timeStamp = " <<  timeStamp 			<< std::endl;

				message = NotifyClientConnectionEvent(requestID,eventName,clientIP,timeStamp);
			}
		}
		json_decref(parent);
	}
}


static void JsonEncode(const NotifyClientConnectionEvent &r, json_t *parent)
{
	json_t *JT_notification = parent;
	json_object_set_new(JT_notification,   "eventName",json_string(r.getEventName().c_str()));
	json_object_set_new(JT_notification,   "clientIP",json_string(r.getClientIP().c_str()));
	json_object_set_new(JT_notification,   "timeStamp",json_integer(r.getEventTimeStamp()));
}

static void JsonDecode(json_t * parent, NotifyClientConnectionEvent & notification)
{
	json_t *JT_notification = parent;

	json_t *JT_eventName 			= json_object_get(JT_notification, "eventName");
	json_t *JT_clientIP		        = json_object_get(JT_notification, "clientIP");
	json_t *JT_timeStamp 			= json_object_get(JT_notification, "timeStamp");

	const char *requestID 			= "";
	const char *eventName 			= json_string_value(JT_eventName);
	const char *clientIP    		= json_string_value(JT_clientIP);
	uint64_t timeStamp    		    = json_integer_value(JT_timeStamp);

	std::cout << "[DEC][NotifyClientConnectionEvent] eventName = " << eventName << std::endl;
	std::cout << "[DEC][NotifyClientConnectionEvent] clientIP = "  << clientIP << std::endl;
	std::cout << "[DEC][NotifyClientConnectionEvent] timeStamp = " <<  timeStamp << std::endl;
}

/*
 * "GetTRMConnectionEvents" =:
 * {
 *   "requestId"         		 : [String] requestId,
 *    "status" 					 : [String] status,
 *    "conEventList"             ; [<NotifyClientConnectionEvent>.. ]
 * }
 */
void JsonEncode(const GetTRMConnectionEvents &r, std::vector<uint8_t> &out)
{
	json_t * parent = json_object();
	//Add parent
	if (parent == 0) {

	}
	else
	{//Add parent
		json_object_set_new(parent, r.getClassName().c_str(), json_object());

		{//Add child
			json_t *JT_response = json_object_get(parent,  r.getClassName().c_str());
			JsonEncode(r, JT_response);
			json_object_set_new(JT_response, "connectionEvents", json_array());

			{//Add Array Elements
				typedef GetTRMConnectionEvents::ConnectionEventList conEventLists;

				json_t *JT_eventlist = json_object_get(JT_response, "connectionEvents");
				if (JT_eventlist)
				{ //
					const conEventLists & eventList = r.getConEvents();
					conEventLists::const_iterator it;
					for (it = eventList.begin(); it != eventList.end(); it++) {
						json_t *JT_notification = json_object();
						JsonEncode(*it, JT_notification);
						json_array_append_new(JT_eventlist, JT_notification);
					}
				}
			}
		}
		json_dump_callback(parent, vector_dump_callback, &out, JSON_INDENT(4) | JSON_PRESERVE_ORDER);
		json_decref(parent);
	}
}

void JsonDecode(int handle, GetTRMConnectionEvents & message)
{
	json_t * parent = (json_t *)handle;
	if (parent == 0) {
	}
	else
	//Decode parent
	{
		const char *key;
		json_t *value;
		json_object_foreach(parent, key, value)	{
			//first key is "className", first value is json object.
			Assert(json_object_size(parent) == 1);
			json_t *JT_simpleResponse = value;
			{
				JsonDecode(JT_simpleResponse, message);
				json_t *JT_eventlist    		= json_object_get(JT_simpleResponse, "connectionEvents");
				if (JT_eventlist) {
					int numOfEvents = json_array_size(JT_eventlist);
					
					std::cout << "[DEC][GetTRMConnectionEvents] numOfEvents = "		<< numOfEvents	<< std::endl;

					for (int i = 0; i < numOfEvents; i++) {
						json_t *JT_singleNotifictaion = json_array_get(JT_eventlist, i);
						if (JT_singleNotifictaion) {
							NotifyClientConnectionEvent::ConnectionEvent event;
							JsonDecode(JT_singleNotifictaion,event);
							message.addConEvents(event);
						}
					}
				}

			}
		}
		json_decref(parent);
	}
}


/*
 * "GetAllConnectedDeviceIdsResponse" =:
 * {
 *   "requestId"         		 : [String] requestId,
 *    "status" 					 : [String] status,
 *    "DeviceIds"                 ; [<DeviceIds>, ... ]
 * }
 */
void JsonEncode(const GetAllConnectedDeviceIdsResponse &r, std::vector<uint8_t> &out)
{
	json_t * parent = json_object();
	//Add parent
	if (parent == 0) {

	}
	else
	{//Add parent
		json_object_set_new(parent, r.getClassName().c_str(), json_object());

		{//Add child
			json_t *JT_response = json_object_get(parent,  r.getClassName().c_str());
			JsonEncode(r, JT_response);
			json_object_set_new(JT_response, "DeviceIds", json_array());

			{//Add Array Elements
				json_t *JT_DeviceIds = json_object_get(JT_response,  "DeviceIds");
				const std::list<std::string> & DeviceIds = r.getDeviceIds();
				std::list<std::string>::const_iterator it;
				for (it = DeviceIds.begin(); it != DeviceIds.end(); it++) {
					json_array_append_new(JT_DeviceIds, json_string(it->c_str()));
				}
			}
		}

		json_dump_callback(parent, vector_dump_callback, &out, JSON_INDENT(4) | JSON_PRESERVE_ORDER);
		json_decref(parent);
	}

}

void JsonDecode(int handle, GetAllConnectedDeviceIdsResponse & message)
{
	json_t * parent = (json_t *)handle;
	if (parent == 0) {
	}
	else
	//Decode parent
	{
		const char *key;
		json_t *value;
		json_object_foreach(parent, key, value){
			//first key is "className", first value is json object.
			Assert(json_object_size(parent) == 1);
			json_t *JT_simpleResponse = value;
			{
				JsonDecode(JT_simpleResponse, message);
				json_t *JT_DeviceIds 			= json_object_get(JT_simpleResponse, "DeviceIds");
				int numOfDevices = json_array_size(JT_DeviceIds);
				for (int i = 0; i < numOfDevices; i++) {
					json_t *JT_id = json_array_get(JT_DeviceIds, i);
					message.addDeviceId(json_string_value(JT_id));
				}
			}
		}
		json_decref(parent);
	}
}

void delete_ReservationCustomAttributes(void *p)
{
    if (p != NULL) delete ((ReservationCustomAttributes *)p);
}


TRM_END_NAMESPACE


/** @} */
/** @} */
