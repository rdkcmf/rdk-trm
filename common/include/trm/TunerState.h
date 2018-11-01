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
 * @defgroup TRM_STATES Tuner States
 * @ingroup TRM_MAIN
 *
 * @par Tuner States
 * State       | Meaning
 * ------------| -----------
 * Free        | The Tuner is available for allocation.
 * Live        | Currently tuned to a service and in use for viewing the live TV channel.
 * Record      | Currently tuned to a service and recording a program.
 * Hybrid      | Live view of recording in progress.
 * EAS         | Emergency Alert Service.
 *
 * @n The Client device can take one tuner in case there is a Free tuner, or in case the client device
 * requests a tuner for channel that currently is recording by some of the tuners
 * (in this case this tuner becomes to Hybrid mode).
 *
 * @par Tuner State Diagram
 * @image html trm_state1.png
 * - User makes a request from the tuner in a @b Free state which is not allocated.
 * - If the user wants to view the live video, the tuner @b Live state is accessed.
 * - If the user wants to record a program, the tuner @b Recording state is accessed.
 * - If the user wants to record a program as well as view the live, the @b Hybrid state is accessed.
 * - If the user wishes to view a recorded program, the @b Hybrid state is allowed to this as well.
 * - If there is any alert, the tuner will use the Emergency Alert State.
 *
 * @par Tuner Conflict States
 * - <b> Live playback in conflict </b>
 * @n When a client device currently uses Hybrid tuner and requests a channel that is not recording presently
 * and there is Free tuner at that moment. Let's say tuner states are
 * @n Live(Xi3) - Hybrid(recording and Xi3) - Record - Record
 * @n In this case user gets an overlay posting to tune to requested channel and cancel one of the recordings.
 * Or to leave current channel and keep the recording.
 * @n
 * - <b> Hot recording conflict </b>
 * @n When there is no @b Free tuners and user schedules a hot recording, lets say tuner states are
 * @n Live(Xi3) - Live (Xi3) - Live(Xi3) - Record - Record
 * @user tries to schedule a recording on a channel that is not currently streamed. So since there is no
 * @b Free tuner user will get an overlay proposing to switch the channel and record the program. Or to
 * cancel the Hot recording and stay watching current channel.
 * @n
 * - <b> Future recording conflict </b>
 * @n It is almost same as Hot recording conflict, the difference is that the recording is a future recording and is
 * about to start STB sends a conflict in a minute before the recording starts.
 *
 * @par Tuner State Messages format
 * Tuner State object represents state of the tuner.
 * @n
 * @code
 * State :=
 * {
 *      "state" : [String] state,
 * }
 * @endcode
 * @n
 * The state field indicates the activity state of a tuner:
 * - @b Live: the tuner is reserved for Live activity (Streaming or Playback).
 * - @b Record: the tuner is reserved for Record activity.
 * - @b Hybrid: the tuner is reserved for Live and Record activity.
 * - @b EAS: the tuner is reserved for EAS.
 * - @b Free: the Tuner is not reserved.
 *
 * @par Message format for DetailedTunerState
 * @n
 * @code
 * DetailedTunerState :=
 * {
 *      "state" : <State>
 *      "serviceLocator" : [String] sourceLocator
 *      "owners" :
 *      {
 *           <Activity> : {
 *               "device" : [String] device
 *           }
 *           ...
 *      }
 *      "reservedDeviceId" : [String]
 * }
 * @endcode
 * @n
 * - @b reservedDeviceId: If this value is present in the detailedStates,
 * the corresponding tuner can only be used for the following usages.
 * - @b LIVE streaming for the device represented by the reservedDeviceId.
 * - @b RECORD for any recording request, regardless of the recording.s originating device.
 *
 * @par Message format for AllTunerState
 * @n
 * @code
 * AllTunerStates :=
 * {
 *       <tunerId> : <State>
 *       ...
 * }
 * @endcode
 *
 * @defgroup TRM_STATES_IFACE Tuner State Classes
 * @ingroup TRM_STATES
 */

/**
* @defgroup trm
* @{
* @defgroup common
* @{
**/


#ifndef TRM_TUNER_STATE_H_
#define TRM_TUNER_STATE_H_

#include "Enum.h"
#include "trm/Activity.h"
#include "Klass.h"

TRM_BEGIN_NAMESPACE


/**
 * @brief The TunerState class represents state of the tuner.
 * The state field in the class indicates the activity state of a tuner:
 * - Live : the tuner is reserved for Live activity (Streaming or Playback)
 * - Record : the tuner is reserved for Record activity.
 * - Hybrid : the tuner is reserved for Live and Record activity.
 * - EAS : the tuner is reserved for EAS.
 * - Free : the Tuner is not reserved.
 * @ingroup TRM_STATES_IFACE
 */
class TunerState
{
public:
	static const char *klassName(void) { return Klass::kTunerState; }

	typedef  int   EnumType;

	static const Enum<TunerState> kFree;
	static const Enum<TunerState> kLive;
	static const Enum<TunerState> kRecord;
	static const Enum<TunerState> kHybrid;
	static const Enum<TunerState> kEAS;

	static const std::vector<const Enum<TunerState> * > & getEnums(void);

	TunerState(const Enum<TunerState> &state);
	TunerState(const char *);
	virtual ~TunerState(void);

	const Enum<TunerState> & getState(void) const;

	virtual TunerState operator +  (const Enum<Activity> &activity);
//	virtual TunerState operator -  (const Enum<Activity> &activity);
	bool               operator == (const TunerState &that) const;
	bool               operator != (const TunerState &that) const;

	void print(void) const;

protected:
	Enum<TunerState> state;
};


/**
 * @brief This class is responsible for managing the Free state of a tuner.
 * @ingroup TRM_STATES_IFACE
 */
class FreeState : public Enum<TunerState>
{
public:
	FreeState(void);
	~FreeState(void);
	Enum<TunerState> operator + (const Enum<Activity> &activity);

};

/**
 * @brief This class is responsible for handling the Live activity (Streaming or Playback) of a tuner.
 * @ingroup TRM_STATES_IFACE
 */
class LiveState : public Enum<TunerState>
{
public:
	LiveState(void);
	~LiveState(void);
	Enum<TunerState> operator + (const Enum<Activity> &activity);
	Enum<TunerState> operator - (const Enum<Activity> &activity);

};

/**
 * @brief This class is responsible for handling the Record activity of a tuner.
 * @ingroup TRM_STATES_IFACE
 */
class RecordState : public Enum<TunerState>
{
public:
	RecordState(void);
	~RecordState(void);
	Enum<TunerState> operator + (const Enum<Activity> &activity);
	Enum<TunerState> operator - (const Enum<Activity> &activity);
};

/**
 * @brief The tuner is reserved for Live or Record activity.
 * @brief This class is responsible for handling the Hybrid activity (Playback and Record simultaneously) of a tuner.
 * @ingroup TRM_STATES_IFACE
 */
class HybridState : public Enum<TunerState>
{
public:
	HybridState(void);
	~HybridState(void);
	Enum<TunerState> operator - (const Enum<Activity> &activity);
};

TRM_END_NAMESPACE
#endif



/** @} */
/** @} */
