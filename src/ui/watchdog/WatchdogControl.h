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
 *   @brief Definition of class WatchdogControl
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#ifndef WATCHDOGCONTROL_H
#define WATCHDOGCONTROL_H

#include <inttypes.h>

#include <QWidget>
#include <QTimer>
#include <QVBoxLayout>

#include <map>
#include <string>
#include <sstream>

#include "WatchdogView.h"

#include "UASInterface.h"

namespace Ui
{
class WatchdogControl;
}

/**
 * @brief Overall widget for controlling all watchdogs of all connected MAVs
 */
class WatchdogControl : public QWidget
{
    Q_OBJECT
public:


    ///! Command codes, used to send and receive commands over lcm
    struct Command {
        enum Enum {
            Start         = 0,
            Restart       = 1,
            Stop          = 2,
            Mute          = 3,
            Unmute        = 4,

            RequestInfo   = 254,
            RequestStatus = 255
        };
    };

    ///! This struct represents a process on the watchdog. Used to store all values.
    struct ProcessInfo {
        ///! Process state - each process is in exactly one of those states (except unknown, that's just to initialize it)
        struct State {
            enum Enum {
                Unknown       = 0,
                Running       = 1,
                Stopped       = 2,
                Stopped_OK    = 3,
                Stopped_ERROR = 4
            };
        };

        ///! Constructor - initialize the values
        ProcessInfo() : timeout_(0), state_(State::Unknown), muted_(false), crashes_(0), pid_(-1) {}

        std::string name_;      ///< The name of the process
        std::string arguments_; ///< The arguments (argv of the process)

        int32_t timeout_;       ///< Heartbeat timeout value (in microseconds)

        State::Enum state_;     ///< The current state of the process
        bool muted_;            ///< True if the process is currently muted
        uint16_t crashes_;      ///< The number of crashes
        int32_t pid_;           ///< The PID of the process

        //quint64_t requestTimeout;
        //    Timer requestTimer_;    ///< Internal timer, used to repeat status and info requests after some time (in case of packet loss)
        //    Timer updateTimer_;     ///< Internal timer, used to measure the time since the last update (used only for graphics)
    };

    ///! This struct identifies a watchdog. It's a combination of system-ID and watchdog-ID. implements operator< to be used as key in a std::map
    struct WatchdogID {
        ///! Constructor - initialize the values
        WatchdogID(uint8_t system_id, uint16_t watchdog_id) : system_id_(system_id), watchdog_id_(watchdog_id) {}

        uint8_t system_id_;     ///< The system-ID
        uint16_t watchdog_id_;  ///< The watchdog-ID

        ///! Comparison operator which is used by std::map
        inline bool operator<(const WatchdogID& other) const {
            return (this->system_id_ != other.system_id_) ? (this->system_id_ < other.system_id_) : (this->watchdog_id_ < other.watchdog_id_);
        }

    };

    ///! This struct represents a watchdog
    struct WatchdogInfo {
        ProcessInfo& getProcess(uint16_t index);

        std::vector<ProcessInfo> processes_;    ///< A vector containing all processes running on this watchdog
        uint64_t timeout;
        QTimer* timeoutTimer_;                    ///< Internal timer, used to measure the time since the last heartbeat message
    };

    WatchdogControl(QWidget *parent = 0);
    ~WatchdogControl();

    static const uint16_t ALL         = (uint16_t)-1;   ///< A magic value for a process-ID which addresses "all processes"
    static const uint16_t ALL_RUNNING = (uint16_t)-2;   ///< A magic value for a process-ID which addresses "all running processes"
    static const uint16_t ALL_CRASHED = (uint16_t)-3;   ///< A magic value for a process-ID which addresses "all crashed processes"

public slots:
    void updateWatchdog(int systemId, int watchdogId, unsigned int processCount);
    void addProcess(int systemId, int watchdogId, int processId, QString name, QString arguments, int timeout);
    void updateProcess(int systemId, int watchdogId, int processId, int state, bool muted, int crashed, int pid);
    void setUAS(UASInterface* uas);

signals:
    void sendProcessCommand(int watchdogId, int processId, unsigned int command);

protected:
    void changeEvent(QEvent *e);

    UASInterface* mav;
    QVBoxLayout* listLayout;
    uint64_t updateInterval;

private:
    Ui::WatchdogControl *ui;

    void sendCommand(const WatchdogID& w_id, uint16_t p_id, Command::Enum command);

    WatchdogInfo& getWatchdog(uint8_t system_id, uint16_t watchdog_id);

    std::map<WatchdogID, WatchdogInfo> watchdogs_;          ///< A map containing all watchdogs which are currently active
    std::map<WatchdogID, WatchdogView> views;
    QTimer updateTimer_;
};

#endif // WATCHDOGCONTROL_H

///! Convert a value to std::string
template <class T>
std::string convertToString(T value)
{
    std::ostringstream oss;
    oss << value;
    return oss.str();
}
