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
* @defgroup qtapp
* @{
**/


#ifndef _TRM_MANAGER_H
#define _TRM_MANAGER_H

#include <limits>

#include <QObject>

#include "trm/TRM.h"
#include "trm/TunerReservation.h"
#include "trm/Timer.h"

#include "Tuner.h"
#include "ReservationTimerTask.h"
#include "Executors.h"

namespace TRM {

class ReservationAttributes
{
public:
	ReservationAttributes(uint32_t clientId) : clientId(clientId) {}
	uint32_t clientId;

	uint32_t getClientId(void) {
		return clientId;
	}
};
class PendingRequestProcessor
{
public:
    enum {
        PendingCancelLive,
        PendingReserveTunerConflicts,
        PendingCancelRecording,
    };

    PendingRequestProcessor(const std::string &uuid) : uuid(uuid) {};
	virtual bool timeout() = 0;
	virtual int getType() = 0;
	std::string getUUID() {
		return uuid;
	}
	virtual ~PendingRequestProcessor() {}

private:
	std::string uuid;
};

class PendingCancelLiveProcessor : public PendingRequestProcessor
{
public:
	PendingCancelLiveProcessor(uint32_t clientId, const CancelLive &cancelLiveRequest, const ReserveTuner &reserveRequest, const std::string &parentId) :
		PendingRequestProcessor(cancelLiveRequest.getUUID()), clientId(clientId), cancelLiveRequest(cancelLiveRequest), reserveRequest(reserveRequest), parentId(parentId) {}
	bool timeout();
	int getType() {
		return PendingCancelLive;
	}
	uint32_t clientId;
	CancelLive cancelLiveRequest;
	ReserveTuner reserveRequest;
	std::string parentId;
};

class PendingReserveTunerConflictProcessor: public PendingRequestProcessor
{
public:
	PendingReserveTunerConflictProcessor(const std::string &uuid, uint32_t clientId, const NotifyTunerReservationConflicts &notification, const ReserveTuner &request, const std::string &parentId)
	: PendingRequestProcessor(uuid), clientId(clientId), notification(notification),request(request), parentId(parentId) {}
	bool timeout();
	int getType() {
		return PendingReserveTunerConflicts;
	}
	uint32_t clientId;
	NotifyTunerReservationConflicts  notification;
	ReserveTuner request;
	std::string parentId;

};

class PendingCancelRecordingProcessor: public PendingRequestProcessor
{
public:
	PendingCancelRecordingProcessor(uint32_t clientId, const CancelRecording & request)
	:  PendingRequestProcessor(request.getUUID()), clientId(clientId) , request(request){}
	bool timeout();
	int getType() {
		return PendingCancelRecording;
	}

	uint32_t clientId;
	CancelRecording  request;
};


class Manager : public QObject
{
	Q_OBJECT
public:
	friend class ReservationExpirationTimerTask;
	friend class PendingCancelLiveProcessor;
	friend void Execute(Executor<CancelLiveResponse> &exec);
;

	typedef std::map<std::string, Tuner > TunerCT;
	typedef std::map<std::string, ReservationExpirationTimerTask > ExpireTimerCT;
	typedef std::map<std::string, ReservationStartTimerTask > StartTimerCT;
	typedef std::map<std::string, ReservationPreStartTimerTask > PreStartTimerCT;
	typedef std::map<std::string, PendingRequestTimeoutTimerTask > PedingRequestTimerCT;

	typedef std::map<std::string, ReservationAttributes > ReservationAttributesCT;

	static Manager &getInstance();
	static const int kNumOfTuners = NUM_OF_TUNERS;

	Tuner::IdList & getTunerIds(Tuner::IdList & tunerIds);
	Tuner & getTuner(const std::string &tunerId);
	const std::string & getParent(const std::string &reservationToken);
	void prepareRecordHandover(const std::string &tunerId, const std::string &reservationToken);
	void syncHybrid(const std::string &tunerId, const std::string &reservationToken);

	void addReservation(const TunerReservation & reservation, const std::string &tunerId);
	void setReservationAttributes(const std::string &token, const ReservationAttributes &attr);
	ReservationAttributes & getReservationAttributes(const std::string &token);
	void releaseReservation(const std::string &reservationToken, bool notifyClient = true);
	TunerReservation::TokenList & getReservationTokens(TunerReservation::TokenList &tokens, uint32_t clientId);
	TunerReservation::TokenList & getReservationTokens(TunerReservation::TokenList &tokens, const std::string tunerId);
	TunerReservation::TokenList & getReservationTokens(TunerReservation::TokenList &tokens, const Tuner::IdList & tunerIds);
	TunerReservation & getReservation(const std::string &reservationToken);

	void addPendingRequest(PendingRequestProcessor *, int timeout);
	bool isPendingConflict(const std::string &reservationToken);
	bool isPendingRequest (const std::string &reservationToken);
	PendingRequestProcessor & getPendingRequest (const std::string &reservationToken);
	void removePendingRequest(const std::string &reservationToken);
	NotifyTunerStatesUpdate getTunerStatesUpdate(void);
	void startReservation(const std::string &reservationToken);

	std::string getLocalTuner();
	void setLocalTuner(const std::string &tunerId);
	void adjustExpireTimer(const std::string &reservationToken);

private:
	Manager(void);
	void setTimer(const TunerReservation & reservation);
	void cancelPreStartTimer(const std::string &reservationToken);
	void cancelStartTimer(const std::string &reservationToken);
	void cancelExpireTimer(const std::string &reservationToken);
	void cancelPendingTimer(const std::string &reservationToken);

	TunerCT tuners;
	ExpireTimerCT expireTimers;
	StartTimerCT startTimers;
	PreStartTimerCT preStartTimers;
	PedingRequestTimerCT pendingRequestTimers;

	ReservationAttributesCT attrs;
	//Single listener is enough for current usage.
	std::list<PendingRequestProcessor *>  pendingRequests;
	std::string localTuner;

signals:
    void reservationStarted(uint32_t);
	void reservationReleased(uint32_t, Activity, std::string, std::string);
	void reservationUpdated(void);
	void notifyTunerStatesUpdate(NotifyTunerStatesUpdate);
	void cancelLiveResponse(CancelLiveResponse, ReserveTuner, const std::string);
	void timerDeleted(void *);

public slots:
	void onReservationReleased(uint32_t, Activity, std::string, std::string);
	void onCancelLiveResponse(CancelLiveResponse, ReserveTuner, const std::string);
	void onNotifyTunerStatesUpdate(NotifyTunerStatesUpdate);
	void onReservationUpdated(void);
	void onTimerDeleted(void *);

};

TRM_END_NAMESPACE

#endif


/** @} */
/** @} */
