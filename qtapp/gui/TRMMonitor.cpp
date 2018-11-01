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


#include <QApplication>
#include <QtGlobal>
#include <QtCore>
#include <QObject>
#include <QTcpSocket>
#include <QHostAddress>


#include <QList>

#include "trm/JsonEncoder.h"
#include "trm/Messages.h"


#include "trm/TunerReservation.h"
#include "TRMMonitor.h"

using namespace TRM;

TRMMonitor::TRMMonitor(const QHostAddress &address, quint16 port, uint32_t clientId) : Client (address, port),
		hostAddress(address), portNumber(port), clientId(clientId)
{
}

TRMMonitor::~TRMMonitor()
{
}

const std::list<std::string> & TRMMonitor::getTunerIds(void) const
{
	return tunerIds;
}

#if 1

void TRMMonitor::sendTunerReserve(const QString &device, qint64 startTime, qint64 duration, const QString &locator, const QString &activity, const QString &token)
{

	qDebug() << ("Sending out ReserveTuner");

	std::vector<uint8_t> out;
	Activity activityObject(activity.toUtf8().constData());
	activityObject.addDetail("recordingId", QDateTime(QDate::currentDate(), QTime::currentTime()).toString().toUtf8().constData());
	activityObject.addDetail("hot", "false");

	ReserveTuner requestObject(QUuid::createUuid ().toString().toUtf8().constData(),
    						   device.toUtf8().constData(),
    						   TunerReservation(device.toUtf8().constData(),
    								   	        locator.toUtf8().constData(),
    								   	        startTime,
    								   	        duration,
    								   	        activityObject,
    								   	        token.toUtf8().constData())
    								   	     );

	send(requestObject, out);
    pendingRequestIds.enqueue(QString(requestObject.getUUID().c_str()));
    if (connection) connection->sendMessage(QByteArray((const char *)&out[0], out.size()));
}

void TRMMonitor::sendGetAllTunerIds(void)
{
    qDebug() << ("Sending out GetAllTunerIds");

    std::vector<uint8_t> out;
    GetAllTunerIds requestObject(QUuid::createUuid ().toString().toUtf8().constData());

	send(requestObject, out);
    pendingRequestIds.enqueue(QString(requestObject.getUUID().c_str()));
    if (connection) connection->sendMessage(QByteArray((const char *)&out[0], out.size()));
}

void TRMMonitor::sendGetAllTunerStates(void)
{
	qDebug() << ("Sending out GetAllTunerStates");
	std::vector<uint8_t> out;
	GetAllTunerStates requestObject(QUuid::createUuid ().toString().toUtf8().constData(), "");

	send(requestObject, out);
    pendingRequestIds.enqueue(QString(requestObject.getUUID().c_str()));
    if (connection) connection->sendMessage(QByteArray((const char *)&out[0], out.size()));
}

void TRMMonitor::sendGetAllReservations(void)
{
	qDebug() << ("Sending out GetAllReservations");
	std::vector<uint8_t> out;
	GetAllReservations requestObject(QUuid::createUuid ().toString().toUtf8().constData(), "");

	send(requestObject, out);
    pendingRequestIds.enqueue(QString(requestObject.getUUID().c_str()));
    if (connection) connection->sendMessage(QByteArray((const char *)&out[0], out.size()));
}

void TRMMonitor::sendReleaseTunerReservation(const QString &reservationToken)
{
	qDebug() << ("Sending out ReleaseTunerReservation");
	std::vector<uint8_t> out;
	ReleaseTunerReservation requestObject(QUuid::createUuid ().toString().toUtf8().constData(), "Device::TestApp", reservationToken.toUtf8().constData());

	send(requestObject, out);
    pendingRequestIds.enqueue(QString(requestObject.getUUID().c_str()));
    if (connection) connection->sendMessage(QByteArray((const char *)&out[0], out.size()));
}

void TRMMonitor::sendValidateTunerReservation(const QString &device, const QString &reservationToken)
{
	qDebug() << ("Sending out ValidateTunerReservation");
	std::vector<uint8_t> out;
	ValidateTunerReservation requestObject(QUuid::createUuid ().toString().toUtf8().constData(), device.toUtf8().constData(), reservationToken.toUtf8().constData());

	send(requestObject, out);
    pendingRequestIds.enqueue(QString(requestObject.getUUID().c_str()));
    if (connection) connection->sendMessage(QByteArray((const char *)&out[0], out.size()));

}

void TRMMonitor::sendCancelRecording(const QString &, const QString &reservationToken)
{
	qDebug() << ("Sending out ValidateTunerReservation");
	std::vector<uint8_t> out;
	CancelRecording requestObject(QUuid::createUuid ().toString().toUtf8().constData(), reservationToken.toUtf8().constData());

	send(requestObject, out);
    pendingRequestIds.enqueue(QString(requestObject.getUUID().c_str()));
    if (connection) connection->sendMessage(QByteArray((const char *)&out[0], out.size()));

}

void TRMMonitor::sendCancelRecordingResponse(const QString &uuid, const QString &, const QString &reservationToken, uint32_t outClientId)
{
	qDebug() << ("Sending out ValidateTunerReservation");
	std::vector<uint8_t> out;
	ResponseStatus responseStatus(ResponseStatus::kOk, "Recording Canceled sSuccessfully");
	CancelRecordingResponse requestObject(uuid.toUtf8().constData(), responseStatus, reservationToken.toUtf8().constData(), true);

	send(requestObject, out, outClientId);
	if (connection) connection->sendMessage(QByteArray((const char *)&out[0], out.size()));

}

void TRMMonitor::operator() (const ReserveTunerResponse &msg)
{
	process(msg);
	std::cout << "handleReserveTunerResponse =======================================" << std::endl;

    QString requestId = pendingRequestIds.dequeue();
    if(requestId != QString(msg.getUUID().c_str()))
    {
    	qDebug() << "RequestId: [" << requestId << "] vs Response Id: [" << msg.getUUID().c_str() << "]";
    	Q_ASSERT(false);
    }
    else {
    }

	sendGetAllReservations();
	sendGetAllTunerStates();
	if (msg.getStatus().getDetails() != "") {

		emit statusMessageReceived(msg.getStatus().getDetails());
	}
	const ReserveTunerResponse::ConflictCT &conflicts =  msg.getConflicts();

	if (conflicts.size() != 0) {
		std::cout << "handleReserveTunerResponse FOUND CONFLICTS=======================================" << std::endl;
		emit conflictsReceived(conflicts);
	}
	else {
		std::cout << "handleReserveTunerResponse NO CONFLICTS=======================================" << std::endl;
	}

	sendGetAllReservations();
	sendGetAllTunerStates();
}

void TRMMonitor::operator() (const ReleaseTunerReservationResponse &msg)
{
	process(msg);
	qDebug() << "handleReleaseTunerReservationResponse";
    QString requestId = pendingRequestIds.dequeue();
    if(requestId != QString(msg.getUUID().c_str()))
    {
    	qDebug() << "RequestId: [" << requestId << "] vs Response Id: [" << msg.getUUID().c_str() << "]";
    	Q_ASSERT(false);
    }
    else {
    }


	UNUSED_VARIABLE(connection);
	sendGetAllReservations();
	sendGetAllTunerStates();
}

void TRMMonitor::operator() (const ValidateTunerReservationResponse &msg)
{
	process(msg);
	qDebug() << "handleValidateTunerReservationResponse";

    QString requestId = pendingRequestIds.dequeue();
    if(requestId != QString(msg.getUUID().c_str()))
    {
    	qDebug() << "RequestId: [" << requestId << "] vs Response Id: [" << msg.getUUID().c_str() << "]";
    	Q_ASSERT(false);
    }
    else {
    	qDebug() << "Validation Result is " << msg.isValid();
    }


	UNUSED_VARIABLE(connection);
	sendGetAllReservations();
	sendGetAllTunerStates();

}

void TRMMonitor::operator() (const CancelRecording &msg)
{
	process(msg);
	qDebug() << "handleCancelRecording";
	sendCancelRecordingResponse(msg.getUUID().c_str(), msg.getDevice().c_str(), msg.getReservationToken().c_str(), inClientId);

}

void TRMMonitor::operator() (const CancelRecordingResponse &msg)
{
	process(msg);
	qDebug() << "handleCancelRecordingResponse";

    QString requestId = pendingRequestIds.dequeue();
    if(requestId != QString(msg.getUUID().c_str()))
    {
    	qDebug() << "RequestId: [" << requestId << "] vs Response Id: [" << msg.getUUID().c_str() << "]";
    	Q_ASSERT(false);
    }
    else {
    }

	UNUSED_VARIABLE(connection);
	sendGetAllReservations();
	sendGetAllTunerStates();
}

void TRMMonitor::operator() (const GetAllTunerStatesResponse &msg)
{
	process(msg);

	qDebug() << "handleGetAllTunerStatesResponse";
    QString requestId = pendingRequestIds.dequeue();

	const std::map<std::string, std::string> &states = msg.getTunerStates();
	emit tunerStatesUpdated(states);
}

void TRMMonitor::operator() (const GetAllTunerIdsResponse &msg)
{
	process(msg);
	qDebug() << "handleGetAllTunerIdsResponse";

    QString requestId = pendingRequestIds.dequeue();
    if(requestId != QString(msg.getUUID().c_str()))
    {
    	qDebug() << "RequestId: [" << requestId << "] vs Response Id: [" << msg.getUUID().c_str() << "]";
    	Q_ASSERT(false);
    }
    else {
    }

	tunerIds = msg.getTunerIds();
	/* save a copy of tuner Ids */
	std::list<std::string>::iterator it = tunerIds.begin();
	for (it = tunerIds.begin(); it != tunerIds.end(); it++) {
		qDebug() << "TunerID: " << (*it).c_str();
	}

	emit tunerIdsUpdated(tunerIds);
}

void TRMMonitor::operator() (const GetAllReservationsResponse &msg)
{
	process(msg);
	qDebug() << "handleGetAllReservationsResponse";

	    QString requestId = pendingRequestIds.dequeue();
	    if(requestId != QString(msg.getUUID().c_str()))
	    {
	    	qDebug() << "RequestId: [" << requestId << "] vs Response Id: [" << msg.getUUID().c_str() << "]";
	    	Q_ASSERT(false);
	    }
	    else {
	    }

		const std::map<std::string, std::list<TunerReservation> > & reservations = msg.getAllReservations();
		emit tunerReservationsUpdated(reservations);
}

void TRMMonitor::operator() (const NotifyTunerReservationUpdate &msg)
{
	process(msg);
	qDebug() << "NotifyTunerReservationUpdate";

	sendGetAllReservations();
	sendGetAllTunerStates();
}

void TRMMonitor::operator() (const NotifyTunerReservationRelease &msg)
{
	process(msg);
	qDebug() << "NotifyTunerReservationRelease";

	sendGetAllReservations();
	sendGetAllTunerStates();
}

void TRMMonitor::operator() (const NotifyTunerReservationConflicts &msg)
{
	process(msg);
	std::cout << "NotifyTunerReservationConflicts =======================================" << std::endl;

	const ReserveTunerResponse::ConflictCT &conflicts =  msg.getConflicts();

	if (conflicts.size() != 0) {
		std::cout << "handleReserveTunerResponse FOUND CONFLICTS=======================================" << std::endl;
		emit conflictsReceived(conflicts);
	}
	else {
		std::cout << "handleReserveTunerResponse NO CONFLICTS=======================================" << std::endl;
	}

	sendGetAllReservations();
	sendGetAllTunerStates();
}

void TRMMonitor::operator() (const NotifyTunerStatesUpdate &msg)
{
	process(msg);
	qDebug() << "NotifyTunerStatesUpdate";
}

void TRMMonitor::operator() (const NotifyTunerPretune &msg)
{
	process(msg);
	std::cout << "NotifyTunerPretune" << msg.getServiceLocator().c_str() << std::endl;
}

#endif


/** @} */
/** @} */
