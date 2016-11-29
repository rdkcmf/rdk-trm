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
#include <QDialog>
#include <QPainter>
#include <QDebug>
#include "ui_gui.h"
#include <QTimer>
#include <QTime>
#include <QMessageBox>

#include "trm/TunerState.h"

#include "TRMMonitorGUI.h"
#include "TRMMonitor.h"

using namespace TRM;

#define TUNER_REFRESH_INTERVAL 5000
#define TOTAL_TUNER_NUMBER 4
#define TOTAL_DEVICE_NUMBER 5
#define TOTAL_LOCATOR_NUMBER 7
#define RESERVATION_BAR_WIDTH 550
//#define RESERVATION_BAR_TIME_WIDTH (3600*3) //seconds
#define RBW RESERVATION_BAR_WIDTH
#define SECOND_SCALE 1 //how many actual seconds is for each 1 second in the timespin box

int RESERVATION_BAR_TIME_WIDTH = 0;

const char *deviceNames[TOTAL_DEVICE_NUMBER] = {
		"Xi3 Family Room",
		"Xi3 Living Room",
		"Xi3 Bedroom",
		"Xi3 Kitchen",
		"Xi3 Dining Room",
};

const char *locatorNames[TOTAL_LOCATOR_NUMBER] = {
		"ocap://0xCNN",
		"ocap://0xABC",
		"ocap://0xCBS",
		"ocap://0xNBC",
		"ocap://0xFOX",
		"ocap://0xHBO",
		"ocap://QVC",
};

TRMMonitorGUI::~TRMMonitorGUI(void)
{
}

TRMMonitorGUI::TRMMonitorGUI(const QHostAddress &address, quint16 port, const QString &clientId, const QString &barWidth, QObject *parent) :
		QMainWindow(), monitorUI(0), hostAddress(address), portNumber(port), clientId(clientId)
{
	bool ok;
	uint32_t connectionClientId = clientId.toUInt(&ok, 16);
	uint32_t barWidthSeconds = barWidth.toUInt(&ok, 10);

	qDebug() << "connectionClientId to " << connectionClientId;
	qDebug() << "barWidthSeconds is " << barWidthSeconds;

	RESERVATION_BAR_TIME_WIDTH = barWidthSeconds;

	if (!ok) connectionClientId = 0;
	monitor = new TRMMonitor(hostAddress, portNumber, connectionClientId);

	UNUSED_VARIABLE(parent);
	monitorUI = new Ui_TRMMonitor;
	monitorUI->setupUi(this);

	/*
	 * =======================================================
	 * Bootup Configurations.
	 */
	qDebug() << "Setting Window Title to " << clientId;
	setWindowTitle(clientId);

	{/* Set Tab Order */
		QWidget::setTabOrder(monitorUI->deviceListComboBox, monitorUI->startTimeHourSpinBox);
		QWidget::setTabOrder(monitorUI->startTimeHourSpinBox, monitorUI->startTimeMinSpinBox);
		QWidget::setTabOrder(monitorUI->startTimeMinSpinBox, monitorUI->durationHourSpinBox);
		QWidget::setTabOrder(monitorUI->durationHourSpinBox, monitorUI->durationMinSpinBox);
		QWidget::setTabOrder(monitorUI->durationMinSpinBox, monitorUI->locatorListComboBox);
		QWidget::setTabOrder(monitorUI->locatorListComboBox, monitorUI->activityLiveRadioButton);
		QWidget::setTabOrder(monitorUI->activityLiveRadioButton, monitorUI->activityRecordRadioButton);
		QWidget::setTabOrder(monitorUI->activityRecordRadioButton, monitorUI->activityEASRadioButton);
		QWidget::setTabOrder(monitorUI->activityEASRadioButton, monitorUI->reserveButton);
	}


	{/* Populate list of devices */
		for (int i = 0; i < TOTAL_DEVICE_NUMBER; i++) {
			monitorUI->deviceListComboBox->insertItem(0, deviceNames[TOTAL_DEVICE_NUMBER - i - 1]);
		}
	}

	{/* Populate list of locators */
		for (int i = 0; i < TOTAL_LOCATOR_NUMBER; i++) {
			monitorUI->locatorListComboBox->insertItem(0, locatorNames[TOTAL_LOCATOR_NUMBER - i - 1]);
		}
	}

	monitorUI->deviceListComboBox->setCurrentIndex(0);

	{/* Connect a 1-second repeat timer to currentTimeLabel */
		QTimer *timer = new QTimer(monitorUI->currentTimeLabel);
		QObject::connect(timer, SIGNAL(timeout()), this, SLOT(onTimestampTimerTimeout()));
		onTimestampTimerTimeout();
		timer->start(1000);
	}

	{/* Connect reserveButton to reserve action.*/
		QObject::connect(monitorUI->reserveButton, SIGNAL(clicked()), this, SLOT(onReserveButtonClicked()));
	}


	{/* TunerIds must be retrieved first */
		monitor->sendGetAllTunerIds();

		QTimer *timer = new QTimer(this);
		QObject::connect(timer, SIGNAL(timeout()), this, SLOT(onTunerStateUpdateTimerTimeout()));
		onTunerStateUpdateTimerTimeout();
		timer->start(TUNER_REFRESH_INTERVAL);

		QObject::connect(monitor,SIGNAL(tunerStatesUpdated(std::map<std::string, std::string>)), this, SLOT(onTunerStatesUpdated(std::map<std::string, std::string>)));
		QObject::connect(monitor,SIGNAL(tunerIdsUpdated(std::list<std::string>)), this, SLOT(onTunerIdsUpdated(std::list<std::string>)));
		QObject::connect(monitor,SIGNAL(tunerReservationsUpdated(std::map<std::string, std::list<TunerReservation> >)), this, SLOT(onTunerReservationsUpdated(std::map<std::string, std::list<TunerReservation> >)));
		QObject::connect(monitor,SIGNAL(statusMessageReceived(std::string)), this, SLOT(onStatusMessageReceived(std::string)));
		QObject::connect(monitor,SIGNAL(conflictsReceived(ReserveTunerResponse::ConflictCT)), this, SLOT(onConflictsReceived(ReserveTunerResponse::ConflictCT)));



	}
}

void TRMMonitorGUI::drawReservationBar(QLabel *label, TunerReservation *reservation)
{
	int totalWidth = label->width();
	int totalDuration = RESERVATION_BAR_TIME_WIDTH;
	qint64 durationWidth = totalWidth;
	QString labelText = "";
	qint64 now = QDateTime(QDate::currentDate(), QTime::currentTime()).toMSecsSinceEpoch();
	QColor fillColor3(255, 153, 51);
	QColor fillColor2(0xFF, 0xC1, 0xC1);
	QColor fillColor1(0xBD, 0xFC, 0xC9);
	QColor fillColor(0xE8, 0xE8, 0xE8);
	QPixmap reservationBar;
	if (reservation == 0) {
		reservationBar = QPixmap(totalWidth, 30);
		reservationBar.fill(fillColor);
	}
	else {
		reservationBar = QPixmap(*label->pixmap());
	}

	if (reservation) {
		std::cout << "DRAW DRAW " << reservation->getServiceLocator() << std::endl;
	}

	qint64 currentEpoch = QDateTime(QDate::currentDate(), QTime::currentTime()).toMSecsSinceEpoch();

	label->clear();
	if (reservation != 0) {
		/* Only show token string of the in-progress reservation */
		if (reservation->getStartTime() < now) {
			qDebug() << "setting text" << QString(reservation->getReservationToken().c_str());

			label->setText(QString(reservation->getReservationToken().c_str()));
			label->setToolTip(QString(reservation->getReservationToken().c_str()) + " "  + QString(reservation->getServiceLocator().c_str()));
			label->setVisible(true);
			if (reservation->getActivity() == Activity::kLive) {
				labelText = "   [" + QString(reservation->getDevice().c_str()) + "]  -  [" + QString(reservation->getServiceLocator().c_str()) + "]";
				fillColor = fillColor1;
			}
			else {
				labelText = "   [" + QString(reservation->getServiceLocator().c_str()) + "]";
				fillColor = fillColor2;
			}
		}
		else {
			fillColor = fillColor3;
		}

#if 0
		//draw fixed duration
		durationWidth = (reservation->getDuration() * totalWidth) / (totalDuration * 1000);
#else
		/* calculate what is left in duration */
		qint64 duration = 0;
		if (reservation->getStartTime() < now) {
			duration = reservation->getStartTime() + reservation->getDuration() - now;
		}
		else {
			duration = reservation->getDuration();
		}

		if (duration < 0) {
			duration = 0;
		}

		durationWidth = (duration * label->width()) / (totalDuration * 1000);
#endif

		qDebug() << "Duration Width is " << durationWidth;
	}

	int state = TunerState::kHybrid;
	QPainter painter;
	painter.begin(&reservationBar);
	int startX = 0;
	if (reservation != 0){
		if (reservation->getStartTime() < now) {

			startX = 0;
		}
		else {
			startX = (((reservation->getStartTime() - now) * label->width()) / (totalDuration * 1000)) + 2;

		}
	}

	painter.fillRect(startX, 0, durationWidth, 30, fillColor);
	painter.drawText(QPoint(0, 20), labelText);
	painter.end();
	label->setPixmap(reservationBar);
}

void TRMMonitorGUI::setTunerStateIndicator(const QString &tunerId, const TunerState& state)
{
	qDebug() << "setTunerStateIndicator " << tunerId << " for state " << (const char *)(state.getState());;

	tunerStateIndicatorLabels[tunerId]->clear();
	QPixmap labelSquare = QPixmap(60, 60);
	QColor fillColor  = Qt::darkGray;
	QColor fillColor2 = Qt::darkGray;

	if (state == TunerState::kFree) {
		fillColor = Qt::darkGray;
		fillColor2 = Qt::darkGray;
	} else if (state == TunerState::kLive) {
		fillColor = Qt::darkGreen;
		fillColor2 = Qt::darkGreen;
	} else if (state == TunerState::kRecord) {
		fillColor = Qt::darkRed;
		fillColor2 = Qt::darkRed;
	} else if (state == TunerState::kHybrid) {
		fillColor = Qt::darkGreen;
		fillColor2 = Qt::darkRed;
	} else if (state == TunerState::kEAS) {
		fillColor = Qt::red;
		fillColor2 = Qt::red;
	}

	labelSquare.fill(fillColor);
	QPainter painter;
	painter.begin(&labelSquare);
	painter.fillRect(0, 0, 30, 60, fillColor);
	painter.fillRect(30, 0, 60, 60, fillColor2);
	painter.end();
	tunerStateIndicatorLabels[tunerId]->setPixmap(labelSquare);
}

void TRMMonitorGUI::onReserveButtonClicked(void)
{
	/* Reserve */
#if 0
	//startTimeHourSpinBox and MinSpinBox indicates the start HH::MM
	QTime startQTime(monitorUI->startTimeHourSpinBox->value(), monitorUI->startTimeMinSpinBox->value());

	if (monitorUI->startTimeHourSpinBox->value() == 0 &&  monitorUI->startTimeMinSpinBox->value() == 0) {
		startQTime = QTime::currentTime();
	}
#elif 0
	//startTime = now();
	qint64 startTime = QDateTime(QDate::currentDate(), startQTime).toMSecsSinceEpoch();
#else
	//startTimeHourSpinBox and MinSpinBox indicates the startTime = now() + MM::SS
	qint64 startTime = QDateTime(QDate::currentDate(), QTime::currentTime()).toMSecsSinceEpoch() ;//+ monitorUI->startTimeMinSpinBox->value();
	startTime += ((monitorUI->startTimeHourSpinBox->value() * 60 + monitorUI->startTimeMinSpinBox->value() * 10) * SECOND_SCALE * 1000);
#endif

#if 0
#else
	//durationHourSpinBox and MinSpinBox indicates the duration = MM::SS
	qint64 duration = (monitorUI->durationHourSpinBox->value() * 60 + monitorUI->durationMinSpinBox->value()) * SECOND_SCALE * 1000;
#endif

	QString device = deviceNames[monitorUI->deviceListComboBox->currentIndex()];
	QString locator = locatorNames[monitorUI->locatorListComboBox->currentIndex()];
	QString activity="";
	if (monitorUI->activityLiveRadioButton->isChecked()) {
		activity="Live";
	}
	else if (monitorUI->activityRecordRadioButton->isChecked()) {
		activity="Record";
	}
	else if (monitorUI->activityEASRadioButton->isChecked()) {
		activity="EAS";
	}
	else {
		//@TODO Error message box.
		return;
	}
	qDebug() << "onReserveButtonClicked";

	monitor->sendTunerReserve(device, startTime, duration, locator, activity, "");
}

void TRMMonitorGUI::onTimestampTimerTimeout(void)
{
	monitorUI->currentTimeLabel->setText(QTime::currentTime().toString());
}

void TRMMonitorGUI::onTunerStateUpdateTimerTimeout(void)
{
	qDebug() << "Updating Tuner State";
	monitor->sendGetAllTunerStates();
	monitor->sendGetAllReservations();
}

void TRMMonitorGUI::onTunerIdsUpdated(std::list<std::string> tunerIds)
{
	qDebug() << "onTunerIdsUpdated";
	{/* Gray out all tuners */
		std::list<std::string>::iterator it = tunerIds.begin();
		Q_ASSERT(tunerIds.size() == TOTAL_TUNER_NUMBER);
		{
			tunerStateIndicatorLabels.insert(QString((*it).c_str()), monitorUI->tuner1Label); it++;
			tunerStateIndicatorLabels.insert(QString((*it).c_str()), monitorUI->tuner2Label); it++;
			tunerStateIndicatorLabels.insert(QString((*it).c_str()), monitorUI->tuner3Label); it++;
			tunerStateIndicatorLabels.insert(QString((*it).c_str()), monitorUI->tuner4Label); it++;
		}

		{ /* Index ReservationBar with "TunerID" + "Activity" */
			it = tunerIds.begin();
			QMap<QString, QLabel *> tunerReservationBarLabel;
			tunerReservationBarLabel.clear();
			tunerReservationBarLabel.insert(QString((const char *)Activity::kLive), monitorUI->tuner1LiveReservationIndicationBar);
			tunerReservationBarLabel.insert(QString((const char *)Activity::kRecord), monitorUI->tuner1RecordReservationIndicationBar);
			tunerReservationBarLabels.insert(QString((*it).c_str()),tunerReservationBarLabel); it++;

			tunerReservationBarLabel.clear();
			tunerReservationBarLabel.insert(QString((const char *)Activity::kLive), monitorUI->tuner2LiveReservationIndicationBar);
			tunerReservationBarLabel.insert(QString((const char *)Activity::kRecord), monitorUI->tuner2RecordReservationIndicationBar);
			tunerReservationBarLabels.insert(QString((*it).c_str()),tunerReservationBarLabel); it++;

			tunerReservationBarLabel.clear();
			tunerReservationBarLabel.insert(QString((const char *)Activity::kLive), monitorUI->tuner3LiveReservationIndicationBar);
			tunerReservationBarLabel.insert(QString((const char *)Activity::kRecord), monitorUI->tuner3RecordReservationIndicationBar);
			tunerReservationBarLabels.insert(QString((*it).c_str()),tunerReservationBarLabel); it++;

			tunerReservationBarLabel.clear();
			tunerReservationBarLabel.insert((const char *)Activity::kLive, monitorUI->tuner4LiveReservationIndicationBar);
			tunerReservationBarLabel.insert((const char *)Activity::kRecord, monitorUI->tuner4RecordReservationIndicationBar);
			tunerReservationBarLabels.insert(QString((*it).c_str()),tunerReservationBarLabel); it++;

		}
	}

	{/* Draw Tuner Status Square */
		const std::list<std::string> &tunerIds = monitor->getTunerIds();
		std::list<std::string>::const_iterator it = tunerIds.begin();

		for (it = tunerIds.begin(); it !=  tunerIds.end(); it++) {
			setTunerStateIndicator(QString((*it).c_str()), TunerState::kFree);
		}

	}
	{/* Draw Reservation Bars */
		monitorUI->tuner1LiveReservationIndicationBar->setVisible(true);
		monitorUI->tuner2LiveReservationIndicationBar->setVisible(true);
		monitorUI->tuner3LiveReservationIndicationBar->setVisible(true);
		monitorUI->tuner4LiveReservationIndicationBar->setVisible(true);

		monitorUI->tuner1RecordReservationIndicationBar->setVisible(true);
		monitorUI->tuner2RecordReservationIndicationBar->setVisible(true);
		monitorUI->tuner3RecordReservationIndicationBar->setVisible(true);
		monitorUI->tuner4RecordReservationIndicationBar->setVisible(true);

		drawReservationBar(monitorUI->tuner1LiveReservationIndicationBar, 0);
		drawReservationBar(monitorUI->tuner2LiveReservationIndicationBar, 0);
		drawReservationBar(monitorUI->tuner3LiveReservationIndicationBar, 0);
		drawReservationBar(monitorUI->tuner4LiveReservationIndicationBar, 0);

		drawReservationBar(monitorUI->tuner1RecordReservationIndicationBar, 0);
		drawReservationBar(monitorUI->tuner2RecordReservationIndicationBar, 0);
		drawReservationBar(monitorUI->tuner3RecordReservationIndicationBar, 0);
		drawReservationBar(monitorUI->tuner4RecordReservationIndicationBar, 0);
	}

	{/*Setup Reservation Bar Actions, action data is ReservationToken */

		QList<QString> tunerIds = tunerReservationBarLabels.keys();
		for (int i = 0; i < tunerIds.size(); i++) {
			QList<QString> activities = tunerReservationBarLabels[tunerIds[i]].keys();
			for (int j = 0; j < activities.size(); j++) {
				QLabel *reservationIndicationBar = tunerReservationBarLabels[tunerIds[i]][activities[j]];
				qDebug() << "Adding Action for " << tunerIds[i] << "for activity" << activities[j];
				QAction *releaseTunerReservationAction = new QAction(tr("Release"), this);
				releaseTunerReservationAction->setVisible(false);
				releaseTunerReservationAction->setEnabled(false);
				releaseTunerReservationAction->setData(0);
				reservationIndicationBar->addAction(releaseTunerReservationAction);
				QObject::connect(releaseTunerReservationAction, SIGNAL(triggered()), this, SLOT(onReleaseTunerReservationActionTriggered()));

				QAction *validateTunerReservationAction = new QAction(tr("Validate"), this);
				validateTunerReservationAction->setVisible(false);
				validateTunerReservationAction->setEnabled(false);
				validateTunerReservationAction->setData(0);
				reservationIndicationBar->addAction(validateTunerReservationAction);
				QObject::connect(validateTunerReservationAction, SIGNAL(triggered()), this, SLOT(onValidateTunerReservationActionTriggered()));

				QAction *cancelRecordingAction = new QAction(tr("CancelRecording"), this);
				cancelRecordingAction->setVisible(false);
				cancelRecordingAction->setEnabled(false);
				cancelRecordingAction->setData(0);
				reservationIndicationBar->addAction(cancelRecordingAction);
				QObject::connect(cancelRecordingAction, SIGNAL(triggered()), this, SLOT(onCancelRecordingActionTriggered()));

				QAction *renewTunerReservationAction = new QAction(tr("Renew"), this);
				renewTunerReservationAction->setVisible(false);
				renewTunerReservationAction->setEnabled(false);
				renewTunerReservationAction->setData(0);
				reservationIndicationBar->addAction(renewTunerReservationAction);
				QObject::connect(renewTunerReservationAction, SIGNAL(triggered()), this, SLOT(onRenewTunerReservationActionTriggered()));

				reservationIndicationBar->setContextMenuPolicy(Qt::ActionsContextMenu);

			}

		}
	}
}

void TRMMonitorGUI::onTunerStatesUpdated(std::map<std::string, std::string> states)
{
	qDebug() << "onTunerStatesUpdated";
	std::map<std::string, std::string>::iterator it = states.begin();
	Q_ASSERT(states.size() == TOTAL_TUNER_NUMBER);
	for (it = states.begin(); it != states.end(); it++ ) {
		qDebug() << "Updating tuner [" << (*it).first.c_str() << "] state to " << (*it).second.c_str();
		setTunerStateIndicator((*it).first.c_str(), TunerState((*it).second.c_str()));
		tunerStates[(*it).first.c_str()] = (*it).second.c_str();
	}
}

void TRMMonitorGUI::onTunerReservationsUpdated(std::map<std::string, std::list<TunerReservation> > reservations)
{
#if 1
	std::map<std::string, std::list<TunerReservation> >::iterator it = reservations.begin();

	qDebug() << "onTunerReservationsUpdated===========================with tuner size ===  " << reservations.size();
    {
        //wipe out everything first.
        drawReservationBar(monitorUI->tuner1LiveReservationIndicationBar, 0);
        drawReservationBar(monitorUI->tuner2LiveReservationIndicationBar, 0);
        drawReservationBar(monitorUI->tuner3LiveReservationIndicationBar, 0);
        drawReservationBar(monitorUI->tuner4LiveReservationIndicationBar, 0);

        drawReservationBar(monitorUI->tuner1RecordReservationIndicationBar, 0);
        drawReservationBar(monitorUI->tuner2RecordReservationIndicationBar, 0);
        drawReservationBar(monitorUI->tuner3RecordReservationIndicationBar, 0);
        drawReservationBar(monitorUI->tuner4RecordReservationIndicationBar, 0);
    }
	//Q_ASSERT(tunerIds.size() <= TOTAL_TUNER_NUMBER);
	for (it = reservations.begin(); it != reservations.end(); it++ ) {
		std::list<TunerReservation> & tunerReservations = (*it).second;

		qDebug() << "===========================Updating tuner [" << (*it).first.c_str() << "]";
		/* first set all actions invisible */
		{
			QList<QAction *> actions = tunerReservationBarLabels[(*it).first.c_str()][(const char *)Activity::kLive]->actions();
			for(int j = 0; j < actions.size(); j++) {
				actions[j]->setVisible(false);
				actions[j]->setEnabled(false);
				TunerReservation *reservation = (TunerReservation *)((actions[j]->data()).toInt());
				delete reservation;
				actions[j]->setData(0);
			}
			drawReservationBar(tunerReservationBarLabels[(*it).first.c_str()][(const char *)Activity::kLive], 0);

			actions = tunerReservationBarLabels[(*it).first.c_str()][(const char *)Activity::kRecord]->actions();
			for(int j = 0; j < actions.size(); j++) {
				actions[j]->setVisible(false);
				actions[j]->setEnabled(false);
				TunerReservation *reservation = (TunerReservation *)((actions[j]->data()).toInt());
				delete reservation;
				actions[j]->setData(0);
			}
			drawReservationBar(tunerReservationBarLabels[(*it).first.c_str()][(const char *)Activity::kRecord], 0);
		}

		/* Update actions */
		std::list<TunerReservation>::iterator it2 = tunerReservations.begin();
		for (it2 = tunerReservations.begin(); it2 != tunerReservations.end(); it2++) {
			qDebug() << " udpating action for "
					 << QString((*it2).getReservationToken().c_str())
			         << "  WITH STAR TIME " << (*it2).getStartTime()
			         << "  WITH DURATION  " << (*it2).getDuration()
			         ;

			QLabel *reservationBar = tunerReservationBarLabels[(*it).first.c_str()][(const char *)((*it2).getActivity().getActivity())];
			QList<QAction *> actions = reservationBar->actions();
			for(int k = 0; k < actions.size(); k++) {
				actions[k]->setVisible(true);
				actions[k]->setEnabled(true);
#if 1
				//save entire copy
				TunerReservation *copyReservation = new TunerReservation();
				*copyReservation = (*it2);
				actions[k]->setData((int)copyReservation);
#else
				//just save the token
				actions[k]->setData(QString((*it2).getReservationToken().c_str()));
#endif
			}
			drawReservationBar(reservationBar, &(*it2));
		}
	}


#endif
}

void TRMMonitorGUI::onReleaseTunerReservationActionTriggered(void)
{
	QAction *action = qobject_cast<QAction *>(sender());
	if (action) {
		TunerReservation *reservation = (TunerReservation *)(action->data().toInt());
		qDebug() << reservation->getReservationToken().c_str();
		const QString &reservationToken =  reservation->getReservationToken().c_str();
		if (!reservationToken.isEmpty()) {
			monitor->sendReleaseTunerReservation(reservationToken);
		}
	}
	qDebug() << "sendReleaseTunerReservation done";
}

void TRMMonitorGUI::onValidateTunerReservationActionTriggered(void)
{
	QAction *action = qobject_cast<QAction *>(sender());
	if (action) {
		TunerReservation *reservation = (TunerReservation *)(action->data().toInt());
		qDebug() << reservation->getReservationToken().c_str();
		const QString &reservationToken =  reservation->getReservationToken().c_str();
		if (!reservationToken.isEmpty()) {
			monitor->sendValidateTunerReservation("MonitorUI", reservationToken);
		}
	}
	qDebug() << "sendValidateTunerReservation done";
}

void TRMMonitorGUI::onCancelRecordingActionTriggered(void)
{
	QAction *action = qobject_cast<QAction *>(sender());
	if (action) {
		TunerReservation *reservation = (TunerReservation *)(action->data().toInt());
		qDebug() << reservation->getReservationToken().c_str();
		const QString &reservationToken =  reservation->getReservationToken().c_str();
		if (!reservationToken.isEmpty()) {
			monitor->sendCancelRecording("MonitorUI", reservationToken);
		}
	}
	qDebug() << "sendValidateTunerReservation done";
}

void TRMMonitorGUI::onRenewTunerReservationActionTriggered(void)
{
	QAction *action = qobject_cast<QAction *>(sender());
	if (action) {
		TunerReservation *reservation = (TunerReservation *)(action->data().toInt());
		monitor->sendTunerReserve(reservation->getDevice().c_str(),
						  QDateTime(QDate::currentDate(), QTime::currentTime()).toMSecsSinceEpoch(),
						  reservation->getDuration(),
						  reservation->getServiceLocator().c_str(),
						  (const char *)reservation->getActivity().getActivity(),
						  reservation->getReservationToken().c_str());
	}
	qDebug() << "sendRenewTunerReservation done";
}


void TRMMonitorGUI::onStatusMessageReceived(std::string statusMessage)
{
	//QMessageBox::information(this, "TRM Status Message", statusMessage);
	monitorUI->logTextEdit->clear();
	monitorUI->logTextEdit->setText(QString(statusMessage.c_str()));
}

void TRMMonitorGUI::onConflictsReceived(ReserveTunerResponse::ConflictCT conflicts)
{
	QString conflictMessage;
	ReserveTunerResponse::ConflictCT::iterator it = conflicts.begin();

	conflictMessage += ("<b>" + clientId + "<br>");
	conflictMessage += "<b>Requested Activity</b> is in conflict with the following group of activities\r\n\r\n<br>";
	conflictMessage += ("<br>");
	conflictMessage += ("<br>");

	for (it = conflicts.begin(); it != conflicts.end(); it++) {
		conflictMessage += "    " + QString((const char *)((*it).getActivity().getActivity())) + " : " + QString((*it).getServiceLocator().c_str()) + "<br>";
		conflictMessage += ("<br>");
	}

	conflictMessage += ("<br>");
	conflictMessage += ("<br>");

	conflictMessage += "Please cancel one group and retry your request<br>";
	QMessageBox::information(this, "Conflicts", conflictMessage);

}



/** @} */
/** @} */
