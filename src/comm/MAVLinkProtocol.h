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
#include "QGCToolbox.h"

class LinkManager;
class MultiVehicleManager;
class QGCApplication;

Q_DECLARE_LOGGING_CATEGORY(MAVLinkProtocolLog)

/**
 * @brief MAVLink micro air vehicle protocol reference implementation.
 *
 * MAVLink is a generic communication protocol for micro air vehicles.
 * for more information, please see the official website.
 * @ref http://pixhawk.ethz.ch/software/mavlink/
 **/
class MAVLinkProtocol : public QGCTool
{
    Q_OBJECT

public:
    MAVLinkProtocol(QGCApplication* app);
    ~MAVLinkProtocol();

    /** @brief Get the human-friendly name of this protocol */
    QString getName();
    /** @brief Get the system id of this application */
    int getSystemId();
    /** @brief Get the component id of this application */
    int getComponentId();
    
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
        return totalReceiveCounter[link->getMavlinkChannel()];
    }
    /**
     * Retrieve a total of all parsing errors for the specified link.
     * @returns -1 if this is not available for this protocol, # of errors otherwise.
     */
    qint32 getParsingErrorCount(const LinkInterface *link) const {
        return totalErrorCounter[link->getMavlinkChannel()];
    }
    /**
     * Retrieve a total of all dropped packets for the specified link.
     * @returns -1 if this is not available for this protocol, # of packets otherwise.
     */
    qint32 getDroppedPacketCount(const LinkInterface *link) const {
        return totalLossCounter[link->getMavlinkChannel()];
    }
    /**
     * Reset the counters for all metadata for this link.
     */
    virtual void resetMetadataForLink(const LinkInterface *link);
    
    /// Suspend/Restart logging during replay.
    void suspendLogForReplay(bool suspend);

    // Override from QGCTool
    virtual void setToolbox(QGCToolbox *toolbox);

public slots:
    /** @brief Receive bytes from a communication interface */
    void receiveBytes(LinkInterface* link, QByteArray b);
    
    /** @brief Set the system id of this application */
    void setSystemId(int id);

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

    /** @brief Load protocol settings */
    void loadSettings();
    /** @brief Store protocol settings */
    void storeSettings();
    
#ifndef __mobile__
    /// @brief Deletes any log files which are in the temp directory
    static void deleteTempLogFiles(void);
    
    /// Checks for lost log files
    void checkForLostLogFiles(void);
#endif

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
    /// Heartbeat received on link
    void vehicleHeartbeatInfo(LinkInterface* link, int vehicleId, int vehicleMavlinkVersion, int vehicleFirmwareType, int vehicleType);

    /** @brief Message received and directly copied via signal */
    void messageReceived(LinkInterface* link, mavlink_message_t message);
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
    /** @brief Emitted if action request timeout changed */
    void actionRetransmissionTimeoutChanged(int ms);

    void receiveLossPercentChanged(int uasId, float lossPercent);
    void receiveLossTotalChanged(int uasId, int totalLoss);

    /**
     * @brief Emitted if a new radio status packet received
     *
     * @param rxerrors receive errors
     * @param fixed count of error corrected packets
     * @param rssi local signal strength in dBm
     * @param remrssi remote signal strength in dBm
     * @param txbuf how full the tx buffer is as a percentage
     * @param noise background noise level
     * @param remnoise remote background noise level
     */
    void radioStatusChanged(LinkInterface* link, unsigned rxerrors, unsigned fixed, int rssi, int remrssi,
    unsigned txbuf, unsigned noise, unsigned remnoise);
    
    /// @brief Emitted when a temporary log file is ready for saving
    void saveTempFlightDataLog(QString tempLogfile);

private slots:
    void _vehicleCountChanged(int count);
    
private:
    void _sendMessage(mavlink_message_t message);
    void _sendMessage(LinkInterface* link, mavlink_message_t message);
    void _sendMessage(LinkInterface* link, mavlink_message_t message, quint8 systemid, quint8 componentid);

#ifndef __mobile__
    bool _closeLogFile(void);
    void _startLogging(void);
    void _stopLogging(void);

    bool _logSuspendError;      ///< true: Logging suspended due to error
    bool _logSuspendReplay;     ///< true: Logging suspended due to replay
    bool _logPromptForSave;     ///< true: Prompt for log save when appropriate

    QGCTemporaryFile    _tempLogFile;            ///< File to log to
    static const char*  _tempLogFileTemplate;    ///< Template for temporary log file
    static const char*  _logFileExtension;       ///< Extension for log files
#endif

    LinkManager*            _linkMgr;
    MultiVehicleManager*    _multiVehicleManager;
};

#endif // MAVLINKPROTOCOL_H_
