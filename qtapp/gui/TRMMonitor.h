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


#ifndef _TRM_TRM_MONITOR_H
#define _TRM_TRM_MONITOR_H

#include <QMutex>
#include <QWaitCondition>
#include <QMap>
#include <QQueue>

#include "Client.h"
#include "trm/TunerReservation.h"
#include "trm/JsonEncoder.h"
#include "trm/Header.h"
#include "trm/MessageProcessor.h"

namespace TRM {

class TRMMonitor : public Client, public MessageProcessor
{
	Q_OBJECT
public:
	TRMMonitor(const QHostAddress &address, quint16 port, uint32_t clientId);
	virtual ~TRMMonitor();
	void sendTunerReserve(const QString &device, qint64 startTime, qint64 duration, const QString &locator, const QString &activity, const QString &token);
	void sendGetAllTunerIds(void);
	void sendGetAllTunerStates(void);
	void sendGetAllReservations(void);
	void sendReleaseTunerReservation(const QString &reservationToken);
	void sendValidateTunerReservation(const QString &device, const QString &reservationToken);
	void sendCancelRecording(const QString &device, const QString &reservationToken);
	void sendCancelRecordingResponse(const QString &uuid, const QString &device, const QString &reservationToken, uint32_t outClientId);

	template<class MsgT>
	void send(const MsgT &msg, std::vector<uint8_t> &out, uint32_t outClientId = Connection::kInvalidClientId) {
		if (connection == 0) {
			return;
		}
		out.clear();
		out.resize(Header::kHeaderLength, 0);
		JsonEncoder().encode(msg, out);
		out.push_back('\0'); //append '\0' so we could print the msg as string
		std::cout << (const char *)(&out[Header::kHeaderLength]) << "\r\n";
		out.pop_back();      // Some json parser had issue with extra '\0' byte;
		//Now send response bytes out to connection.
		std::vector<uint8_t> headerBytes;
		if (outClientId == Connection::kInvalidClientId) {
			outClientId = clientId;
		}
		Header header(Request, outClientId, out.size() - Header::kHeaderLength);
		header.serialize(headerBytes);
		memcpy(&out[0], &headerBytes[0], Header::kHeaderLength);
	}

	MessageProcessor & getMessageProcessor(uint32_t inClientId) {
		this->inClientId = inClientId;
		return *this;
	}

	bool hasResponse(void) const { return false; }
	const std::vector<uint8_t> & getResponse(void) const { throw IllegalArgumentException(); }
	template<class MsgT>
	void process(const MsgT &msg) {
		msg.print();
	}
	void operator() (const ReserveTunerResponse &msg);
	void operator() (const ReleaseTunerReservationResponse &msg);
	void operator() (const ValidateTunerReservationResponse &msg);
	void operator() (const CancelRecording &msg);
	void operator() (const CancelRecordingResponse &msg);
	void operator() (const GetAllTunerIdsResponse &msg);
	void operator() (const GetAllTunerStatesResponse &msg);
	void operator() (const GetAllReservationsResponse &msg);
	void operator() (const NotifyTunerReservationConflicts &msg);
	void operator() (const NotifyTunerReservationUpdate &msg);
	void operator() (const NotifyTunerReservationRelease &msg);
	void operator() (const NotifyTunerStatesUpdate &msg);
	void operator() (const NotifyTunerPretune &msg);

	const std::list<std::string> & getTunerIds(void) const;

Q_SIGNALS:
	void tunerIdsUpdated(std::list<std::string>);
	void tunerStatesUpdated(std::map<std::string, std::string>);
	void tunerReservationsUpdated(std::map<std::string, std::list<TunerReservation> >);
	void statusMessageReceived(std::string);
	void conflictsReceived(ReserveTunerResponse::ConflictCT);

private:
    QHostAddress hostAddress;
    quint16 portNumber;
	uint32_t clientId;
	uint32_t inClientId;
	QQueue<QString> pendingRequestIds;
	std::list<std::string>  tunerIds;

};

}

#endif


/** @} */
/** @} */
