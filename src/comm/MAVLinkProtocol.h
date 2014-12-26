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
#include <QLoggingCategory>

#include "LinkInterface.h"
#include "QGCMAVLink.h"
#include "QGC.h"
#include "QGCTemporaryFile.h"
#include "QGCSingleton.h"

class LinkManager;

Q_DECLARE_LOGGING_CATEGORY(MAVLinkProtocolLog)

/**
 * @brief MAVLink micro air vehicle protocol reference implementation.
 *
 * MAVLink is a generic communication protocol for micro air vehicles.
 * for more information, please see the official website.
 * @ref http://pixhawk.ethz.ch/software/mavlink/
 **/
class MAVLinkProtocol : public QGCSingleton
{
    Q_OBJECT
    
    DECLARE_QGC_SINGLETON(MAVLinkProtocol, MAVLinkProtocol)

public:
    /** @brief Get the human-friendly name of this protocol */
    QString getName();
    /** @brief Get the system id of this application */
    int getSystemId();
    /** @brief Get the component id of this application */
    int getComponentId();
    /** @brief The auto heartbeat emission rate in Hertz */
    int getHeartbeatRate();
    /** @brief Get heartbeat state */
    bool heartbeatsEnabled() const {
        return _heartbeatsEnabled;
    }
    
    /** @brief Get protocol version check state */
    bool versionCheckEnabled() const {
        return m_enable_version_check;
    }
    /** @brief Get the multiplexing state */
    bool multiplexingEnabled() const {
        return m_multiplexingEnabled;
    }
    /** @brief Get the authentication state */
    bool getAuthEnabled() {
        return m_authEnabled;
    }
    /** @brief Get the protocol version */
    int getVersion() {
        return MAVLINK_VERSION;
    }
    /** @brief Get the auth key */
    QString getAuthKey() {
        return m_authKey;
    }
    /** @brief Get state of parameter retransmission */
    bool paramGuardEnabled() {
        return m_paramGuardEnabled;
    }
    /** @brief Get parameter read timeout */
    int getParamRetransmissionTimeout() {
        return m_paramRetransmissionTimeout;
    }
    /** @brief Get parameter write timeout */
    int getParamRewriteTimeout() {
        return m_paramRewriteTimeout;
    }
    /** @brief Get state of action retransmission */
    bool actionGuardEnabled() {
        return m_actionGuardEnabled;
    }
    /** @brief Get parameter read timeout */
    int getActionRetransmissionTimeout() {
        return m_actionRetransmissionTimeout;
    }
    /**
     * Retrieve a total of all successfully parsed packets for the specified link.
     * @returns -1 if this is not available for this protocol, # of packets otherwise.
     */
    qint32 getReceivedPacketCount(const LinkInterface *link) const {
        return totalReceiveCounter[link->getId()];
    }
    /**
     * Retrieve a total of all parsing errors for the specified link.
     * @returns -1 if this is not available for this protocol, # of errors otherwise.
     */
    qint32 getParsingErrorCount(const LinkInterface *link) const {
        return totalErrorCounter[link->getId()];
    }
    /**
     * Retrieve a total of all dropped packets for the specified link.
     * @returns -1 if this is not available for this protocol, # of packets otherwise.
     */
    qint32 getDroppedPacketCount(const LinkInterface *link) const {
        return totalLossCounter[link->getId()];
    }
    /**
     * Reset the counters for all metadata for this link.
     */
    virtual void resetMetadataForLink(const LinkInterface *link);
    
    /// Suspend/Restart logging during replay.
    void suspendLogForReplay(bool suspend);

public slots:
    /** @brief Receive bytes from a communication interface */
    void receiveBytes(LinkInterface* link, QByteArray b);
    
    void linkConnected(void);
    void linkDisconnected(void);
    
    /** @brief Send MAVLink message through serial interface */
    void sendMessage(mavlink_message_t message);
    /** @brief Send MAVLink message */
    void sendMessage(LinkInterface* link, mavlink_message_t message);
    /** @brief Send MAVLink message with correct system / component ID */
    void sendMessage(LinkInterface* link, mavlink_message_t message, quint8 systemid, quint8 componentid);
    /** @brief Set the rate at which heartbeats are emitted */
    void setHeartbeatRate(int rate);
    /** @brief Set the system id of this application */
    void setSystemId(int id);

    /** @brief Enable / disable the heartbeat emission */
    void enableHeartbeats(bool enabled);

    /** @brief Enabled/disable packet multiplexing */
    void enableMultiplexing(bool enabled);

    /** @brief Enable / disable parameter retransmission */
    void enableParamGuard(bool enabled);

    /** @brief Enable / disable action retransmission */
    void enableActionGuard(bool enabled);

    /** @brief Set parameter read timeout */
    void setParamRetransmissionTimeout(int ms);

    /** @brief Set parameter write timeout */
    void setParamRewriteTimeout(int ms);

    /** @brief Set parameter read timeout */
    void setActionRetransmissionTimeout(int ms);

    /** @brief Enable / disable version check */
    void enableVersionCheck(bool enabled);

    /** @brief Enable / disable authentication */
    void enableAuth(bool enable);

    /** @brief Set authentication token */
    void setAuthKey(QString key) {
        m_authKey = key;
    }

    /** @brief Send an extra heartbeat to all connected units */
    void sendHeartbeat();

    /** @brief Load protocol settings */
    void loadSettings();
    /** @brief Store protocol settings */
    void storeSettings();
    
    /// @brief Deletes any log files which are in the temp directory
    static void deleteTempLogFiles(void);
    
    /// Checks for lost log files
    void checkForLostLogFiles(void);

protected:
    bool m_multiplexingEnabled; ///< Enable/disable packet multiplexing
    bool m_authEnabled;        ///< Enable authentication token broadcast
    QString m_authKey;         ///< Authentication key
    bool m_enable_version_check; ///< Enable checking of version match of MAV and QGC
    int m_paramRetransmissionTimeout; ///< Timeout for parameter retransmission
    int m_paramRewriteTimeout;    ///< Timeout for sending re-write request
    bool m_paramGuardEnabled;       ///< Parameter retransmission/rewrite enabled
    bool m_actionGuardEnabled;       ///< Action request retransmission enabled
    int m_actionRetransmissionTimeout; ///< Timeout for parameter retransmission
    QMutex receiveMutex;        ///< Mutex to protect receiveBytes function
    int lastIndex[256][256];    ///< Store the last received sequence ID for each system/componenet pair
    int totalReceiveCounter[MAVLINK_COMM_NUM_BUFFERS];    ///< The total number of successfully received messages
    int totalLossCounter[MAVLINK_COMM_NUM_BUFFERS];       ///< Total messages lost during transmission.
    int totalErrorCounter[MAVLINK_COMM_NUM_BUFFERS];      ///< Total count of all parsing errors. Generally <= totalLossCounter.
    int currReceiveCounter[MAVLINK_COMM_NUM_BUFFERS];     ///< Received messages during this sample time window. Used for calculating loss %.
    int currLossCounter[MAVLINK_COMM_NUM_BUFFERS];        ///< Lost messages during this sample time window. Used for calculating loss %.
    bool versionMismatchIgnore;
    int systemId;

signals:
    /** @brief Message received and directly copied via signal */
    void messageReceived(LinkInterface* link, mavlink_message_t message);
    /** @brief Emitted if heartbeat emission mode is changed */
    void heartbeatChanged(bool heartbeats);
    /** @brief Emitted if multiplexing is started / stopped */
    void multiplexingChanged(bool enabled);
    /** @brief Emitted if authentication support is enabled / disabled */
    void authKeyChanged(QString key);
    /** @brief Authentication changed */
    void authChanged(bool enabled);
    /** @brief Emitted if version check is enabled / disabled */
    void versionCheckChanged(bool enabled);
    /** @brief Emitted if a message from the protocol should reach the user */
    void protocolStatusMessage(const QString& title, const QString& message);
    /** @brief Emitted if a new system ID was set */
    void systemIdChanged(int systemId);
    /** @brief Emitted if param guard status changed */
    void paramGuardChanged(bool enabled);
    /** @brief Emitted if param read timeout changed */
    void paramRetransmissionTimeoutChanged(int ms);
    /** @brief Emitted if param write timeout changed */
    void paramRewriteTimeoutChanged(int ms);
    /** @brief Emitted if action guard status changed */
    void actionGuardChanged(bool enabled);
    /** @brief Emitted if actiion request timeout changed */
    void actionRetransmissionTimeoutChanged(int ms);
    /** @brief Update the packet loss from one system */
    void receiveLossChanged(int uasId, float loss);

    /**
     * @brief Emitted if a new radio status packet received
     *
     * @param rxerrors receive errors
     * @param fixed count of error corrected packets
     * @param rssi local signal strength
     * @param remrssi remote signal strength
     * @param txbuf how full the tx buffer is as a percentage
     * @param noise background noise level
     * @param remnoise remote background noise level
     */
    void radioStatusChanged(LinkInterface* link, unsigned rxerrors, unsigned fixed, unsigned rssi, unsigned remrssi,
    unsigned txbuf, unsigned noise, unsigned remnoise);
    
    /// @brief Emitted when a temporary log file is ready for saving
    void saveTempFlightDataLog(QString tempLogfile);
    
private:
    MAVLinkProtocol(QObject* parent = NULL);
    ~MAVLinkProtocol();

    void _linkStatusChanged(LinkInterface* link, bool connected);
    bool _closeLogFile(void);
    void _startLogging(void);
    void _stopLogging(void);
    
    QList<LinkInterface*> _connectedLinks;  ///< List of all links connected to protocol
    
    bool _logSuspendError;      ///< true: Logging suspended due to error
    bool _logSuspendReplay;     ///< true: Logging suspended due to replay
    
    QGCTemporaryFile    _tempLogFile;            ///< File to log to
    static const char*  _tempLogFileTemplate;    ///< Template for temporary log file
    static const char*  _logFileExtension;       ///< Extension for log files
    
    LinkManager* _linkMgr;
    
    QTimer  _heartbeatTimer;    ///< Timer to emit heartbeats
    int     _heartbeatRate;     ///< Heartbeat rate, controls the timer interval
    bool    _heartbeatsEnabled; ///< Enabled/disable heartbeat emission
};

#endif // MAVLINKPROTOCOL_H_
