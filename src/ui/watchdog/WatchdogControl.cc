/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2010 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

This file is part of the QGROUNDCONTROL project

    QGROUNDCONTROL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    QGROUNDCONTROL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 * @file
 *   @brief Implementation of class WatchdogControl
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#include "WatchdogControl.h"
#include "WatchdogView.h"
#include "WatchdogProcessView.h"
#include "ui_WatchdogControl.h"
#include "PxQuadMAV.h"

#include "UASManager.h"

#include <QDebug>

WatchdogControl::WatchdogControl(QWidget *parent) :
    QWidget(parent),
    mav(NULL),
    updateInterval(2000000),
    ui(new Ui::WatchdogControl)
{
    ui->setupUi(this);

    // UI is initialized, setup layout
    listLayout = new QVBoxLayout(ui->mainWidget);
    listLayout->setSpacing(6);
    listLayout->setMargin(0);
    listLayout->setAlignment(Qt::AlignTop);
    ui->mainWidget->setLayout(listLayout);

    connect(UASManager::instance(), SIGNAL(UASCreated(UASInterface*)), this, SLOT(setUAS(UASInterface*)));

    this->setVisible(false);
}

WatchdogControl::~WatchdogControl()
{
    delete ui;
}

void WatchdogControl::setUAS(UASInterface* uas)
{
    PxQuadMAV* qmav = dynamic_cast<PxQuadMAV*>(uas);

    if (qmav) {
        connect(qmav, SIGNAL(processReceived(int,int,int,QString,QString,int)), this, SLOT(addProcess(int,int,int,QString,QString,int)));
        connect(qmav, SIGNAL(watchdogReceived(int,int,uint)), this, SLOT(updateWatchdog(int,int,uint)));
        connect(qmav, SIGNAL(processChanged(int,int,int,int,bool,int,int)), this, SLOT(updateProcess(int,int,int,int,bool,int,int)));
    }
}

void WatchdogControl::updateWatchdog(int systemId, int watchdogId, unsigned int processCount)
{
    // request the watchdog with the given ID
    // Get the watchdog and request the info for it
    WatchdogInfo& watchdog = this->getWatchdog(systemId, watchdogId);

    // if the proces count doesn't match, the watchdog is either new or has changed - create a new vector with new (and empty) ProcessInfo structs.
    if (watchdog.processes_.size() != processCount) {
        watchdog.processes_ = std::vector<ProcessInfo>(processCount);
        // Create new UI widget
        //WatchdogView* view = new WatchdogView(this);

    }

    // start the timeout timer
    //watchdog.timeoutTimer_.reset();

    qDebug() << "WATCHDOG RECEIVED";
    //qDebug() << "<-- received mavlink_watchdog_heartbeat_t " << msg->sysid << " / " << payload.watchdog_id << " / " << payload.process_count << std::endl;
}

void WatchdogControl::addProcess(int systemId, int watchdogId, int processId, QString name, QString arguments, int timeout)
{
    // request the watchdog and the process with the given IDs
    WatchdogInfo& watchdog = this->getWatchdog(systemId, watchdogId);
    ProcessInfo& process = watchdog.getProcess(processId);

    // store the process information in the ProcessInfo struct
    process.name_ = name.toStdString();
    process.arguments_ = arguments.toStdString();
    process.timeout_ = timeout;
    qDebug() << "PROCESS RECEIVED";
    qDebug() << "SYS" << systemId << "WD" << watchdogId << "PROCESS" << processId << name << "ARG" << arguments << "TO" << timeout;
    //qDebug() << "<-- received mavlink_watchdog_process_info_t " << msg->sysid << " / " << (const char*)payload.name << " / " << (const char*)payload.arguments << " / " << payload.timeout << std::endl;
}


void WatchdogControl::updateProcess(int systemId, int watchdogId, int processId, int state, bool muted, int crashes, int pid)
{
    // request the watchdog and the process with the given IDs
    WatchdogInfo& watchdog = this->getWatchdog(systemId, watchdogId);
    ProcessInfo& process = watchdog.getProcess(processId);

    // store the status information in the ProcessInfo struct
    process.state_ = static_cast<ProcessInfo::State::Enum>(state);
    process.muted_ = muted;
    process.crashes_ = crashes;
    process.pid_ = pid;

    qDebug() << "PROCESS UPDATED";
    qDebug() << "SYS" << systemId << "WD" << watchdogId << "PROCESS" << processId << "STATE" << state << "CRASH" << crashes << "PID" << pid;

    //process.updateTimer_.reset();
    //qDebug() << "<-- received mavlink_watchdog_process_status_t " << msg->sysid << " / " << payload.state << " / " << payload.muted << " / " << payload.crashes << " / " << payload.pid << std::endl;
}

/**
    @brief Returns a WatchdogInfo struct that belongs to the watchdog with the given system-ID and watchdog-ID
*/
WatchdogControl::WatchdogInfo& WatchdogControl::getWatchdog(uint8_t systemId, uint16_t watchdogId)
{
    WatchdogID id(systemId, watchdogId);

    std::map<WatchdogID, WatchdogInfo>::iterator it = this->watchdogs_.find(id);
    if (it != this->watchdogs_.end()) {
        // the WatchdogInfo struct already exists in the map, return it
        return it->second;
    } else {
        // the WatchdogInfo struct doesn't exist - request info and status for all processes and create the struct
        this->sendCommand(id, WatchdogControl::ALL, Command::RequestInfo);
        this->sendCommand(id, WatchdogControl::ALL, Command::RequestStatus);
        return this->watchdogs_[id];
    }
}

/**
    @brief Returns a ProcessInfo struct that belongs to the process with the given ID.
*/
WatchdogControl::ProcessInfo& WatchdogControl::WatchdogInfo::getProcess(uint16_t index)
{
    // if the index is out of bounds, resize the vector
    if (index >= this->processes_.size())
        this->processes_.resize(index + 1);

    return this->processes_[index];
}

/**
    @brief Sends a watchdog command to a process on a given watchdog.
    @param w_id The WatchdogID struct (containing system-ID and watchdog-ID) that identifies the watchdog
    @param p_id The process-ID
    @param command The command-ID
*/
void WatchdogControl::sendCommand(const WatchdogID& w_id, uint16_t p_id, Command::Enum command)
{
    emit sendProcessCommand(w_id.watchdog_id_, p_id, command);
}

void WatchdogControl::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}
