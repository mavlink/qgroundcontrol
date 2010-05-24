#include "WatchdogControl.h"
#include "ui_WatchdogControl.h"

#include <QDebug>

WatchdogControl::WatchdogControl(QWidget *parent) :
        QWidget(parent),
        ui(new Ui::WatchdogControl)
{
    ui->setupUi(this);
}

WatchdogControl::~WatchdogControl()
{
    delete ui;
}

void WatchdogControl::updateWatchdog(int systemId, int watchdogId, unsigned int processCount)
{
    // request the watchdog with the given ID
    WatchdogInfo& watchdog = this->getWatchdog(systemId, watchdogId);

    // if the proces count doesn't match, the watchdog is either new or has changed - create a new vector with new (and empty) ProcessInfo structs.
    if (watchdog.processes_.size() != processCount)
        watchdog.processes_ = std::vector<ProcessInfo>(processCount);

    // start the timeout timer
    //watchdog.timeoutTimer_.reset();
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
    if (it != this->watchdogs_.end())
    {
        // the WatchdogInfo struct already exists in the map, return it
        return it->second;
    }
    else
    {
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
    /*
    mavlink_watchdog_command_t payload;
    payload.target_system_id = w_id.system_id_;
    payload.watchdog_id = w_id.watchdog_id_;
    payload.process_id = p_id;
    payload.command_id = (uint8_t)command;

    mavlink_message_t msg;
    mavlink_msg_watchdog_command_encode(sysid, compid, &msg, &payload);
    mavlink_message_t_publish(this->lcm_, "MAVLINK", &msg);*/
//std::cout << "--> sent mavlink_watchdog_command_t " << payload.target_system_id << " / " << payload.watchdog_id << " / " << payload.process_id << " / " << (int)payload.command_id << std::endl;
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
