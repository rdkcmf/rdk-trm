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


#ifndef _TRM_TRM_MONITOR_GUI_H
#define _TRM_TRM_MONITOR_GUI_H
#include <QApplication>
#include <QDialog>
#include <QPainter>
#include <QDebug>
#include <QHostAddress>
#include <string>

#include "ui_gui.h"
#include "trm/TunerReservation.h"
#include "trm/TunerState.h"
#include "trm/Messages.h"

namespace TRM {
class TRMMonitor;

class TRMMonitorGUI : public QMainWindow
{
	Q_OBJECT
public:
	TRMMonitorGUI(const QHostAddress &address, quint16 port, const QString &clientId, const QString &barWidth, QObject *parent = 0);
	virtual ~TRMMonitorGUI();

	void closeEvent(QCloseEvent *event) {
    	 std::cout << "Shutting down... " << std::endl;
    	 QApplication::quit();
     }
private slots:
	void onTimestampTimerTimeout(void);
	void onTunerStateUpdateTimerTimeout(void);
	void onReserveButtonClicked(void);
	void onTunerReservationsUpdated(std::map<std::string, std::list<TunerReservation> >);
	void onTunerIdsUpdated(std::list<std::string>);
	void onTunerStatesUpdated(std::map<std::string, std::string>);
	void onReleaseTunerReservationActionTriggered(void);
	void onValidateTunerReservationActionTriggered(void);
	void onRenewTunerReservationActionTriggered(void);
	void onCancelRecordingActionTriggered(void);
	void onStatusMessageReceived(std::string);
	void onConflictsReceived(ReserveTunerResponse::ConflictCT);
private:
	void setTunerStateIndicator(const QString &tunerId, const TunerState& state);
	void drawReservationBar(QLabel *label, TunerReservation *reservation);

	QHostAddress hostAddress;
	quint16 portNumber;
	QString clientId;

	Ui_TRMMonitor *monitorUI;
	TRMMonitor *monitor;

	QMap<QString, QLabel *> tunerStateIndicatorLabels;
	QMap<QString, QMap<QString, QLabel *> > tunerReservationBarLabels;
	QMap<QString, QString> tunerStates;
};

}
#endif


/** @} */
/** @} */
