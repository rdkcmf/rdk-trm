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


#include <limits>
#include <map>

#include "trm/TRM.h"
#include "trm/Enum.h"
#include "trm/TunerState.h"
#include "trm/Messages.h"
#include "trm/JsonEncoder.h"

#include "Server.h"
#include "Manager.h"
#include "Filter.h"
#include "Util.h"
#include "Executors.h"

TRM_BEGIN_NAMESPACE
std::string GetDeviceId();
TRM_END_NAMESPACE

LOCAL_BEGIN_NAMESPACE

using namespace TRM;



class Policy {
public:
	/*
	 * If a device already owns a valid token for certain activity, the
	 * device must include the token in the new request for the same
	 * activity.
	 */
	static bool RequireTokenInRequest(void) {
		return false;
	}

	static bool allowLinearRecordSharing(void) {
		return true;
	}

    static bool enableConflictsForHotRecording(void) {
        return true;
    }

    /* 
     * policy.allowOverlapRecordings() - true, means that
     * there can be two active recodings of same sourceid 
     * 
     * This can happen if User explicitly request recordings of 
     * same source Id with overlapping time window, by exetending
     * the stop time of current recoridng in back-to-back recording
     * scenarios.
     */
    static bool allowOverlapRecordings(void) {
        return true;
    }
};

void FindConflictsForRequestedLive(const ReserveTuner &request, ReserveTunerResponse::ConflictCT &conflicts)
{
	/*
	 * Populate with conflicts for the LIVE request.
	 * Only the following reservations can be returned as conflicts:
	 * 1) R (Active or Pending) reservations on R tuners without Pending L.
	 * 2) R (Pending) reservations on F tuner without Pending L.
	 * 3) L reservation on the owning tuner (could be L or H).
	 * 4) R reservation on the owning tuner (could be L or H)
	 *
	 */

	TunerReservation::TokenList R_tokens_on_R_tuners; //Active or Pending
	TunerReservation::TokenList R_tokens_on_F_tuners; //Pending
	TunerReservation::TokenList L_tokens_on_Self_tuner;//Active, self tuner could be in H or L
	TunerReservation::TokenList R_tokens_on_Self_tuner;//Active, self tuner could be in H or L

	Tuner::IdList tunerIds;

	tunerIds.clear();
	Filter<ByActivity>(Activity::kRecord,
			Manager::getInstance().
			    getReservationTokens(R_tokens_on_R_tuners,
			            Filter<ByTunerState>(TunerState::kRecord, Manager::getInstance().getTunerIds(tunerIds))));
	tunerIds.clear();
	Filter<ByActivity>(Activity::kRecord,
			Manager::getInstance().
			    getReservationTokens(R_tokens_on_F_tuners,
			            Filter<ByTunerState>(TunerState::kFree, Manager::getInstance().getTunerIds(tunerIds))));


	if (1 || (!request.getTunerReservation().getReservationToken().empty())) {
		/* Get the L reservation on same parent tuner */
		tunerIds.clear();
		Filter<ByDevice>(request.getDevice(),
				    	  Filter<ByActivity>(Activity::kLive,
						  Manager::getInstance().getReservationTokens(L_tokens_on_Self_tuner, Manager::getInstance().getTunerIds(tunerIds))));

		Assert(L_tokens_on_Self_tuner.size() <= 1);

		if (L_tokens_on_Self_tuner.size() == 1) {
			tunerIds.clear();
			Filter<ByActivity>(Activity::kRecord,
			    Manager::getInstance().getReservationTokens(R_tokens_on_Self_tuner,
			        Manager::getInstance().getParent(*L_tokens_on_Self_tuner.begin())));
		}
	}

	{
		const int kStateActive = TunerReservation::ACTIVE;
		Filter<ByReservationState> (kStateActive, R_tokens_on_R_tuners);
		Filter<ByReservationState> (kStateActive, R_tokens_on_F_tuners);
		Filter<ByReservationState> (kStateActive, R_tokens_on_Self_tuner);
	}

	TunerReservation::TokenList::const_iterator it;

	for (it = R_tokens_on_R_tuners.begin(); it != R_tokens_on_R_tuners.end(); it++) {
		conflicts.push_back(Manager::getInstance().getReservation(*it));
	}

	for (it = R_tokens_on_F_tuners.begin(); it != R_tokens_on_F_tuners.end(); it++) {
		conflicts.push_back(Manager::getInstance().getReservation(*it));
	}

	for (it = R_tokens_on_Self_tuner.begin(); it != R_tokens_on_Self_tuner.end(); it++) {
		conflicts.push_back(Manager::getInstance().getReservation(*it));
	}
}

void FindConflictsForRequestedRecord(const ReserveTuner &request, ReserveTunerResponse::ConflictCT &conflicts)
{
	/*
	 * Populate with conflicts for the LIVE request.
	 * Only the following reservations can be returned as conflicts with Recording:
	 * 1) L reservations on L tuners.
	 */

	TunerReservation::TokenList L_tokens_on_L_tuners;
	Tuner::IdList tunerIds;

    if (!request.getTunerReservation().getDevice().empty()) {
		Log() << "Look for L tuners owned by " << request.getTunerReservation().getDevice() << std::endl;
		Filter<ByDevice>(request.getTunerReservation().getDevice(),
		    Filter<ByInConflict>(false,
		        Filter<ByActivity>(Activity::kLive,
		                Manager::getInstance().
		                    getReservationTokens(L_tokens_on_L_tuners,
		                            Filter<ByTunerState>(TunerState::kLive, Manager::getInstance().getTunerIds(tunerIds))))));
		if (L_tokens_on_L_tuners.size() != 0) {
		    Log() << "Found L tuner owned by " << request.getTunerReservation().getDevice() << std::endl;

		    /* Skip (the only 1) L tuners that already have a pending R */
		    {

	        	TunerReservation::TokenList::iterator it = L_tokens_on_L_tuners.begin();
	        	while (it != L_tokens_on_L_tuners.end()) {
					TunerReservation::TokenList R_tokens_on_L_tuner;
					R_tokens_on_L_tuner.clear();
					Filter<ByActivity>(Activity::kRecord,
			                Manager::getInstance().
			                    getReservationTokens(R_tokens_on_L_tuner,
			                    		             Manager::getInstance().getParent(*it)));
				    if (R_tokens_on_L_tuner.size() != 0) {
				    	/* Skip */
				    	it = L_tokens_on_L_tuners.erase(it);
					    Log() << "Skip L tuner owned by " << request.getTunerReservation().getDevice() << " Because it has R pending " << std::endl;

				    }
				    else {
				    	it++;
				    }

	        	}
	        }
		}
    }

    if (request.getTunerReservation().getDevice().empty() || L_tokens_on_L_tuners.size() == 0) {
        Filter<ByInConflict>(false,
            Filter<ByActivity>(Activity::kLive,
                    Manager::getInstance().
                        getReservationTokens(L_tokens_on_L_tuners,
                                Filter<ByTunerState>(TunerState::kLive, Manager::getInstance().getTunerIds(tunerIds)))));

	    /* Skip L tuners that already have a pending R */
        {
        	TunerReservation::TokenList::iterator it = L_tokens_on_L_tuners.begin();
        	while (it != L_tokens_on_L_tuners.end()) {
				TunerReservation::TokenList R_tokens_on_L_tuner;
				R_tokens_on_L_tuner.clear();
				Filter<ByActivity>(Activity::kRecord,
		                Manager::getInstance().
		                    getReservationTokens(R_tokens_on_L_tuner,
               		                             Manager::getInstance().getParent(*it)));
			    if (R_tokens_on_L_tuner.size() != 0) {
			    	/* Skip */
			    	it = L_tokens_on_L_tuners.erase(it);
				    Log() << "Skip L tuner owned by " << request.getTunerReservation().getDevice() << " Because it has R pending " << std::endl;

			    }
			    else {
			    	it++;
			    }

        	}
        }

    }

	TunerReservation::TokenList::const_iterator it;
		//Only return 1 L for 1 R conflict.
		//@TODO: Better selection than simply returning the first L?
	it = L_tokens_on_L_tuners.begin();
    if (it != L_tokens_on_L_tuners.end()) {
		conflicts.push_back(Manager::getInstance().getReservation(*it));
    }	

	if (conflicts.size() == 0) {
		/* No L conflicts available from L tuners. Now look for H tuners
		 * For H tuner, the L is a conflict if the R on the H will terminate before the new start time
		 */
		TunerReservation::TokenList R_tokens_on_H_tuners_That_Ends_Before_New_StartTime;

		Filter<ByReservationEndBefore>(request.getTunerReservation().getStartTime(),
			Filter<ByActivity>(Activity::kRecord,
					Manager::getInstance().
						getReservationTokens(R_tokens_on_H_tuners_That_Ends_Before_New_StartTime,
								Filter<ByTunerState>(TunerState::kHybrid, Manager::getInstance().getTunerIds(tunerIds)))));

			//@TODO: Better selection than simply returning the first L?
			/* Return the peer L reservation as conflict */
		it = R_tokens_on_H_tuners_That_Ends_Before_New_StartTime.begin();

            Log()  << "Found A future conflict to return \r\n" << std::endl;
			TunerReservation::TokenList L_tokens_on_H_tuners;

			tunerIds.clear();
			L_tokens_on_H_tuners.clear();

        if (it != R_tokens_on_H_tuners_That_Ends_Before_New_StartTime.end()) {
			Filter<ByActivity>(Activity::kLive,
					Manager::getInstance().getReservationTokens(L_tokens_on_H_tuners,
							Filter<ByTunerId>(Manager::getInstance().getParent(*it), Manager::getInstance().getTunerIds(tunerIds))));
        }
		Assert(tunerIds.size() == 1);

		Assert(L_tokens_on_H_tuners.size() == 1);
		conflicts.push_back(Manager::getInstance().getReservation(*L_tokens_on_H_tuners.begin()));
	}
}
/*
 * Reserve Tuner For Live:
 *
 * When asking for a NEW reservation:
 * The new tuner is either taken from a Free tuner
 * or share with a R tuner with matching locator.
 *
 * H,R,L-Tuners may have PENDING R tokens.
 *
 *
 * IF this is RENEW (the requester already owns a L)
 * --
 * -- IF exiting L is on L-Tuner,
 * -- -- IF has pending R, check if source locator match new locator.
 * -- -- -- IF locator match, grant token, T state stays at L
 * -- -- -- ELSE must move to F-Tuner or R-Tuner.
 * -- -- ELSE, grant L on same tuner (a.k.a renew current L with same or different locator)
 * -- ELSE exiting L is on H-Tuner
 * -- -- must move to F-Tuner or R-Tuner.
 *
 * CONTINUE. Need to look for F-Tuner or R-Tuner with matching Locator.
 * IF there is a R tuner with ALL and ONLY (ACTIVE|PENDING) tokens matching locator, move to this R-TUNER
 * ELSE there is a F tuner with ALL and ONLY PENDING R tokens matching locator, move to this F-tuner.
 *
 * CONTINUE. Now prepare conflicts.
 * L conflicts to all ACTIVE & PENDING R tokens on (F,R) tuners.
 *
 */
//TODOTODO:
//Now we have F with pending R, L with Pending R and R with pending R.
void reserveLive(const ReserveTuner &request, ReserveTunerResponse &response, int32_t clientId)
{
	response.getStatus() += "Start reservation process for <Live> \r\n";

    {
        const int alwaysAllowFuture = 1;
        const int firstThreshold  = (60 * 1000);    // 60 second
        const int secondThreshold = (300 * 1000);  // 5 minute 

        int64_t   diff = ((int64_t)request.getTunerReservation().getStartTime()) - ((int64_t)GetCurrentEpoch());
        if (diff <= 0) {
            /* Request startTime is in the past. OK */
        }
        else {
            if (alwaysAllowFuture || diff <= firstThreshold) {
                if (!alwaysAllowFuture) {
                    Log() << "Requesting <Live> reservation is less than 60 seoncds in the future...this is now allowed\r\n";
                    response.getStatus() += "Requesting <Live> reservation is less than 60 seconds in the future...this is now allowed \r\n";
                }
                else {
                    Log() << "Requesting <Live> reservation starts from 'NOW'\r\n";
                    response.getStatus() += "Requesting <Live> reservation starts from 'NOW'\r\n";
                }
                TunerReservation & reservation  = const_cast<TunerReservation &>(request.getTunerReservation());
                reservation.setStartTime(GetCurrentEpoch() - 1);
            }
            else {
                Log() << "Requesting <Live> reservation is more than 60 seoncds in the future...this is not allowed\r\n";
                response.getStatus() += "Requesting <Live> reservation is more than 60 seconds in the future...this is not allowed \r\n";
                response.getStatus() = ResponseStatus::kMalFormedRequest;
                throw IllegalArgumentException();
            }
        }
    }

	/*
	 * Basically the resolution is to find the existing token, if any, to release,
	 * and to find the new destination to host the token.
	 */
	std::string tokenToAddTo   = "";
	std::string tokenToRelease = "";
	std::string tunerToAddTo   = "";
	std::string tokenToResurrect  = "";

	tokenToAddTo = request.getTunerReservation().getReservationToken();
	int isResurrect =  (request.getResurrect().compare("true") == 0);

	{
		Tuner::IdList tunerIds;
		TunerReservation::TokenList tokens;

		tunerIds.clear();
		tokens.clear();
		Filter<ByDevice>(request.getDevice(),
						  Filter<ByActivity>(Activity::kLive,
						  Manager::getInstance().getReservationTokens(tokens, Manager::getInstance().getTunerIds(tunerIds))));

		if (tokens.size() != 0) {
			Log() << "Found Existing <Live> reservation owned by requesting device\r\n";
			response.getStatus() += "Found Existing <Live> reservation owned by requesting device \r\n";
			/* A device can only have 1 L token, either active or pending */
			Assert(tokens.size() == 1);
			tokenToRelease = (*tokens.begin());
		}
		else
		{
			if((isResurrect) && (!tokenToAddTo.empty()))
			{
				tokenToResurrect = tokenToAddTo;
				tokenToAddTo   = "";
				Log() << "Existing <Live> reservation does not exist\r\n";
				Log() << "Use Resurrect Token = " << tokenToResurrect << std::endl;
			}
		}
	}

    if ((!tokenToRelease.empty())  && (Manager::getInstance().getReservation(tokenToRelease).state == TunerReservation::IDLE)) {
		Log() << "Existing <Live> reservation is not yet started\r\n";
		response.getStatus() += "Existing <Live> reservation is not yet started \r\n";
		response.getStatus() = ResponseStatus::kInvalidState;
		throw InvalidStateException();
    }

	if ( ((tokenToRelease.empty())   && (!tokenToAddTo.empty())) ||
		 ((!tokenToRelease.empty())  && (Manager::getInstance().getReservation(tokenToRelease).state == TunerReservation::IDLE)) ||
		 ((!tokenToRelease.empty())  && (!tokenToAddTo.empty()) && (tokenToRelease.compare(tokenToAddTo) != 0)))
	{
		Log() << "Existing <Live> reservation does not exist\r\n";
		response.getStatus() += "Existing <Live> reservation does not match that in request \r\n";
		response.getStatus() = ResponseStatus::kInvalidToken;
		throw ItemNotFoundException();
	}

	tokenToAddTo = tokenToRelease;

	Log() << "tokenToRelease = " << tokenToRelease << std::endl;
	Log() << "tokenToAddTo = " << tokenToAddTo << std::endl;

	if (!tokenToRelease.empty()) {
		/* Requesting device has an existing token */
		if (request.getTunerReservation().
				getServiceLocator().
				    compare(Manager::getInstance().getReservation(tokenToRelease).getServiceLocator()) == 0) {
			Log() << "Existing <Live> reservation has same locator, reuse it \r\n";
			response.getStatus() += "Existing <Live> reservation has same locator, reuse it \r\n";
			tunerToAddTo = Manager::getInstance().getTuner(Manager::getInstance().getParent(tokenToRelease)).getId();
		}
	}

	/* Need a tuner for LIVE token. Check for sharing R tuner*/

	if (tunerToAddTo.empty()) {
		Tuner::IdList tunerIds;
		TunerReservation::TokenList tokens;
		tunerIds.clear();
		tokens.clear();

        Log() << "look for R tuner that has tokens of same locator \r\n";
		response.getStatus() += "look for R tuner that has tokens of same locator \r\n";

		Filter<ByTunerState>(TunerState::kRecord, Manager::getInstance().getTunerIds(tunerIds));

		if (tunerIds.size() != 0) {
            Tuner::IdList::const_iterator tidIT = tunerIds.begin();
            while (tidIT != tunerIds.end()) {
            	/* A security check: R tuner should not have active L reservation*/
            	try {
            		Manager::getInstance().getTuner(*tidIT).getReservation(Activity::kLive, TunerReservation::ACTIVE);
            		Assert(0);
            	}
            	catch(ItemNotFoundException &e) {
            	}

            	/* R tuner should always have active R reservation*/
            	if (Manager::getInstance().
            			getTuner(*tidIT).
            			    getReservation(Activity::kRecord, TunerReservation::ACTIVE).
            			        getServiceLocator().
            			            compare(request.getTunerReservation().getServiceLocator()) == 0) {
                    Log() << "found R tuner that has tokens of same locator \r\n";
                    response.getStatus() += "found R tuner that has tokens of same locator \r\n";
                    tunerToAddTo = *tidIT;
                    break;
            	}

                tidIT++;
            }
        }
	}

	/* There is no existing R tuner to share, check for currently owned tuner with future R of same locator */

	std::string fallbackTuner   = "";

	if (tunerToAddTo.empty()) {
		if (((!tokenToRelease.empty())) &&
			   Manager::getInstance().
			       getTuner(Manager::getInstance().getParent(tokenToRelease)).
			           getState() == TunerState::kLive) {
			/* Check existing L tuner to see if we can reuse. This is a L tuner so its R token, if any, has to be IDLE */
			try {
				//SM-N4
				if (Manager::getInstance().
					    getTuner(Manager::getInstance().getParent(tokenToRelease)).
					        getReservation(Activity::kRecord, TunerReservation::IDLE).
					            getServiceLocator().
					                compare(request.getTunerReservation().getServiceLocator()) == 0) {
                    Log() << "Current L tuner that has Future R token of same locator \r\n";
                    response.getStatus() += "Current L tuner that has Future R token of same locator \r\n";
                    tunerToAddTo = Manager::getInstance().getTuner(Manager::getInstance().getParent(tokenToRelease)).getId();
				 }
				else {
                    Log() << "Current L tuner that has Future R token of different locator \r\n";
                    response.getStatus() += "Current L tuner that has Future R token of different locator \r\n";
                    fallbackTuner = Manager::getInstance().getTuner(Manager::getInstance().getParent(tokenToRelease)).getId();
				}

			}
			catch(ItemNotFoundException &e) {
				//There is no IDLE Record Token.
                Log() << "Current L tuner that no Future R tokens\r\n";
                response.getStatus() += "Current L tuner that no Future R tokens \r\n";
                fallbackTuner = Manager::getInstance().getTuner(Manager::getInstance().getParent(tokenToRelease)).getId();
			}
		}
	}

	/* There is no currently owned tuner. or the current tuner does not have future R of same locator,
	 * check for F tuner with future R of same locator.
	 */
	if (tunerToAddTo.empty()) {
        //SM-N6
		Tuner::IdList tunerIds;
		TunerReservation::TokenList tokens;
		tunerIds.clear();
		tokens.clear();

        Log() << "look for F tuner that has future tokens of same locator \r\n";
		response.getStatus() += "look for F tuner that has future tokens of same locator \r\n";

		Filter<ByTunerState>(TunerState::kFree, Manager::getInstance().getTunerIds(tunerIds));

		if (tunerIds.size() != 0) {
            Tuner::IdList::const_iterator tidIT = tunerIds.begin();
            while (tidIT != tunerIds.end()) {
            	/* A security check: F tuner should not have active L or R reservation*/
            	try {
            		Manager::getInstance().getTuner(*tidIT).getReservation(Activity::kLive, TunerReservation::ACTIVE);
            		Assert(0);
            	}
            	catch(ItemNotFoundException &e) {
            	}

            	try {
            		Manager::getInstance().getTuner(*tidIT).getReservation(Activity::kRecord, TunerReservation::ACTIVE);
            		Assert(0);
            	}
            	catch(ItemNotFoundException &e) {
            	}

            	/* F tuner may have future R reservation*/
            	try {
					if (Manager::getInstance().
							getTuner(*tidIT).
								getReservation(Activity::kRecord, TunerReservation::IDLE).
									getServiceLocator().
									    compare(request.getTunerReservation().getServiceLocator()) == 0) {
						Log() << "found F tuner that has future R tokens of same locator \r\n";
						response.getStatus() += "found F tuner that has future R tokens of same locator \r\n";
						tunerToAddTo = *tidIT;
						break;
					}
            	}
            	catch(ItemNotFoundException &e) {
            	}

                tidIT++;
            }
        }
	}

	//SM-N7
	/* There is no F tuner with future R of same locator,
	 * check a F without future R.
	 */
	if (tunerToAddTo.empty()) {
		Tuner::IdList tunerIds;
		tunerIds.clear();

		Log() << "look for F tuner that has no  future tokens \r\n";
		response.getStatus() += "look for F tuner that has no  future tokens \r\n";

		Filter<ByTunerState>(TunerState::kFree, Manager::getInstance().getTunerIds(tunerIds));

		if (tunerIds.size() != 0) {
			Tuner::IdList::const_iterator tidIT = tunerIds.begin();
			while (tidIT != tunerIds.end()) {
				/* A security check: F tuner should not have active L or R reservation*/
				try {
					Manager::getInstance().getTuner(*tidIT).getReservation(Activity::kLive, TunerReservation::ACTIVE);
					Assert(0);
				}
				catch(ItemNotFoundException &e) {
				}

				try {
					Manager::getInstance().getTuner(*tidIT).getReservation(Activity::kRecord, TunerReservation::ACTIVE);
					Assert(0);
				}
				catch(ItemNotFoundException &e) {
				}

				TunerReservation::TokenList tokens;
				tokens.clear();
				Manager::getInstance().getTuner(*tidIT).getReservationTokens(tokens);
				if (tokens.size() == 0) {
					tunerToAddTo = *tidIT;
					break;
				}

				tidIT++;
			}
		}
	}

    if (!tunerToAddTo.empty()) { 
        /*If requester is remote, and tunerToAddTo is the last Free tuner, and this Free tuner is reserved local tuner
         * we cannot use it unless the requester has no tuner and we can relocate the reserved local tuner
         */
        if (request.getDevice().compare(GetDeviceId()) != 0) {
            Tuner::IdList tunerIds;
            tunerIds.clear();

            Filter<ByTunerState>(TunerState::kFree, Manager::getInstance().getTunerIds(tunerIds));
            if ((tunerIds.size() == 1) && (tunerToAddTo.compare(Manager::getInstance().getLocalTuner()) == 0)) {
                Log() << "There are (" << tunerIds.size() << " - 1) F tuners (" << tunerToAddTo << ") to move reservedDeviceId to for remote device" << std::endl;
                if (fallbackTuner.empty()) {
                    /* check if we can relocate the reserved local tuner to a R tuner */
                    tunerIds.clear();
                    Filter<ByTunerState>(TunerState::kRecord, Manager::getInstance().getTunerIds(tunerIds));
                    Log() << "There are (" << tunerIds.size() << " R tuners to move reservedDeviceId to for remote device" << std::endl;
                    if (tunerIds.size() >= 1) {
                        /* set the first one to be reserved */
                        Log() << "set reservedDeviceId to R tuner" << std::endl;
                        Manager::getInstance().setLocalTuner(*(tunerIds.begin()));
                    }
                    else {
                        tunerToAddTo ="";
                    }
                }
                else {
                    tunerToAddTo ="";
                }
            }
        }
    }

	if (tunerToAddTo.empty()) {
		tunerToAddTo = fallbackTuner;
	}

	if (tunerToAddTo.empty()) {

		Tuner::IdList tunerIds;
		tunerIds.clear();

		Log() << "look for F tuner that has ANY future tokens (of diff serviceLocator) \r\n";
		response.getStatus() += "look for F tuner that has ANY future tokens (of diff serviceLocator) \r\n";

		Filter<ByTunerState>(TunerState::kFree, Manager::getInstance().getTunerIds(tunerIds));

		if (tunerIds.size() != 0) {
            Tuner::IdList::const_iterator tidIT = tunerIds.begin();
            while (tidIT != tunerIds.end()) {
            	try {
            		Manager::getInstance().getTuner(*tidIT).getReservation(Activity::kRecord, TunerReservation::IDLE);
					Log() << "found F tuner that has future R tokens of diff locator \r\n";
					response.getStatus() += "found F tuner that has future R tokens of diff locator \r\n";
					tunerToAddTo = *tidIT;
					break;
            	}
            	catch(ItemNotFoundException &e) {
            	}

                tidIT++;
            }
        }

	}

	if (!tunerToAddTo.empty()) {
		/*
		 * Check tuner allocation and make sure that the host device has a dedicated tuner for Live:
		 *  The granted tuner is either in one of the following states
		 *  Free state:  The requester does not yet have a L token.
		 *  Record state:The requester does not yet have a L token.
		 *  Live state:  The requester already has a L token and is reusing it.
		 */
		if (tunerToAddTo.compare(fallbackTuner) == 0) {
			/* Reusing current tuner. No changes */
			Log() << "[reserveLocal] Reusing current tuner. No changes" << std::endl;
			if ((request.getDevice().compare(GetDeviceId()) == 0) && Manager::getInstance().getLocalTuner().empty()) {
                Log() << "[reserveLocal] set reservedDeviceId for local tuner" << std::endl;
                Manager::getInstance().setLocalTuner(tunerToAddTo);
		}
		}
		else if (!GetDeviceId().empty()) {
			/* switching to new tuner for Live. The new tuner is either Free or Record */
			Log() << "[reserveLocal] switching to new tuner for Live. The new tuner (Free or Record) is curently in state "
				  << (const char *)(Manager::getInstance().getTuner(tunerToAddTo).getState().getState())
				  << std::endl;

			if (request.getDevice().compare(GetDeviceId()) == 0) {
				Log() << "[reserveLocal] requester is local host, moving reservedDeviceId "
					  << GetDeviceId() << " from "
					  << Manager::getInstance().getLocalTuner()
					  << " to " << tunerToAddTo << std::endl;

				Manager::getInstance().setLocalTuner(tunerToAddTo);
			}
			else {
				Log() << "[reserveLocal] requester is remote host" << std::endl;

				/* If remote host is taking the reservedDeviceId tuner, move the reserveDeviceId to a new one */
				/* The remove host is taking either a F or a R tuner for requested Live*/

				/* If granted tuner is last tuner that can take LIVE (i.e. a F or R),
				 * we have to make sure there is already one reserved for local host */
				if (tunerToAddTo.compare(Manager::getInstance().getLocalTuner()) == 0) {
					/* If tunerToAddTo is a F or R, we have to find another F or R */
					bool foundReserved = false;
					if (!foundReserved) {
						Tuner::IdList tunerIds;
						tunerIds.clear();

						Filter<ByTunerState>(TunerState::kFree, Manager::getInstance().getTunerIds(tunerIds));
						if (tunerIds.size() != 0) {
							Log() << "There are (" << tunerIds.size() << " - 1) F tuners to move reservedDeviceId to " << std::endl;
							tunerIds.remove(tunerToAddTo);
							if (tunerIds.size() != 0) {
								Manager::getInstance().setLocalTuner(*tunerIds.begin());
								foundReserved = true;
							}
							else {
								/* no more Free, look for R */
							}
						}
					}


					if (!foundReserved) {
						Log() << "There is no more F tuner to move reservedDeviceId to " << std::endl;
						Tuner::IdList tunerIds;
						tunerIds.clear();
						/* Move reservedTuner to one of the other R's */
						Filter<ByTunerState>(TunerState::kRecord, Manager::getInstance().getTunerIds(tunerIds));
						if (tunerIds.size() != 0) {
							Log() << "There are (" << tunerIds.size() << " - 1) R tuners to move reservedDeviceId to " << std::endl;
							tunerIds.remove(tunerToAddTo);
							if (tunerIds.size() != 0) {
								Manager::getInstance().setLocalTuner(*tunerIds.begin());
								foundReserved = true;
							}
							else {
								/* no more R */
							}
						}
					}

					if (!foundReserved) {
						Log() << "cannot find a new home for reserved tuner, reject current request " << std::endl;
						tunerToAddTo = "";
					}
				}
			}
		}
        else {
            Log() << "Warning: DeviceId Not available, ignore reserveDeviceId assignment " << std::endl;
            Manager::getInstance().setLocalTuner(std::string(""));
		}
	}

	if (!tunerToAddTo.empty()) {
		Log() << "Granting New Reservation to Live on tuner " << tunerToAddTo << std::endl;

		TunerReservation newReservation = request.getTunerReservation();
		if (!tokenToRelease.empty()) {
			newReservation.setReservationToken(tokenToRelease);
			Manager::getInstance().releaseReservation(tokenToRelease, false/*transition release do not notify client*/);
		}
		else if((isResurrect) && (!tokenToResurrect.empty())) {
				newReservation.setReservationToken(tokenToResurrect);
				Log() << "Granting Reservation on Live with Resurrect Token = " << tokenToResurrect << std::endl;
			}
		else {
			newReservation.setReservationToken(GenerateUUID());
		}

		const bool considerFutureToken = false;
		if(!Manager::getInstance().getTuner(tunerToAddTo).getServiceLocator(considerFutureToken).empty()) {
			if (newReservation.getServiceLocator().
					compare(Manager::getInstance().getTuner(tunerToAddTo).getServiceLocator(considerFutureToken)) != 0) {
				Log() << "Modify response locator from "  << newReservation.getServiceLocator() <<  " to " << Manager::getInstance().getTuner(tunerToAddTo).getServiceLocator(considerFutureToken) << "\r\n";
				response.getStatus() += "Modify response locator\r\n";
				newReservation.setServiceLocator(Manager::getInstance().getTuner(tunerToAddTo).getServiceLocator(considerFutureToken));
			}
		}

		Manager::getInstance().addReservation(newReservation, tunerToAddTo);
		printf("===============+Reservation %s from Connection %x ADDED\r\n", newReservation.getReservationToken().c_str(), clientId);
		Manager::getInstance().setReservationAttributes(newReservation.getReservationToken(), ReservationAttributes(clientId));

		response.setTunerReservation(newReservation);
		response.getStatus() = ResponseStatus::kOk;
	}
	else {
		response.getStatus()  += "Error: Reservation Failed with conflicts\r\n";
		response.getStatus()  = ResponseStatus::kGeneralError;
		FindConflictsForRequestedLive(request, response.getConflicts());
		
		/* Send Response as Insufficient resource during no conflicts */
		if (response.getConflicts().size() == 0){
			response.getStatus()  = ResponseStatus::kInsufficientResource;
		}
		else {
			response.getStatus()  = ResponseStatus::kOk;
		}


		/* For each conflicts, change the state from ACTIVE to IN_CONFLICT */
	}
}

void renewRecord(const ReserveTuner &request, ReserveTunerResponse &response, int32_t clientId)
{

	Tuner::IdList tunerIds;
	TunerReservation::TokenList tokens;
	std::string tokenToRenew = "";

	/* RENEW */
	Log() << "Look for Existing <Record> reservation for the same locator\r\n";
	response.getStatus() += "Look for Existing <Record> reservation for the same locator\r\n";

	tunerIds.clear();
	tokens.clear();
	Filter<ByActivity>(Activity::kRecord,
        Filter<ByReservationLocator>(request.getTunerReservation().getServiceLocator(),
            Filter<ByDevice>(request.getTunerReservation().getDevice(),
                              Manager::getInstance().getReservationTokens(tokens, Manager::getInstance().getTunerIds(tunerIds)))));

	if (tokens.size() != 0) {
		Log() << "Found Existing <Record> reservation for SrcID "
				  << request.getTunerReservation().getServiceLocator()
				  << " owned by requesting device " << request.getTunerReservation().getDevice() << "\r\n";

		SafeAssert(tokens.size() == 1);

		response.getStatus() += "Found Existing <Record> reservation : ";
        response.getStatus() += (*tokens.begin()).c_str(); 
		response.getStatus() += " owned by requesting device \r\n";

		Log() << "Found Existing <Record> reservation : " << (*tokens.begin()) << " owned by requesting device \r\n";

		if (!request.getTunerReservation().getReservationToken().empty()) {
			tokenToRenew = (*tokens.begin());
			Assert(tokenToRenew.compare(request.getTunerReservation().getReservationToken()) == 0);
		}
		else {
			/* Recorder is not trying to renew, even though the source locator is same */
			tokenToRenew = "";
		}

		Log() << "Token to renew is " << tokenToRenew << std::endl;
		/* Update the token */
		tunerIds.clear();
		tunerIds.insert(tunerIds.begin(), Manager::getInstance().getParent(tokenToRenew));
		//Exception will be thrown if release fails.
		if (!tokenToRenew.empty()) {
			Manager::getInstance().releaseReservation(tokenToRenew, false/*transition release do not notify client*/);
			Manager::getInstance().addReservation(request.getTunerReservation(), (*(tunerIds.begin())));
			Manager::getInstance().setReservationAttributes(request.getTunerReservation().getReservationToken(), ReservationAttributes(clientId));
			response.setTunerReservation(request.getTunerReservation());
		}
		else {
			//Simply add a 2nd reservation to the tuner.
		}
	}
	else {
		Log() << "No Existing <Record> reservation owned by requesting device\r\n";
		response.getStatus() += "No Existing <Record> reservation owned by requesting device \r\n";
		tokenToRenew = "";
		response.getStatus()  = ResponseStatus::kInvalidToken;
	}

}

void reserveRecord(const ReserveTuner &request, ReserveTunerResponse &response, int32_t clientId)
{
	response.getStatus() += "Start reservation process for <Recording> \r\n";

	std::string tunerToAddTo   = "";
	const uint64_t endTime = request.getTunerReservation().getStartTime();
	bool hasStarted = (request.getTunerReservation().getStartTime() < GetCurrentEpoch());

	/*
	 * Check R Token can be granted without triggering conflicts. Searched in the following order
	 *
	 * A tuner without future tokens is considered to have a future token whose end time is NOW().
	 *
	 * If there is a H tuner whose R end time is before the new start time and whose srcId is same.
	 * If there is a L tuner with no <future> R who has the longest active time and whose srcId is same.
	 * If there is a R tuner whose end time is before the new start time.
	 * If there is a F tuner.
	 *
	 */

	if(!request.getTunerReservation().getReservationToken().empty()) {
		Log() << "Renew existing token \r\n";
		response.getStatus() += "Renew existing token \r\n";
		renewRecord(request, response, clientId);
		return;
	}

	/* At any given time, there can only be upto N active recording and 2 future recording per source Id, per Tuner */
	{
		Tuner::IdList tunerIds;
		tunerIds.clear();
		int count = 0;
		Manager::getInstance().getTunerIds(tunerIds);
        Tuner::IdList::const_iterator tidIT = tunerIds.begin();
        while (tidIT != tunerIds.end()) {
        	try {
        		Manager::getInstance().getTuner(*tidIT).getReservation(
        				Activity::kRecord,
        				TunerReservation::ACTIVE,
        				request.getTunerReservation().getServiceLocator());
        		count++;
        	}
        	catch(ItemNotFoundException &e) {
        	}

            tidIT++;
        }

        if(!Policy::allowOverlapRecordings()) {
            if (hasStarted) {
            	Assert(count ==0);
            }
            else {
            	Assert(count <=1);
            }
        }
        else {
            if (count != 0) {
                Log() << "Recoridng " << request.getTunerReservation().getServiceLocator() << "already has " << count << " in progress" << std::endl;
            }
        }
    }

	{
		Tuner::IdList tunerIds;
		tunerIds.clear();
		int count = 0;
		Manager::getInstance().getTunerIds(tunerIds);
        Tuner::IdList::const_iterator tidIT = tunerIds.begin();
        while (tidIT != tunerIds.end()) {
        	try {
        		Manager::getInstance().getTuner(*tidIT).getReservation(
        				Activity::kRecord,
        				TunerReservation::IDLE,
        				request.getTunerReservation().getServiceLocator());
                Log() << "There is already an pending recording on locator " 
                      << request.getTunerReservation().getServiceLocator() 
                      << " on tuner " << tunerToAddTo 
                      << std::endl;
                /* 
                 * use this tuner if the new recording is back-2-back with the 
                 * pending recording, and if the time does not overlap
                 */
                if (Manager::getInstance().getTuner(*tidIT).getEndTime(Activity::kRecord) <= request.getTunerReservation().getStartTime()) {
                    tunerToAddTo = *tidIT; 
                }
        		count++;
        	}
        	catch(ItemNotFoundException &e) {
        	}

            tidIT++;
        }

    	if (!Policy::allowOverlapRecordings()) Assert(count <= 1);
    }

	if (tunerToAddTo.empty())
	{
		Tuner::IdList tunerIds;
		TunerReservation::TokenList tokens;

		Log() << "First look for H tuner whose R end time is before the new start time and whose srcId is same \r\n";
		response.getStatus() += "First look for H tuner whose R end time is before the new start time and whose srcId is same \r\n";

		Filter<ByTunerLocator>(request.getTunerReservation().getServiceLocator(),
			Filter<ByTunerEndBefore>(endTime,
				  Filter<ByTunerState>(TunerState::kHybrid, Manager::getInstance().getTunerIds(tunerIds))));

		if (tunerIds.size() != 0) {
			Log() << "Found <Hybrid> tuner of same locator for the recording \r\n";
		    response.getStatus() += "Found <Hybrid> tuner of same locator for the recording\r\n";
		    Assert(tunerIds.size() == 1);
			tunerToAddTo = (*tunerIds.begin());
		}
	}

	if (tunerToAddTo.empty())
	{
		Tuner::IdList tunerIds;
		tunerIds.clear();

		Log() << "Then look for L tuner with no <future> R who has the longest active time and whose srcId is same \r\n";
		response.getStatus() += "Then look for L tuner with no <future> R who has the longest active time and whose srcId is same \r\n";

		Filter<ByTunerLocator>(request.getTunerReservation().getServiceLocator(),
			  Filter<ByTunerState>(TunerState::kLive, Manager::getInstance().getTunerIds(tunerIds)));

        Tuner::IdList::const_iterator tidIT = tunerIds.begin();
        while (tidIT != tunerIds.end()) {
			/* There can be multiple LIVE tuners streaming the recording channel. Pick the oldest */
			TunerReservation::TokenList tokens;
			tokens.clear();

			Manager::getInstance().getTuner(*tidIT).getReservationTokens(tokens);
			if (tokens.size() == 1) {
				if (Manager::getInstance().
						getTuner(*tidIT).
						    getServiceLocator(false).
						        compare(request.getTunerReservation().getServiceLocator()) == 0) {
					Log() << "found L tuner with no <future> R who has the longest active time and whose srcId is same \r\n";
					response.getStatus() += "found L tuner with no <future> R who has the longest active time and whose srcId is same \r\n";
					tunerToAddTo = (*tidIT);
					break;
				}
			}
            tidIT++;
        }
	}

	if (tunerToAddTo.empty())
	{
		Tuner::IdList tunerIds;
		tunerIds.clear();

		Log() << "Then look for R tuner of same sourceId whose end time is before the new start time \r\n";
		response.getStatus() += "Then look for R tuner of same sourceId whose end time is before the new start time \r\n";

		/* This R tuner must not already have a future R token */
		Filter<ByTunerLocator>(request.getTunerReservation().getServiceLocator(),
		    Filter<ByTunerEndBefore>(endTime,
				  Filter<ByTunerState>(TunerState::kRecord, Manager::getInstance().getTunerIds(tunerIds))));

		/* There can only be at most 1 tuner actively recording a given sourceId */
		if (!Policy::allowOverlapRecordings()) Assert(((int)tunerIds.size()) <= 1);
		if (tunerIds.size() != 0) {
			tunerToAddTo = (*tunerIds.begin());
		}
	}

	if (tunerToAddTo.empty())
	{
		Tuner::IdList tunerIds;
		tunerIds.clear();

		Log() << "Then look for R tuner of different sourceId whose end time is before the new start time \r\n";
		response.getStatus() += "Then look for R tuner of different sourceId whose end time is before the new start time \r\n";

		/* This R tuner must not already have a future R token */
        Filter<ByTunerEndBefore>(endTime,
              Filter<ByTunerState>(TunerState::kRecord, Manager::getInstance().getTunerIds(tunerIds)));

		if (tunerIds.size() != 0) {
			tunerToAddTo = (*tunerIds.begin());
		}
	}

	if (tunerToAddTo.empty())
	{
		Log() << "Then look for F tuner with no future R  \r\n";
		response.getStatus() += "Then look for F tuner with no future R   \r\n";

		Tuner::IdList tunerIds;
		tunerIds.clear();

		Filter<ByTunerState>(TunerState::kFree, Manager::getInstance().getTunerIds(tunerIds));

		if (tunerIds.size() != 0) {
			Tuner::IdList::const_iterator tidIT = tunerIds.begin();
			while (tidIT != tunerIds.end()) {
				/* A security check: F tuner should not have active L or R reservation*/
				try {
					Manager::getInstance().getTuner(*tidIT).getReservation(Activity::kLive, TunerReservation::ACTIVE);
					Assert(0);
				}
				catch(ItemNotFoundException &e) {
				}

				try {
					Manager::getInstance().getTuner(*tidIT).getReservation(Activity::kRecord, TunerReservation::ACTIVE);
					Assert(0);
				}
				catch(ItemNotFoundException &e) {
				}

				/* A security check: F tuner can have an  IDLE token whose expiration time is in the past*/
				try {
					Manager::getInstance().getTuner(*tidIT).getReservation(Activity::kLive, TunerReservation::IDLE);
					Log() << "Tuner " << *tidIT << " has IDLE tokens " << std::endl;
				}
				catch(ItemNotFoundException &e) {
				}


				TunerReservation::TokenList tokens;
				tokens.clear();
				Manager::getInstance().getTuner(*tidIT).getReservationTokens(tokens);
				if (tokens.size() == 0) {
					tunerToAddTo = *tidIT;
					break;
				}
				else if (tokens.size() == 1) {
                    /* First filter out the tuner if the only token is for futhre R */
                    if (Manager::getInstance().getReservation(*tokens.begin()).getActivity() == Activity::kLive) {
                        /* Accept if the token is IDLE and statTime has passed */
                        TunerReservation reservation = Manager::getInstance().getTuner(*tidIT).getReservation(Activity::kLive, TunerReservation::IDLE);
                        if (reservation.getServiceLocator().compare(request.getTunerReservation().getServiceLocator()) == 0) {
                            if (reservation.getStartTime() <= GetCurrentEpoch()) {
                                Log() << "Tuner " << *tidIT << " is accepted " << std::endl;
                                tunerToAddTo = *tidIT;
                                break;
                            }
                        }
                    }
                    else {
                    }
				}

				tidIT++;
			}
		}
	}

	/*
	 * Check tuner allocation and make sure that the host device has a dedicated tuner for Live:
	 *  The granted tuner is either in one of the following states
	 *  Live state:  The R will make it into H state.
	 *  Free state:  The R will make it into R state. If the F tuner originally belong to local device, find a replacement.
	 */
	if (!tunerToAddTo.empty()) {
		if ((Manager::getInstance().getTuner(tunerToAddTo).getState().getState() == TunerState::kFree) &&
			(tunerToAddTo.compare(Manager::getInstance().getLocalTuner()) == 0)) {
			/* Look for another free tuner and to give it to local device */
			Tuner::IdList tunerIds;
			tunerIds.clear();
			Filter<ByTunerState>(TunerState::kFree, Manager::getInstance().getTunerIds(tunerIds));
			if (tunerIds.size() != 0) {
				tunerIds.remove(tunerToAddTo);
				/* Pick the one that is no the granted tuner */
				if (tunerIds.size() != 0) {
					Log() << "[reserveLocal] requester is recorder, moving reservedDeviceId "
						  << GetDeviceId() << " from "
						  << Manager::getInstance().getLocalTuner()
						  << " to " << *tunerIds.begin() << std::endl;
					Manager::getInstance().setLocalTuner(*tunerIds.begin());
				}
				else {
					/* no more F tuner, reservedDeviceId stays with current R tuner */
				}
			}
		}
	}

	if (!tunerToAddTo.empty())
	{
		Log() << "Granting New Reservation to Record on tuner " << tunerToAddTo << std::endl;
		//@TODO: Validate the requested reservation. For now we simply make a copy and use it.
		TunerReservation recordReservation(request.getTunerReservation());
		recordReservation.setReservationToken(GenerateUUID());
		/* Use the tuner whose last endtime is closet to the new startTime */
		Manager::getInstance().addReservation(recordReservation, ((tunerToAddTo)));
		Manager::getInstance().setReservationAttributes(recordReservation.getReservationToken(), ReservationAttributes(clientId));
		response.setTunerReservation(recordReservation);
	}
    else {
    	/* Looking for a conflict */
		  //@TODO How to handle over booking?
		response.getStatus()  += "Error: Reservation Failed with conflicts\r\n";
		response.getStatus()  = ResponseStatus::kInsufficientResource;

		/* Only look for conflict resolution when recording is not hot */
		if (Policy::enableConflictsForHotRecording() || request.getTunerReservation().getActivity().getDetail("hot").compare("true") != 0) {
		    response.getStatus()  += "Error: This is a recording, see if we can find resolution\r\n";
		    FindConflictsForRequestedRecord(request, response.getConflicts());
	    }
		else {
		    response.getStatus()  += "Error: This is a hot recording, let request fail\r\n";
		}

		ReserveTunerResponse::ConflictCT & conflicts = response.getConflicts();
		if (conflicts.size() == 0 ) {
		     /* Over booked */
		     response.getStatus()  += "Error: All tuners are recording\r\n";
		}
		else if (conflicts.size() == 1) {
		    Log() << "Found one L tuner to start Conflict-resolution process\r\n";
		    response.getStatus()  += "Found one L tuner to start Conflict-resolution process\r\n";
		}
		else if (conflicts.size() > 1) {
			Assert(0);
		}
	}
}


LOCAL_END_NAMESPACE

extern Server *serverInstance;

TRM_BEGIN_NAMESPACE


void Execute(Executor<ReserveTuner> &exec)
{
	const ReserveTuner &request = exec.messageIn;
	ReserveTunerResponse response(request.getUUID());

	Log() << "[EXEC]Reserving"
			  << " For Activity " << (const char *)request.getTunerReservation().getActivity().getActivity()
			  << " From Device " << request.getDevice() << std::endl;


	try {
		if (request.getTunerReservation().getActivity() == Activity::kLive) {
			
			try
			{
				reserveLive(request, response, exec.getClientId());
				exec.messageOut = response;
			}
			catch(InvalidStateException &e) {
				exec.messageOut.getStatus()  += (response.getStatus().getDetails() + " -->Error: Token Is In a bad state").c_str();
				exec.messageOut.getStatus()  = ResponseStatus::kInvalidState;
			}
			catch(ItemNotFoundException &e) {
				exec.messageOut.getStatus()  += (response.getStatus().getDetails() + " -->Error: Token Is Not Found").c_str();
				exec.messageOut.getStatus()  = ResponseStatus::kInvalidToken;
			}
			catch(IllegalArgumentException &e) {
				exec.messageOut.getStatus()  += (response.getStatus().getDetails() + " -->Error: Request Is malformed").c_str();
				exec.messageOut.getStatus()  = ResponseStatus::kMalFormedRequest;
			}

			{
				Log() << "Sending the message:RL: " << std::endl;
				std::vector<uint8_t> out;
				SerializeMessage(exec.messageOut, exec.getClientId(), out);
				::serverInstance->getConnection(exec.getClientId()).sendAsync(out);
			}
		}
		else if(request.getTunerReservation().getActivity() == Activity::kRecord) {
			try {
				reserveRecord(request, response, exec.getClientId());
				exec.messageOut = response;

				if (exec.messageOut.getConflicts().size() == 1) {

					/* Send Conflicts to L's Connection */
					std::string temporaryToken = GenerateUUID();
					const int CONFLICT_RESOLUTION_TIMEOUT_MS = (53500);

					//PendingRequest(exec.getClientId(), temporaryToken, request, exec.messageOut.getConflicts())
					TunerReservation & conflict = *(exec.messageOut.getConflicts().begin());
                    /* conflict to R is L token,therefore, the conflict's clientId is L token's requester, i.e. XRE */
					ReservationAttributes &attrs = Manager::getInstance().getReservationAttributes(conflict.getReservationToken());
					/* If the connection is still alive, send connection to it */
	#if 0
					/* Send conflict content in this order (R, L) */
					NotifyTunerReservationConflicts notification(GenerateUUID(), request.getTunerReservation(), exec.messageOut.getConflicts());
	#else
					/* Send conflict content in this order (L, R) */
					NotifyTunerReservationConflicts notification(GenerateUUID(), conflict, request.getTunerReservation());
	#endif
					notification.getConflicts().begin()->setReservationToken(temporaryToken);
					std::string parentId =  Manager::getInstance().getParent(notification.getTunerReservation().getReservationToken());
					Manager::getInstance().addPendingRequest(new PendingReserveTunerConflictProcessor(temporaryToken, exec.getClientId(), notification, request, parentId), CONFLICT_RESOLUTION_TIMEOUT_MS);

					{
						std::vector<uint8_t> out;
						SerializeMessage(notification, attrs.clientId, out);
						::serverInstance->getConnection(attrs.clientId).sendAsync(out);
					}
				}
				else {
					Log() << "to Send Record Response, with no conflict resolution\r\n";
					std::vector<uint8_t> out;
					SerializeMessage(exec.messageOut, exec.getClientId(), out);
					::serverInstance->getConnection(exec.getClientId()).sendAsync(out);
				}
			}
			catch(AssertionFailureException &e) {
				/* Respond with general error */
				Log() << "to Send failed Record Response, with no conflict resolution included\r\n";
				exec.messageOut.getStatus()  = ResponseStatus::kGeneralError;
				std::vector<uint8_t> out;
				SerializeMessage(exec.messageOut, exec.getClientId(), out);
				::serverInstance->getConnection(exec.getClientId()).sendAsync(out);
			}

		}
	}
	catch(ConnectionNotFoundException &e) {
		Log() << "Connection for message has reset/lost, discarding message\r\n";
	}
	catch(...) {
		Assert(0);
	}

}

void Execute(Executor<ReleaseTunerReservation> &exec)
{
	const ReleaseTunerReservation &request = exec.messageIn;
	ReleaseTunerReservationResponse response(request.getUUID(), request.getReservationToken());;

	Log() << "[EXEC]Releasing"
			  << " Token " << request.getReservationToken()
			  << " From device" << request.getDevice() << std::endl;

	try {
		Manager::getInstance().releaseReservation(request.getReservationToken());
		response.setReleased(true);
		response.getStatus() = ResponseStatus::kOk;
		exec.messageOut = response;

	}
	catch(ItemNotFoundException &e) {
		exec.messageOut.setReleased(false);
		exec.messageOut.getStatus()  += "Error: Token Is Not Found";
		exec.messageOut.getStatus()  = ResponseStatus::kInvalidToken;
	}

	std::vector<uint8_t> out;
	SerializeMessage(exec.messageOut, exec.getClientId(), out);
	::serverInstance->getConnection(exec.getClientId()).sendAsync(out);
}

void Execute(Executor<ValidateTunerReservation> &exec)
{
	const ValidateTunerReservation &request = exec.messageIn;
	ValidateTunerReservationResponse response(request.getUUID(), request.getReservationToken());;

	Log() << "[EXEC]Validating"
			  << " Token " << request.getReservationToken()
			  << " From device" << request.getDevice() << std::endl;
	try {
		TunerReservation &reservation = Manager::getInstance().getReservation(request.getReservationToken());
		ReservationAttributes & attr = Manager::getInstance().getReservationAttributes(reservation.getReservationToken());

		if (attr.clientId == exec.getClientId()) {
			exec.messageOut.setValid(true);
			response.getStatus() = ResponseStatus::kOk;
			exec.messageOut = response;
		}
		else {
			Log() << "Throw ItemNotFoundException from Execute()" << std::endl;
			throw ItemNotFoundException();
		}
	}
	catch(...) {
		exec.messageOut.setValid(false);
		exec.messageOut.getStatus() += "Error: Token Is Not Found";
		exec.messageOut.getStatus()  = ResponseStatus::kOk;
	}

	std::vector<uint8_t> out;
	SerializeMessage(exec.messageOut, exec.getClientId(), out);
	::serverInstance->getConnection(exec.getClientId()).sendAsync(out);
}

void Execute(Executor<CancelRecording> &exec)
{
	/* Forward Request to Recorder */
	const CancelRecording &request = exec.messageIn;
	CancelRecordingResponse response(request.getUUID(), request.getReservationToken());;

	try {
		/*
		 * Check if the recording being cancelled has any pending reserveTuner() requests.
		 */
		if (Manager::getInstance().isPendingConflict(request.getReservationToken())) {
            /* The recording has not yet started. Reject Recorder's reservation request */
			PendingReserveTunerConflictProcessor &pendingRequest = *static_cast<PendingReserveTunerConflictProcessor *>(&Manager::getInstance().getPendingRequest(request.getReservationToken()));

			Assert(pendingRequest.getUUID() == request.getReservationToken());
			Log() << "[EXEC]CancelRecording "
					  << "Temporary Recording Token " << pendingRequest.getUUID()
                      << " clientId " << exec.getClientId()
                      << " clientId2 " << pendingRequest.clientId
					  << std::endl;

			/* Respond to cancel Temporary as "User Cancel */
			{
				ReserveTunerResponse response(pendingRequest.request.getUUID());
				response.getStatus() += "User canceled recording as Conflict Resolution";
				response.getStatus() = ResponseStatus::kUserCancellation;

				SafeAssert(pendingRequest.clientId != exec.getClientId());

				std::vector<uint8_t> out;
				/* First notify requester of cancel success */
				SerializeMessage(response, pendingRequest.clientId, out);
				::serverInstance->getConnection(pendingRequest.clientId).sendAsync(out);
			}

			Manager::getInstance().removePendingRequest(pendingRequest.getUUID());
			delete &pendingRequest;
			/*Then notify xre of cancellation */
			{
				CancelRecordingResponse response(request.getUUID(), request.getReservationToken());;
				response.getStatus() += "Cancellation Success \r\n";
				response.getStatus() = ResponseStatus::kOk;
				response.setCanceled(true);

				exec.messageOut = response;
				std::vector<uint8_t> out;
				SerializeMessage(exec.messageOut, exec.getClientId(), out);
				::serverInstance->getConnection(exec.getClientId()).sendAsync(out);
			}
		}
		else {
            /* The recording being cancelled does not have any pending requests. Forward to Recorder*/
			if (!(Manager::getInstance().getReservation(request.getReservationToken()).getActivity() == Activity::kRecord)) {
				throw IllegalArgumentException();
			}

			Log() << "[EXEC]CancelRecording "
                      << "From client " << exec.getClientId() << " " 
					  << "Actual Recording Token " << Manager::getInstance().getReservation(request.getReservationToken()).getReservationToken()
					  <<  std::endl;

			const int CANCEL_RECORDING_TIMEOUT_MS = (15000);

			/* Forward to cancel Actual to Recorder */
            Manager::getInstance().addPendingRequest(new PendingCancelRecordingProcessor(exec.getClientId(), request), CANCEL_RECORDING_TIMEOUT_MS);

			std::vector<uint8_t> out;
			SerializeMessage(exec.messageIn, exec.getClientId(), out);
			::serverInstance->getConnection(Connection::kRecorderClientId).sendAsync(out);
		}

	}
	catch(ConnectionNotFoundException &e) {
		/* There is no recorder.  Send Response Error. */
		exec.messageOut.getStatus() += "Recorder is not connected \r\n";
		exec.messageOut.getStatus() = ResponseStatus::kGeneralError;
		exec.messageOut.setCanceled(false);

		std::vector<uint8_t> out;
		SerializeMessage(exec.messageOut, exec.getClientId(), out);
		::serverInstance->getConnection(exec.getClientId()).sendAsync(out);
	}
	catch(IllegalArgumentException &e) {
		exec.messageOut.getStatus() += "Reservation Token is not valid \r\n";
		exec.messageOut.getStatus() = ResponseStatus::kGeneralError;
		exec.messageOut.setCanceled(false);

		std::vector<uint8_t> out;
		SerializeMessage(exec.messageOut, exec.getClientId(), out);
		::serverInstance->getConnection(exec.getClientId()).sendAsync(out);

	}

	catch(ItemNotFoundException &e) {
		/* Received Invalid or NULL Token,  Send Response Error. */
		exec.messageOut.getStatus() += "Error: Token Is Not Found";
		exec.messageOut.getStatus() = ResponseStatus::kInvalidToken;
		exec.messageOut.setCanceled(false);

		std::vector<uint8_t> out;
		SerializeMessage(exec.messageOut, exec.getClientId(), out);
		::serverInstance->getConnection(exec.getClientId()).sendAsync(out);
	}

}

void Execute(Executor<CancelRecordingResponse> &exec)
{
	/* Forward Request to Recorder */
	const CancelRecordingResponse &response = exec.messageIn;

	Log() << "[EXEC]CancelRecordingResponse"
			  << " Token " << response.getReservationToken()
			  << " For Client " << exec.getClientId()
			  << std::endl;
	try {
		if (!(Manager::getInstance().getReservation(response.getReservationToken()).getActivity() == Activity::kRecord)) {
			throw IllegalArgumentException();
		}

		if (exec.getClientId() == Connection::kTrmClientId) {
			/* The CancelRecording was sent by TRM. Consume it */
			/* Assert(0) as now TRM doesn't initiate CancelRecording */
			Assert(0);
		}
		else {
            const PendingCancelRecordingProcessor &pendingRequest = *static_cast<PendingCancelRecordingProcessor *>(&Manager::getInstance().getPendingRequest(response.getUUID()));


            Log() << "[EXEC]CancelRecordingResponse "
                << "Recording Token " << pendingRequest.request.getReservationToken()
                << "for client " << pendingRequest.clientId
                << std::endl;

			Assert(pendingRequest.request.getReservationToken() == response.getReservationToken());

			/* Release the reservation and forward the response to the orginator */
			Log() << "Release the reservation and Send the response to the orginator" << std::endl;
            Manager::getInstance().removePendingRequest(response.getUUID());
			Assert(!Manager::getInstance().isPendingRequest(response.getUUID()));

			std::vector<uint8_t> out;
			SerializeMessage(exec.messageIn, pendingRequest.clientId, out);
			::serverInstance->getConnection(pendingRequest.clientId).sendAsync(out);

			delete &pendingRequest;

		}

		/* Either way, release the token. */
		Manager::getInstance().releaseReservation(response.getReservationToken());

	}
	catch(IllegalArgumentException &) {
		Log() << "[EXEC]CancelRecordingResponse contains invalid arguments..discarding " << std::endl;
	}
	catch(AssertionFailureException &) {
		/* Do nothing */
		Log() << "[EXEC]Assert Error caught " << std::endl;
	}
    catch (ItemNotFoundException &) {
		Log() << "[EXEC]Matching CancelRecording Request is not found" << std::endl;
	}
	catch(...) {
		/* Do nothing */
		Log() << "[EXEC]Unknonw Exception caught " << std::endl;
	}
}

void Execute(Executor<GetAllTunerIds> &exec)
{
	const GetAllTunerIds &request = exec.messageIn;
	GetAllTunerIdsResponse response(request.getUUID());;

	try {
		Tuner::IdList tunerIds;
		response.addTunerId(Manager::getInstance().getTunerIds(tunerIds));
		response.getStatus() = ResponseStatus::kOk;
		response.getStatus() += "GetAllTunerIds ok";
		exec.messageOut = response;
	}
	catch(...) {
		exec.messageOut.getStatus() += "Error: Cannot get IDs";
		exec.messageOut.getStatus() = ResponseStatus::kGeneralError;
	}

	std::vector<uint8_t> out;
	SerializeMessage(exec.messageOut, exec.getClientId(), out);
	::serverInstance->getConnection(exec.getClientId()).sendAsync(out);
}

void Execute(Executor<GetAllTunerStates> &exec)
{
	const GetAllTunerStates &request = exec.messageIn;
	GetAllTunerStatesResponse response(request.getUUID());;
	try {
		Tuner::IdList tunerIds;
		Manager::getInstance().getTunerIds(tunerIds);
		Tuner::IdList::const_iterator it;
		for (it=tunerIds.begin(); it != tunerIds.end(); it++) {
			response.addTunerState(*it, (const char *)(Manager::getInstance().getTuner(*it).getState().getState()));
		}

		/* 2.0 or above only */
		for (it=tunerIds.begin(); it != tunerIds.end(); it++) {
			Log()  << "Adding Detailed Tuner State "
					   << (const char *)(Manager::getInstance().getTuner(*it).getState().getState())
					   << " with locator " << (Manager::getInstance().getTuner(*it).getServiceLocator())
					   << " for Tuner " << *it
			           << std::endl;

			std::string reservedDeviceId = "";
			if (Manager::getInstance().getLocalTuner().compare(*it) == 0) {
				reservedDeviceId = GetDeviceId();
			}
			response.addTunerState(*it,
					               (const char *)(Manager::getInstance().getTuner(*it).getState().getState()),
					               (Manager::getInstance().getTuner(*it).getServiceLocator()), reservedDeviceId);

			TunerReservation::TokenList tokens;
			TunerReservation::TokenList::const_iterator itt;
			Manager::getInstance().getReservationTokens(tokens, *it);
			for (itt=tokens.begin(); itt != tokens.end(); itt++) {
				/* Get the activity and device of the token*/
				if (Manager::getInstance().getReservation(*itt).state == TunerReservation::ACTIVE) {
					Log()  << "Adding Detailed Tuner Activity "
							   << (const char *)Manager::getInstance().getReservation(*itt).getActivity().getActivity()
							   << " with Owner " << Manager::getInstance().getReservation(*itt).getDevice()
							   << std::endl;
					response.addTunerOwner(*it,
										   (const char *)Manager::getInstance().getReservation(*itt).getActivity().getActivity(),
										   Manager::getInstance().getReservation(*itt).getDevice());
				}
				else {
					Log()  << "Ignoring Inactive token " << *itt << std::endl;
				}
			}
		}

		response.getStatus() = ResponseStatus::kOk;
		exec.messageOut = response;
	}
	catch(...) {
		exec.messageOut.getStatus() += "Error: Cannot get States";
		exec.messageOut.getStatus() = ResponseStatus::kGeneralError;
	}

	std::vector<uint8_t> out;
	SerializeMessage(exec.messageOut, exec.getClientId(), out);
	::serverInstance->getConnection(exec.getClientId()).sendAsync(out);
}

void Execute(Executor<GetAllReservations> &exec)
{
	const GetAllReservations &request = exec.messageIn;
	GetAllReservationsResponse response(request.getUUID());;

	try {
		Tuner::IdList tunerIds;
		Manager::getInstance().getTunerIds(tunerIds);
		Tuner::IdList::const_iterator it;
		for (it=tunerIds.begin(); it != tunerIds.end(); it++) {
			TunerReservation::TokenList tokens;
			Manager::getInstance().getReservationTokens(tokens, *it);
			//Apply Filter:
			if (!request.getFilter("device").empty()) {
				Filter<ByDevice>(request.getFilter("device"), tokens);
			}

			if (!request.getFilter("activity").empty()) {
				Filter<ByActivity>(Activity(request.getFilter("activity").c_str()).getActivity(), tokens);
			}

			if (!request.getFilter("state").empty()) {
				Filter<ByTunerState>(TunerState(request.getFilter("state").c_str()).getState(), tokens);
			}

			TunerReservation::TokenList::const_iterator itt;
			for (itt = tokens.begin(); itt != tokens.end(); itt++) {
				response.addTunerReservation(*it, Manager::getInstance().getReservation(*itt));
			}
		}
		response.getStatus() = ResponseStatus::kOk;
		exec.messageOut = response;
	}
	catch(...) {
		exec.messageOut.getStatus() += "Error: Cannot get Reservations";
		exec.messageOut.getStatus() = ResponseStatus::kGeneralError;
	}

	std::vector<uint8_t> out;
	SerializeMessage(exec.messageOut, exec.getClientId(), out);
	::serverInstance->getConnection(exec.getClientId()).sendAsync(out);
}

void Execute(Executor<ReserveTuner> &exec, const std::string &parentTunerId)
{
	const ReserveTuner &request = exec.messageIn;
	ReserveTunerResponse response(request.getUUID());

	Log() << "[EXEC]Reserving and Always Granting"
			  << " For Activity " << (const char *)request.getTunerReservation().getActivity().getActivity()
			  << " From Device " << request.getDevice() << std::endl;

	try {
		if (request.getTunerReservation().getActivity() == Activity::kLive) {
			/* Not supported */
			Assert(0);
		}
		else if(request.getTunerReservation().getActivity() == Activity::kRecord) {
			/* Grant token unconditionally. Caller is responsible to make sure there will not be conflicts */
			TunerReservation recordReservation(request.getTunerReservation());
			if (request.getTunerReservation().getReservationToken().empty()) {
				recordReservation.setReservationToken(GenerateUUID());
			}
			else {
				recordReservation.setReservationToken(request.getTunerReservation().getReservationToken());
			}

			response.getStatus() += "Granting token unconditionally\r\n";
			response.getStatus() = ResponseStatus::kOk;

			/* Grant reservation to Record, and put the tuner that owsn the conflicting LIVE to HYBRID */
			response.setTunerReservation(recordReservation);

            uint64_t origStartTime = recordReservation.getStartTime();
            uint64_t origDuration  = recordReservation.getDuration();

            /* only adjust startTime to 'now' when this is the first recording on the tuner.
             * (i.e. when tuner is in F or H state). 
             */
            {
                const TunerState &tunerState = Manager::getInstance().getTuner(parentTunerId).getState();
                if (tunerState.getState() == TunerState::kFree || tunerState.getState() == TunerState::kLive) {
                    recordReservation.setStartTime(GetCurrentEpoch()-1); /* start the timer now so we can force synchybrid */
                }
            }

            if (origStartTime < GetCurrentEpoch()) {
                /* Recording already started */
                recordReservation.setStartTime(GetCurrentEpoch()); /* start the timer now so we can force synchybrid */
            }

            if (recordReservation.getStartTime() > origStartTime) {
                /* Start time is move slightly ahead */
                if (recordReservation.getStartTime() > (origStartTime + origDuration)) {
                    recordReservation.setDuration(0);
                }
                else {
                    recordReservation.setDuration(origStartTime + origDuration - recordReservation.getStartTime());
                }
            }
            else {
                /* Start time is move slightly behind */
                recordReservation.setDuration(origDuration + origStartTime - recordReservation.getStartTime());
            }
            
            SafeAssert((origStartTime + origDuration) == (recordReservation.getStartTime() + recordReservation.getDuration()));

            Log() << "Orig Record (start,dur) vs Adjusted : (" 
                << origStartTime << ", " << origDuration << "), (" 
                << recordReservation.getStartTime() << ", " << recordReservation.getDuration() << ")" 
                << std::endl;

			Manager::getInstance().addReservation(recordReservation, parentTunerId);
			Manager::getInstance().setReservationAttributes(recordReservation.getReservationToken(), ReservationAttributes(exec.getClientId()));
			exec.messageOut = response;
			try {
				Log() << "Sending the message:RL: " << std::endl;
				std::vector<uint8_t> out;
				SerializeMessage(exec.messageOut, exec.getClientId(), out);
				::serverInstance->getConnection(exec.getClientId()).sendAsync(out);
			}
            catch (...) {
                Log() << "Sending the mssage failed" << std::endl;
            }
		}
	}
	catch(...) {
		Assert(0);
	}

}

void Execute(Executor<GetVersion> &exec)
{
	const GetVersion &request = exec.messageIn;
	GetVersionResponse response(request.getUUID());;

	try {
		response.setVersion(GetSpecVersion().toString().c_str());
		response.getStatus() = ResponseStatus::kOk;
		response.getStatus() += "GetVersion ok";
		exec.messageOut = response;
	}
	catch(...) {
		exec.messageOut.getStatus() += "Error: Cannot get version";
		exec.messageOut.getStatus() = ResponseStatus::kGeneralError;
	}

	std::vector<uint8_t> out;
	SerializeMessage(exec.messageOut, exec.getClientId(), out);
	::serverInstance->getConnection(exec.getClientId()).sendAsync(out);
}
#if 1
void Execute(Executor<CancelLive> &exec)
{
	const CancelLive &request = exec.messageIn;
	CancelLiveResponse response(request.getUUID());;
	Log() << "Executing CancelLive from " << std::hex << exec.getClientId() << std::endl;
	try {
		//forward this to TuneAgent/Recorder
		std::vector<uint8_t> out;
		SerializeMessage(exec.messageIn, exec.getClientId(), out);
		::serverInstance->getConnection(Connection::kRecorderClientId).sendAsync(out);
	}
	catch(...) {
	}
#if 0
	exec.messageOut.getStatus() += "Test: Fake Cancel";
	exec.messageOut.getStatus() = ResponseStatus::kOk;
	exec.messageOut.setCanceled(true);

	std::vector<uint8_t> out;
	SerializeMessage(exec.messageOut, exec.getClientId(), out);
	::serverInstance->getConnection(exec.getClientId()).sendAsync(out);
#endif
}

void Execute(Executor<CancelLiveResponse> &exec)
{
	/* Check if the request to this response message is still pending. */
	const CancelLiveResponse &response = exec.messageIn;

	if(Manager::getInstance().isPendingRequest(response.getUUID())) {
		Log() << "emit CancelLiveResponse from " << std::hex << exec.getClientId() << std::endl;
		PendingCancelLiveProcessor &pendingRequest = *static_cast<PendingCancelLiveProcessor *>(&Manager::getInstance().getPendingRequest(response.getUUID()));
		emit Manager::getInstance().cancelLiveResponse(response, pendingRequest.reserveRequest, pendingRequest.parentId);
		Manager::getInstance().removePendingRequest(response.getUUID());
		delete &pendingRequest;
	}
}

#endif
TRM_END_NAMESPACE


/** @} */
/** @} */
