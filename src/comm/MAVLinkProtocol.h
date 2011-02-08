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
 *   @brief Definition of class MAVLinkProtocol
 *   @author Lorenz Meier <mail@qgroundcontrol.org>
 */

#ifndef MAVLINKPROTOCOL_H_
#define MAVLINKPROTOCOL_H_

#include <QObject>
#include <QMutex>
#include <QString>
#include <QTimer>
#include <QFile>
#include <QMap>
#include <QByteArray>
#include "ProtocolInterface.h"
#include "LinkInterface.h"
#include "QGCMAVLink.h"
#include "QGC.h"

/**
 * @brief MAVLink micro air vehicle protocol reference implementation.
 *
 * MAVLink is a generic communication protocol for micro air vehicles.
 * for more information, please see the official website.
 * @ref http://pixhawk.ethz.ch/software/mavlink/
 **/
class MAVLinkProtocol : public ProtocolInterface {
    Q_OBJECT

public:
    MAVLinkProtocol();
    ~MAVLinkProtocol();

    void run();
    /** @brief Get the human-friendly name of this protocol */
    QString getName();
    /** @brief Get the system id of this application */
    int getSystemId();
    /** @brief Get the component id of this application */
    int getComponentId();
    /** @brief The auto heartbeat emission rate in Hertz */
    int getHeartbeatRate();
    /** @brief Get heartbeat state */
    bool heartbeatsEnabled() const { return m_heartbeatsEnabled; }
    /** @brief Get logging state */
    bool loggingEnabled() const { return m_loggingEnabled; }
    /** @brief Get protocol version check state */
    bool versionCheckEnabled() const { return m_enable_version_check; }
    /** @brief Get the multiplexing state */
    bool multiplexingEnabled() const { return m_multiplexingEnabled; }
    /** @brief Get the protocol version */
    int getVersion() { return MAVLINK_VERSION; }
    /** @brief Get the name of the packet log file */
    QString getLogfileName();

public slots:
    /** @brief Receive bytes from a communication interface */
    void receiveBytes(LinkInterface* link, QByteArray b);
    /** @brief Send MAVLink message through serial interface */
    void sendMessage(mavlink_message_t message);
    /** @brief Send MAVLink message through serial interface */
    void sendMessage(LinkInterface* link, mavlink_message_t message);
    /** @brief Set the rate at which heartbeats are emitted */
    void setHeartbeatRate(int rate);
    /** @brief Set the system id of this application */
    void setSystemId(int id);

    /** @brief Enable / disable the heartbeat emission */
    void enableHeartbeats(bool enabled);

    /** @brief Enable/disable binary packet logging */
    void enableLogging(bool enabled);

    /** @brief Enabled/disable packet multiplexing */
    void enableMultiplexing(bool enabled);

    /** @brief Set log file name */
    void setLogfileName(const QString& filename);

    /** @brief Enable / disable version check */
    void enableVersionCheck(bool enabled);

    /** @brief Send an extra heartbeat to all connected units */
    void sendHeartbeat();

    /** @brief Load protocol settings */
    void loadSettings();
    /** @brief Store protocol settings */
    void storeSettings();

protected:
    QTimer* heartbeatTimer;    ///< Timer to emit heartbeats
    int heartbeatRate;         ///< Heartbeat rate, controls the timer interval
    bool m_heartbeatsEnabled;  ///< Enabled/disable heartbeat emission
    bool m_loggingEnabled;     ///< Enable/disable packet logging
    bool m_multiplexingEnabled; ///< Enable/disable packet multiplexing
    QFile* m_logfile;           ///< Logfile
    bool m_enable_version_check; ///< Enable checking of version match of MAV and QGC
    QMutex receiveMutex;       ///< Mutex to protect receiveBytes function
    int lastIndex[256][256];
    int totalReceiveCounter;
    int totalLossCounter;
    int currReceiveCounter;
    int currLossCounter;
    bool versionMismatchIgnore;
    int systemId;

signals:
    /** @brief Message received and directly copied via signal */
    void messageReceived(LinkInterface* link, mavlink_message_t message);
    /** @brief Emitted if heartbeat emission mode is changed */
    void heartbeatChanged(bool heartbeats);
    /** @brief Emitted if logging is started / stopped */
    void loggingChanged(bool enabled);
    /** @brief Emitted if multiplexing is started / stopped */
    void multiplexingChanged(bool enabled);
    /** @brief Emitted if version check is enabled / disabled */
    void versionCheckChanged(bool enabled);
    /** @brief Emitted if a message from the protocol should reach the user */
    void protocolStatusMessage(const QString& title, const QString& message);
};

#endif // MAVLINKPROTOCOL_H_
