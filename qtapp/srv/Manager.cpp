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


#include "trm/TRM.h"
#include "Manager.h"
#include "Util.h"
#include "Server.h"
#include "Executors.h"
#include "Connection.h"

extern TRM::Server *serverInstance;

TRM_BEGIN_NAMESPACE
extern std::string GetDeviceId();

Manager &Manager::getInstance()
{
	static Manager managerInstance_;
	return managerInstance_;
}

Tuner::IdList &  Manager::getTunerIds(Tuner::IdList & Ids)
{
	Ids.clear();
	TunerCT::const_iterator it;
	for (it = tuners.begin(); it != tuners.end(); it++) {
		Ids.push_back(it->first);
	}
	Assert(tuners.size() == Ids.size());
	return Ids;
}


TunerReservation::TokenList & Manager::getReservationTokens(TunerReservation::TokenList &tokens, uint32_t clientId)
{
	ReservationAttributesCT::iterator it = attrs.begin();
	for (it = attrs.begin(); it != attrs.end(); it++) {
		if ((it->second).clientId == clientId) {
			tokens.push_back(it->first);
		}
	}

	return tokens;
}

TunerReservation::TokenList & Manager::getReservationTokens(TunerReservation::TokenList &tokens, const std::string tunerId)
{

	if(!tunerId.empty()) {
		getTuner(tunerId).getReservationTokens(tokens);
	}
	Log() << "getReservationTokens from tuner " << tunerId << " return " << tokens.size() << std::endl;
	return tokens;
}

TunerReservation::TokenList & Manager::getReservationTokens(TunerReservation::TokenList &tokens, const Tuner::IdList & tunerIds)
{
	Tuner::IdList::const_iterator it;
	for (it = tunerIds.begin(); it != tunerIds.end(); it++) {
		getTuner(*it).getReservationTokens(tokens);
	}

	//Log() << "getReservationTokens return " << tokens.size() << std::endl;
	return tokens;
}

void Manager::addReservation(const TunerReservation & reservation, const std::string &tunerId)
{
	Log() << "vManager Adding Reservation " << reservation.getReservationToken() << " To tuner " << tunerId << std::endl;

	getTuner(tunerId).addReservation((reservation));
	/* Add start timer and expiration timer*/
	{
		PreStartTimerCT::iterator it = preStartTimers.find(reservation.getReservationToken());
		Assert(it == preStartTimers.end());
	}

	{
		StartTimerCT::iterator it = startTimers.find(reservation.getReservationToken());
		Assert(it == startTimers.end());
	}

	{
		ExpireTimerCT::iterator it = expireTimers.find(reservation.getReservationToken());
		Assert(it == expireTimers.end());
	}

	preStartTimers.insert (std::pair<std::string, ReservationPreStartTimerTask>(
						reservation.getReservationToken(),
						ReservationPreStartTimerTask(*(new Timer(reservation.getReservationToken())), reservation.getReservationToken())) );

	startTimers.insert (std::pair<std::string, ReservationStartTimerTask>(
						reservation.getReservationToken(),
						ReservationStartTimerTask(*(new Timer(reservation.getReservationToken())), reservation.getReservationToken())) );


	expireTimers.insert (std::pair<std::string, ReservationExpirationTimerTask>(
						reservation.getReservationToken(),
						ReservationExpirationTimerTask(*(new Timer(reservation.getReservationToken())), reservation.getReservationToken())) );
	setTimer(reservation);

	//emit reservationUpdated();
}

void Manager::setReservationAttributes(const std::string &token, const ReservationAttributes &attr)
{
	Log() << "setReservationAttributes for " << token;
	ReservationAttributesCT::iterator it = attrs.find(token);
	if (it == attrs.end()) {
		attrs.insert (std::pair<std::string, ReservationAttributes>(token, attr));
	}
	else {
		it->second = attr;
	}
}

ReservationAttributes & Manager::getReservationAttributes(const std::string &token)
{
	ReservationAttributesCT::iterator it = attrs.find(token);

	if (it == attrs.end()) {
		//@ItemNotFound Exception
		throw (0);
	}

	return it->second;
}

void Manager::releaseReservation(const std::string &reservationToken, bool notifyClient)
{
	Log() << "vManager Releasing Reservation " << reservationToken << std::endl;

	TunerReservation &reservation = getReservation(reservationToken);
	std::string serviceLocator = reservation.getServiceLocator();
	Activity activity = reservation.getActivity();

    cancelPreStartTimer(reservationToken);
    cancelStartTimer(reservationToken);
    cancelExpireTimer(reservationToken);

	preStartTimers.erase(reservationToken);
	startTimers.erase(reservationToken);
	expireTimers.erase(reservationToken);


	ReservationAttributesCT::iterator itt = attrs.find(reservationToken);
	uint32_t clientId = 0;
	if(itt != attrs.end()) {
		clientId = (*itt).second.clientId;
		attrs.erase(reservationToken);
	}
	else {
		Log() << "cannot find ReservationAttributes for " << reservationToken;

		throw ItemNotFoundException();
	}

	getTuner(getParent(reservationToken)).releaseReservation(reservationToken);

	/* Emit Notification so the pending requests can be retried */
	emit reservationReleased(clientId, activity, reservationToken, serviceLocator);
	if (notifyClient) {
	    emit reservationUpdated();
	}

}

TunerReservation & Manager::getReservation(const std::string &reservationToken)
{
	//Log() << "Request Reservation from token " << reservationToken << std::endl;
	TunerCT::iterator itt;
	for (itt = tuners.begin(); itt != tuners.end(); itt++) {
		TunerReservation::TokenList tokens;
		//Log() << "GetReservation of Tuner " << itt->second.getId() << std::endl;
		itt->second.getReservationTokens(tokens);
		TunerReservation::TokenList::iterator itr;
		for (itr = tokens.begin(); itr != tokens.end(); itr++) {
			//Log() << "GetReservation " << *itr << " of Tuner " << itt->second.getId() << std::endl;
			if (itr->compare(reservationToken) == 0) {
				return itt->second.getReservation(reservationToken);
			}
		}
	}
	Log() << "Throw ItemNotFoundException from Manager::getReservation()" << std::endl;
	throw ItemNotFoundException();
}

const std::string & Manager::getParent(const std::string &reservationToken)
{
	TunerCT::iterator itt;
	for (itt = tuners.begin(); itt != tuners.end(); itt++) {
		TunerReservation::TokenList tokens;
		itt->second.getReservationTokens(tokens);
		TunerReservation::TokenList::iterator itr;
		for (itr = tokens.begin(); itr != tokens.end(); itr++) {
			if (itr->compare(reservationToken) == 0) {
				Log() << "Parent is " << itt->second.getId() << std::endl;
				return itt->second.getId();
			}
		}
	}
	throw ItemNotFoundException();
}

Tuner & Manager::getTuner(const std::string &tunerId)
{
	TunerCT::iterator it = tuners.find(tunerId);
	if (it == tuners.end()) {
		throw ItemNotFoundException();
		//@TODO Throw Exception;
	}

	return it->second;

}

void Manager::addPendingRequest(PendingRequestProcessor *request_, int timeout)
{
	PendingRequestProcessor &request = *request_;
	Log() << "Manager::addPendingRequest "  << request.getUUID() << " " << (void *)&request << std::endl;

	pendingRequests.push_back(&request);

	/* Add wait timer */
	{
		PedingRequestTimerCT::iterator it = pendingRequestTimers.find(request.getUUID());
		if (it == pendingRequestTimers.end()) {

			pendingRequestTimers.insert (std::pair<std::string, PendingRequestTimeoutTimerTask>(
								request.getUUID(),
								PendingRequestTimeoutTimerTask(*(new Timer(request.getUUID())), request.getUUID())) );

			{
				PedingRequestTimerCT::iterator it = pendingRequestTimers.find(request.getUUID());
				Assert(it != pendingRequestTimers.end());
				PendingRequestTimeoutTimerTask &task = it->second;
				task.getTimer().schedule(task, timeout, false);
				Log() << "Pending Request " << request.getUUID() << " Add Timer " << task.getTimer().getToken() << std::endl;
			}
		}

		//emit reservationUpdated();
	}

}

bool Manager::isPendingConflict(const std::string &reservationToken)
{
	bool found = false;
	std::list<PendingRequestProcessor *>::iterator it = pendingRequests.begin();
	while(it != pendingRequests.end() && !found) {
		if ((*it)->getType() == PendingRequestProcessor::PendingReserveTunerConflicts)
		{
			PendingReserveTunerConflictProcessor &pendingRequest = *(static_cast<PendingReserveTunerConflictProcessor *>(*it));
			std::list<TunerReservation>::iterator itt = (pendingRequest).notification.getConflicts().begin();
			while(itt != (pendingRequest).notification.getConflicts().end() && !found) {
				if (reservationToken.compare((*itt).getReservationToken()) == 0) {
					found = true;
				}
				itt++;
			}
		}
		it++;
	}

	return found;
}

bool Manager::isPendingRequest(const std::string &uuid)
{
	std::list<PendingRequestProcessor *>::iterator it = pendingRequests.begin();
	while(it != pendingRequests.end()) {
		if ((**it).getUUID().compare(uuid) == 0) {
			break;
		}
		it++;
	}

	return (it != pendingRequests.end());
}

PendingRequestProcessor & Manager::getPendingRequest(const std::string &uuid)
{
	std::list<PendingRequestProcessor *>::iterator it = pendingRequests.begin();
	while(it != pendingRequests.end()) {
		if ((**it).getUUID().compare(uuid) == 0) {
			break;
		}
		it++;
	}

	if (it == pendingRequests.end()) {
		Log() << "Throw ItemNotFoundException from Manager::getPendingRequest() for " << uuid << std::endl;
		throw ItemNotFoundException();
	}

	return **it;
}

void Manager::removePendingRequest(const std::string &uuid)
{
	Log() << "Manager::removePendingRequest  " << uuid << std::endl;

	std::list<PendingRequestProcessor *>::iterator it = pendingRequests.begin();
	while(it != pendingRequests.end()) {
		if ((**it).getUUID().compare(uuid) == 0) {
			cancelPendingTimer(uuid);
			pendingRequestTimers.erase(uuid);
			it = pendingRequests.erase(it);
			break;
		}
		it++;
	}

	if(it == pendingRequests.end()) {
	}

}

void Manager::onReservationReleased(uint32_t clientId, Activity activity, std::string reservationToken, std::string serviceLocator)
{
	Log() << "Manager::onReservationReleased from client  " << std::hex << clientId <<
			  " for activity " << (const char *)activity.getActivity() << " for serviceLocator " << serviceLocator <<  std::endl;
}

void Manager::onReservationUpdated(void)
{
	Log() << "Manager::onReservationUpdated has pending:" <<pendingRequests.size() <<std::endl;
    std::list<std::string> timedoutPending;
    if (pendingRequests.size()) {
        /* if in the reservationUpdate, there is pendingReserveTunerConflicts 
         * tune to the same channel as the pending request, then re-evaluate pending request
         */
        std::list<PendingRequestProcessor *>::iterator it = pendingRequests.begin();
        while(it != pendingRequests.end()) {
            if ((*it)->getType() == PendingRequestProcessor::PendingReserveTunerConflicts) {
                bool grantConflict = false;

                PendingReserveTunerConflictProcessor &pendingRequest = *(static_cast<PendingReserveTunerConflictProcessor *>(*it));
                const TunerReservation &liveReservationAtConflict =  pendingRequest.notification.getTunerReservation();
                try {
                    const TunerReservation &liveReservationNow = Manager::getReservation(liveReservationAtConflict.getReservationToken());
                    if (pendingRequest.notification.getConflicts().size() == 1) {
                        if (liveReservationNow.getServiceLocator().compare(pendingRequest.notification.getConflicts().begin()->getServiceLocator()) == 0) {
                            Log() << "Manager::onReservationUpdated conflicting live token " 
                                  << liveReservationAtConflict.getReservationToken() 
                                  << " has updated from " 
                                  << liveReservationAtConflict.getServiceLocator()
                                  << " to "
                                  << liveReservationNow.getServiceLocator()
                                  << std::endl;
                            grantConflict = true;
                        }
                    }
                    else {
                        Log() << "Manager::onReservationUpdated detected conflicts mismatch " << pendingRequest.notification.getConflicts().size() << std::endl;
                        SafeAssert(0);
                    }
                }
                catch(...) {
                    Log() << "Manager::onReservationUpdated conflicting live token has expired " << liveReservationAtConflict.getReservationToken() << std::endl;
                    grantConflict = true;
                }

                if (grantConflict) {
                    try {
                        Log() << "Manager::onReservationUpdated pending conflict is effectively timedout :" << liveReservationAtConflict.getReservationToken() << std::endl;
                        pendingRequest.timeout();
                        timedoutPending.push_back(pendingRequest.getUUID());
                    }
                    catch(...) {
                        Log() << "Manager::onReservationUpdated pending conflict is already removed" << std::endl;
                    }
                }
            }
            it++;
        }
    }
    
    if (timedoutPending.size()) {
        std::list<std::string>::iterator it = timedoutPending.begin();
        while(it != timedoutPending.end()) {
            PendingRequestProcessor &pending = Manager::getInstance().getPendingRequest(*it);
            removePendingRequest(*it);
            delete &pending;
            it++;
        }
    }
}

void Manager::onNotifyTunerStatesUpdate(NotifyTunerStatesUpdate update)
{
	Log() << "Manager::onNotifyTunerStatesUpdate" <<  std::endl;
}

Manager::Manager(void)
{
	GetDeviceId();
	tuners.clear();
	for (int i = 0; i < kNumOfTuners; i++) {
		std::string id = "TunerId-" + IntToString(i) + "";
        Log() << Timestamp << "====================Addings TunerId " << id << std::endl;
		tuners.insert ( std::pair<std::string,Tuner>(id, Tuner(id)) );
	}

	setLocalTuner("TunerId-0");
	QObject::connect(this, SIGNAL(reservationReleased(uint32_t, Activity, std::string, std::string)), this, SLOT(onReservationReleased(uint32_t, Activity, std::string, std::string)));
	QObject::connect(this, SIGNAL(cancelLiveResponse(CancelLiveResponse, ReserveTuner, const std::string)), this, SLOT(onCancelLiveResponse(CancelLiveResponse, ReserveTuner, const std::string)));
	QObject::connect(this, SIGNAL(notifyTunerStatesUpdate(NotifyTunerStatesUpdate)), this, SLOT(onNotifyTunerStatesUpdate(NotifyTunerStatesUpdate)));
    QObject::connect(this, SIGNAL(reservationUpdated(void)), this, SLOT(onReservationUpdated(void)), Qt::QueuedConnection);
    QObject::connect(this, SIGNAL(timerDeleted(void*)), this, SLOT(onTimerDeleted(void*)), Qt::QueuedConnection);

}

void Manager::setTimer(const TunerReservation &reservation)
{
	{
		PreStartTimerCT::iterator it = preStartTimers.find(reservation.getReservationToken());
		Assert(it != preStartTimers.end());
		ReservationPreStartTimerTask &task = it->second;
		task.getTimer().schedule(task, reservation.getStartTime() - 250, true);

		Log() << "Reservation " << reservation.getReservationToken() << " Add Pre-Start Timer " << task.getTimer().getToken() << std::endl;
	}

	{
		StartTimerCT::iterator it = startTimers.find(reservation.getReservationToken());
		Assert(it != startTimers.end());
		ReservationStartTimerTask &task = it->second;
		task.getTimer().schedule(task, reservation.getStartTime(), true);

		Log() << "Reservation " << reservation.getReservationToken() << " Add Start Timer " << task.getTimer().getToken() << std::endl;
	}

	{
		ExpireTimerCT::iterator it = expireTimers.find(reservation.getReservationToken());
		Assert(it != expireTimers.end());
		ReservationExpirationTimerTask &task = it->second;
		task.getTimer().schedule(task, reservation.getExpirationTime());

		Log() << "Reservation " << reservation.getReservationToken() << " Add Expire Timer " << task.getTimer().getToken() << std::endl;
	}
}

void Manager::cancelStartTimer(const std::string &reservationToken)
{
	StartTimerCT::iterator it = startTimers.find(reservationToken);
	if(it != startTimers.end()) {
		ReservationStartTimerTask &task = it->second;
		task.getTimer().cancel();
        emit timerDeleted((void *)(&task.getTimer()));
	}
	else {
		//It is ok to miss start timer, so don't throw exception.
	}
}

void Manager::cancelPreStartTimer(const std::string &reservationToken)
{
	PreStartTimerCT::iterator it = preStartTimers.find(reservationToken);
	if(it != preStartTimers.end()) {
		ReservationPreStartTimerTask &task = it->second;
		task.getTimer().cancel();
        emit timerDeleted((void *)(&task.getTimer()));
	}
	else {
		//It is ok to miss start timer, so don't throw exception.
	}
}

void Manager::cancelExpireTimer(const std::string &reservationToken)
{
	ExpireTimerCT::iterator it = expireTimers.find(reservationToken);
	if(it != expireTimers.end()) {
		ReservationExpirationTimerTask &task = it->second;
		task.getTimer().cancel();
        emit timerDeleted((void *)(&task.getTimer()));
	}
	else {
		Log() << "cancelExpireTimer cannot find timer for token " << reservationToken << std::endl;
		throw ItemNotFoundException();
	}
}

void Manager::adjustExpireTimer(const std::string &reservationToken)
{
    try {
        TunerReservation &reservation = Manager::getInstance().getReservation(reservationToken);
        ExpireTimerCT::iterator it = expireTimers.find(reservationToken);
        if(it != expireTimers.end()) {
            Log() << "adjustExpireTimer adjusting expire timer " << reservationToken << std::endl;
            ReservationExpirationTimerTask &task = it->second;
            task.getTimer().cancel();
            task.getTimer().schedule(task, reservation.getExpirationTime());
        }
        else {
        }
    }
    catch(...) {
		Log() << "adjustExpireTimer cannot find timer or token, may expried already " << reservationToken << std::endl;
    }
}

void Manager::cancelPendingTimer(const std::string &reservationToken)
{
	PedingRequestTimerCT::iterator it = pendingRequestTimers.find(reservationToken);
	if(it != pendingRequestTimers.end()) {
		PendingRequestTimeoutTimerTask &task = it->second;
		task.getTimer().cancel();
		//@TODO Timer may already have started.
		delete (&task.getTimer());
	}
	else {
		//It is ok to miss start timer, so don't throw exception.
	}
}

void Manager::prepareRecordHandover(const std::string &tunerId, const std::string &reservationToken)
{
    /* If recording is about to start, and tuner is hybrid on a different
     * channel, pre-sync need to cancelLive to yield tuner to 
     * recording. Upon syncHybrid, guide will be notified and tune LIVE
     * to the recording channel.
     */
	if (getTuner(tunerId).getState().getState() == TunerState::kHybrid) {
		/* First get the recording token */
        std::string liveToken= "";
        std::string recordToken= "";
		std::string recordLocator="";
		std::string liveLocator="";
		std::string nextRecordLocator="";

		TunerReservation::TokenList tokens;
		Manager::getInstance().getReservationTokens(tokens, tunerId);

		TunerReservation::TokenList::const_iterator it = tokens.begin();
		while (it != tokens.end()) {
			if (getReservation(*it).state == TunerReservation::ACTIVE) {
				if (getReservation(*it).getActivity().getActivity() == Activity::kRecord) {
					recordLocator = getReservation(*it).getServiceLocator();
                    recordToken = *it;
				}
				if (getReservation(*it).getActivity().getActivity() == Activity::kLive) {
					liveLocator = getReservation(*it).getServiceLocator();
                    liveToken = *it;
                }
			}
			it++;
		}

		/* Since we are in hybrid mode, there must be 1 Record locator, and 1 live locator */
		Assert(!recordLocator.empty());
		Assert(!liveLocator.empty());
		Assert(liveLocator.compare(recordLocator) == 0);

        nextRecordLocator = Manager::getInstance().getReservation(reservationToken).getServiceLocator();
        Assert(Manager::getInstance().getReservation(reservationToken).getActivity() == Activity::kRecord);
		Assert(!nextRecordLocator.empty());

        if (liveLocator.compare(nextRecordLocator) != 0) {
            Log() << "Hybrid tuner is on : LIVE + RECORD [ " << liveLocator
								  << " ] and is about to switch to RECORD [" << nextRecordLocator << " ]\r\n";
            try {
                Log() << "Pre-Syncing...Target Tuner for recording is not yet on record channel now,  need to cancel tune, so send a real a cancelLive" << std::endl;
                CancelLive cancelLive(GenerateUUID(), 
                        liveLocator, 
                        liveToken);
                std::vector<uint8_t> out;
                SerializeMessage(cancelLive,Connection::kRecorderClientId, out);
                ::serverInstance->getConnection(Connection::kRecorderClientId).sendAsync(out);

            }
            catch (ConnectionNotFoundException) {
                /* Connection may have lost. If so, don't send the message */
            }

		}
	}
	else {
		/* do nothing */
	}

	Log() << "Tuner " << tunerId << " is pre-sync'd in " << (const char *) getTuner(tunerId).getState().getState() << " state \r\n";
}

void Manager::syncHybrid(const std::string &tunerId, const std::string &reservationToken)
{
	if (getTuner(tunerId).getState().getState() == TunerState::kHybrid) {
		/* First get the recording token */
		std::string recordLocator="";
		TunerReservation::TokenList tokens;
		Manager::getInstance().getReservationTokens(tokens, tunerId);

		TunerReservation::TokenList::const_iterator it = tokens.begin();
		while (it != tokens.end()) {
			if (getReservation(*it).state == TunerReservation::ACTIVE) {
				if (getReservation(*it).getActivity().getActivity() == Activity::kRecord) {
					recordLocator = getReservation(*it).getServiceLocator();
					break;
				}
			}
			it++;
		}

		/* Since we are in hybrid mode, there must be 1 Record locator */
		Assert(!recordLocator.empty());
		/* Find active LIVE tokens and send notifyUpdate */
		it = tokens.begin();
		while (it != tokens.end()) {
			if (getReservation(*it).state == TunerReservation::ACTIVE) {
				if (getReservation(*it).getActivity().getActivity() == Activity::kLive) {
					std::string liveLocator = getReservation(*it).getServiceLocator();
					if (liveLocator.compare(recordLocator) != 0) {
						Log() << "Synchronization is needed in hybrid mode : LIVE[ " << liveLocator
								  << " ] vs RECORD [" << recordLocator << " ]\r\n";
						try {
							getReservation(*it).setServiceLocator(recordLocator);
                            if (getReservation(*it).getCustomAttributes() != NULL) {
                                Log() << "Resetting custome attributes to NULL " << std::endl;
                                extern void delete_ReservationCustomAttributes(void *p);
                                delete_ReservationCustomAttributes((void *)getReservation(*it).getCustomAttributes());
                                getReservation(*it).setCustomAttributes(NULL);
                            }
							std::vector<uint8_t> out;
							SerializeMessage(NotifyTunerReservationUpdate(GenerateUUID(), getReservation(*it)), getReservationAttributes(*it).getClientId(), out);
							Log() << "Sending the message:RL: " << std::endl;
							::serverInstance->getConnection(getReservationAttributes(*it).getClientId()).sendAsync(out);
							emit reservationUpdated();
							/* Message sent successfully */
						}
						catch (ConnectionNotFoundException) {
							/* Connection may have lost. If so, don't send the message */
						}

					}
				}
			}
			it++;
		}
	}
	else {
		/* do nothing */
	}

	Log() << "Tuner " << tunerId << " is sync'd in " << (const char *) getTuner(tunerId).getState().getState() << " state \r\n";
}

void Manager::startReservation(const std::string &reservationToken)
{
	/* Send Pretune */
	TunerReservation &reservation = getReservation(reservationToken);

	if (reservation.getActivity() == Activity::kLive) {
		/* Send Pretune */
		Log() << "TRM Request PRETUNE " << reservation.getServiceLocator() << std::endl;
		std::vector<uint8_t> out;
		try {
			if (reservation.getActivity().getDetail("ppv").compare("true") != 0) {
				SerializeMessage(NotifyTunerPretune(GenerateUUID(), reservation.getServiceLocator()), Connection::kTunerAgentId, out);
				::serverInstance->getConnection(Connection::kTunerAgentId).sendAsync(out);
			}
			else
			{
				Log() << "Do not Pretune for PPV Channel " << reservation.getServiceLocator() << std::endl;
			}
		}
		catch(ConnectionNotFoundException &e) {
			Log() << "Tuner Agent is not connected " << std::endl;
		}
	}
	emit reservationUpdated();
}

NotifyTunerStatesUpdate Manager::getTunerStatesUpdate(void)
{
	NotifyTunerStatesUpdate notifyTunerStatesUpdate(GenerateUUID());
	Tuner::IdList tunerIds;
	tunerIds.clear();
	Manager::getInstance().getTunerIds(tunerIds);
    Tuner::IdList::const_iterator tidIT = tunerIds.begin();
    for (tidIT = tunerIds.begin(); tidIT != tunerIds.end(); tidIT++) {
    	DetailedTunerState detailedTunerState;
    	const Tuner &tuner = Manager::getInstance().getTuner(*tidIT);
		Log() << "Adding detailedTunerState for Tuner "
				<< *tidIT << "State= "
				<< (const char *)tuner.getState().getState() << "locator=" << tuner.getServiceLocator() << std::endl;

    	{
			TunerReservation::TokenList tokens;
			tokens.clear();
			Manager::getInstance().getTuner(*tidIT).getReservationTokens(tokens);
			TunerReservation::TokenList::const_iterator itt;
			for (itt=tokens.begin(); itt != tokens.end(); itt++) {
				const TunerReservation &reservation = Manager::getInstance().getReservation(*itt);
				if (reservation.state == TunerReservation::ACTIVE) {
					SafeAssert(reservation.getServiceLocator() == tuner.getServiceLocator(false));
					detailedTunerState.addTunerOwner(((const char *)reservation.getActivity().getActivity()), reservation.getDevice());
					Log() << "Tuner " << *tidIT << "has activity " << ((const char *)reservation.getActivity().getActivity())
							  << "owned by " << reservation.getDevice() << std::endl;
			    	detailedTunerState.setState((const char *)tuner.getState().getState(), tuner.getServiceLocator());
				}
			}
    	}

    	if (getLocalTuner().compare(*tidIT) == 0) {
    		Log() << "LocalTuner " << getLocalTuner() << "set reservedDeviceId" << std::endl;
    		detailedTunerState.setReservedDeviceId(GetDeviceId());
    	}
    	notifyTunerStatesUpdate.addTunerDetailedState(*tidIT, detailedTunerState);
    }

    return notifyTunerStatesUpdate;
}

void Manager::onCancelLiveResponse(const CancelLiveResponse response, const ReserveTuner request, const std::string parentId)
{
	/* regardless cancellation success or failure, it is last change to grant record token */
	Log() << "Manager::onCancelLiveResponse from client  "
			  << " for activity " << (const char *) Activity::kLive
			  << " for serviceLocator " << response.getServiceLocator()
			  << " for UUID " << response.getUUID() << " for Tuner " << parentId <<  std::endl;
	Executor<ReserveTuner> exec(request, Connection::kRecorderClientId);
    Execute(exec, parentId);
}

std::string Manager::getLocalTuner(void)
{
	return localTuner;
}

void Manager::setLocalTuner(const std::string &tunerId)
{
	this->localTuner = tunerId;
}

void Manager::onTimerDeleted(void *ptr)
{
    delete ((Timer *)ptr);
}

bool PendingCancelRecordingProcessor::timeout()
{
	CancelRecordingResponse response(request.getUUID(), request.getReservationToken());
	response.getStatus() += "Cancellation Timedout, assuming cancelled=false\r\n";
	response.getStatus() = ResponseStatus::kOk;
	response.setCanceled(false);

	std::vector<uint8_t> out;
	SerializeMessage(response, clientId, out);
	::serverInstance->getConnection(clientId).sendAsync(out);

	return true;
}

bool PendingReserveTunerConflictProcessor::timeout()
{
	bool recordingCancelled = false;

	Assert(this == &Manager::getInstance().getPendingRequest(getUUID()));
	Assert(request.getTunerReservation().getActivity().getActivity() == Activity::kRecord);
	Assert(notification.getConflicts().size() == 1);

	TunerReservation &conflict = *notification.getConflicts().begin();

	Assert(conflict.getActivity().getActivity() == Activity::kRecord);
	Assert(notification.getTunerReservation().getActivity().getActivity() == Activity::kLive);

	/* First cancelLive if the current LIVE is not recording channel, then start the recording timer */
	{
		TunerReservation::TokenList tokens;
		tokens.clear();
	    Manager::getInstance().getReservationTokens(tokens, parentId);

	    if ((tokens.size() == 1)) {
	    	/* Force tune to the Recording channel if it is not EAS, and if user is not already on recording channel*/
	    	if (Manager::getInstance().getReservation(*tokens.begin()).getActivity().getDetail("eas").compare("true") == 0) {
	    		Log() << "The current Live channel " <<
	    				Manager::getInstance().getReservation(*tokens.begin()).getServiceLocator() <<
	    				" is EAS channel" << std::endl;
	    	    if (request.getTunerReservation().getServiceLocator().compare(Manager::getInstance().getReservation(*tokens.begin()).getServiceLocator()) != 0) {
					Log() << "The recording channel " <<
							request.getTunerReservation().getServiceLocator() <<
							"is not EAS channel, cancel Recording " << std::endl;
					ReserveTunerResponse response(request.getUUID());
					response.getStatus() += "Cancel Recording For EAS\r\n";
					response.getStatus() = ResponseStatus::kInsufficientResource;
					{
						std::vector<uint8_t> out;
						SerializeMessage(response, Connection::kRecorderClientId, out);
						::serverInstance->getConnection(Connection::kRecorderClientId).sendAsync(out);
					}
					recordingCancelled = true;
	    	    }
	    	    else {
					Log() << "The recording channel " <<
							request.getTunerReservation().getServiceLocator() <<
							"is EAS channel, allow Recording " << std::endl;

	    	    }
	    	}

	    	if (!recordingCancelled) {

				if ((Manager::getInstance().getReservation(*tokens.begin()).getActivity().getActivity() == Activity::kLive)) {
					Log() << "PendingReserveTunerConflictProcessor " << getUUID() << " Timedout...Cancel LIVE first before grating Record " << std::endl;
					Assert(notification.getConflicts().size() == 1);
					try {
						if (Manager::getInstance().getReservation(*tokens.begin()).getServiceLocator().compare(request.getTunerReservation().getServiceLocator()) != 0) {
							Log() << "Target Tuner for recording is not on record channel now,  so need to cancel tune, so send a real a cancel" << std::endl;
					       CancelLive cancelLive(GenerateUUID(), 
                                                 Manager::getInstance().getReservation(*tokens.begin()).getServiceLocator(), 
                                                 Manager::getInstance().getReservation(*tokens.begin()).getReservationToken());

							const int CANCEL_LIVE_TIMEOUT_MS = (10000);

							std::vector<uint8_t> out;
							SerializeMessage(cancelLive,Connection::kRecorderClientId, out);
							::serverInstance->getConnection(Connection::kRecorderClientId).sendAsync(out);
							/* Now add cancelLive to pending request */

							Manager::getInstance().addPendingRequest(new PendingCancelLiveProcessor(Connection::kTrmClientId, cancelLive, request, parentId), CANCEL_LIVE_TIMEOUT_MS);
						}
						else {
#if 0
                            /* no race condition */
							Log() << "Target Tuner for recording is on record channel now,  no need to cancel tune, so fake a cancel success and start Recording" << std::endl;
							PendingCancelLiveProcessor fakeCancelProcessor(Connection::kTrmClientId, cancelLive, request, parentId);
							fakeCancelProcessor.timeout();
#else
                            /* race condition */
					       CancelLive cancelLive(GenerateUUID(), 
                                                 notification.getTunerReservation().getServiceLocator(), 
                                                 notification.getTunerReservation().getReservationToken());
							Log() << "Target Tuner for recording is on record channel now, or it is on its way to the recoridng chanel." << std::endl;
							const int CANCEL_LIVE_TIMEOUT_MS = (10000);

							std::vector<uint8_t> out;
							SerializeMessage(cancelLive,Connection::kRecorderClientId, out);
							::serverInstance->getConnection(Connection::kRecorderClientId).sendAsync(out);
							/* Now add cancelLive to pending request */

							Manager::getInstance().addPendingRequest(new PendingCancelLiveProcessor(Connection::kTrmClientId, cancelLive, request, parentId), CANCEL_LIVE_TIMEOUT_MS);

#endif
						}
					}
					catch (ConnectionNotFoundException &e) {
						Log() << "No Recorder alive, ignore " << std::endl;
					}
				}
				else {
					Log() << "Target Tuner for recording is already recording" << std::endl;
					Assert(0);
				}
	    	}
	    }
        else if (tokens.size() == 2) {
            /* The tuner is already in H state. New recording will take over once the current R is completed */
            /* Make sure current recording stop before next recording */
            Assert(Manager::getInstance().getTuner(parentId).getState().getState() == TunerState::kHybrid);
            Log() << "Target Tuner for recording is still recording ,  cannot cancel tune, so fake a cancel success and grant Recording" << std::endl;
			CancelLive cancelLive(GenerateUUID(), Manager::getInstance().getReservation(*tokens.begin()).getServiceLocator(), Manager::getInstance().getReservation(*tokens.begin()).getReservationToken());
            PendingCancelLiveProcessor fakeCancelProcessor(Connection::kTrmClientId, cancelLive, request, parentId);
            fakeCancelProcessor.timeout();
	    }
	    else if (tokens.size() == 0) {
	    	Log() << "Target Tuner for recording is free now,  no need to cancel live, so fake a cancel success and grant Recording" << std::endl;
	    	Assert(Manager::getInstance().getTuner(parentId).getState().getState() == TunerState::kFree);
			CancelLive cancelLive(GenerateUUID(), 
                                  request.getTunerReservation().getServiceLocator(), 
                                  ""/* use empty token for the fake success*/);
            PendingCancelLiveProcessor fakeCancelProcessor(Connection::kTrmClientId, cancelLive, request, parentId);
            fakeCancelProcessor.timeout();
	    }
	    else {
	    	Log() << "Incorrect token number " << tokens.size() << std::endl;
	    	Assert(0);
	    }
	}
	return true;

}

bool PendingCancelLiveProcessor::timeout()
{
	/* CancelLive timeout */
	emit Manager::getInstance().cancelLiveResponse(CancelLiveResponse(
			                    cancelLiveRequest.getUUID(),
			                    ResponseStatus(ResponseStatus::kOk, "Assuming cancellation success upon timeout"),
			                    cancelLiveRequest.getReservationToken(),
			                    cancelLiveRequest.getServiceLocator(),
			                    true), reserveRequest, parentId);
	return true;

}

TRM_END_NAMESPACE


/** @} */
/** @} */
