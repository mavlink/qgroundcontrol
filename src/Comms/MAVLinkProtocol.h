/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QString>

#include "LinkInterface.h"
#include "MAVLinkLib.h"

class QGCTemporaryFile;

Q_DECLARE_LOGGING_CATEGORY(MAVLinkProtocolLog)

/// MAVLink micro air vehicle protocol reference implementation.
/// MAVLink is a generic communication protocol for micro air vehicles.
/// for more information, please see the official website: https://mavlink.io
class MAVLinkProtocol : public QObject
{
    Q_OBJECT

public:
    /// Constructs an MAVLinkProtocol object.
    ///     @param parent The parent QObject.
    explicit MAVLinkProtocol(QObject *parent = nullptr);

    /// Destructor for the MAVLinkProtocol class.
    ~MAVLinkProtocol();

    /// Gets the singleton instance of MAVLinkProtocol.
    ///     @return The singleton instance.
    static MAVLinkProtocol *instance();

    void init();

    /// Get the human-friendly name of this protocol
    static QString getName() { return QStringLiteral("MAVLink protocol"); }

    /// Get the system id of this application
    int getSystemId() const;

    /// Get the component id of this application
    static int getComponentId() { return MAV_COMP_ID_MISSIONPLANNER; }

    /// Get the protocol version
    static int getVersion() { return MAVLINK_VERSION; }

    /// Get the currently configured protocol version
    unsigned getCurrentVersion() const { return _currentVersion; }

    /// Reset the counters for all metadata for this link.
    void resetMetadataForLink(LinkInterface *link);

    /// Suspend/Restart logging during replay.
    void suspendLogForReplay(bool suspend) { _logSuspendReplay = suspend; }

    /// Set protocol version
    void setVersion(unsigned version);

    /// Checks the temp directory for log files which may have been left there.
    /// This could happen if QGC crashes without the temp log file being saved.
    /// Give the user an option to save these orphaned files.
    void checkForLostLogFiles();

signals:
    /// Heartbeat received on link
    void vehicleHeartbeatInfo(LinkInterface *link, int vehicleId, int componentId, int vehicleFirmwareType, int vehicleType);

    /// Message received and directly copied via signal
    void messageReceived(LinkInterface *link, const mavlink_message_t &message);

    void mavlinkMessageStatus(int sysid, uint64_t totalSent, uint64_t totalReceived, uint64_t totalLoss, float lossPercent);

public slots:
    /// Receive bytes from a communication interface and constructs a MAVLink packet
    ///     @param link The interface to read from
    void receiveBytes(LinkInterface *link, const QByteArray &data);

    /// Log bytes sent from a communication interface and logs a MAVLink packet.
    /// It can handle multiple links in parallel, as each link has it's own buffer/parsing state machine.
    ///     @param link The interface to read from
    void logSentBytes(const LinkInterface *link, const QByteArray &data);

    /// Deletes any log files which are in the temp directory
    static void deleteTempLogFiles();

private slots:
    void _vehicleCountChanged();

private:
    void _logData(LinkInterface *link, const mavlink_message_t &message);
    bool _closeLogFile();
    void _startLogging();
    void _stopLogging();

    void _forward(const mavlink_message_t &message);
    void _forwardSupport(const mavlink_message_t &message);

    void _updateCounters(uint8_t mavlinkChannel, const mavlink_message_t &message);
    bool _updateStatus(LinkInterface *link, const SharedLinkInterfacePtr linkPtr, uint8_t mavlinkChannel, const mavlink_message_t &message);
    void _updateVersion(LinkInterface *link, uint8_t mavlinkChannel);

    void _saveTelemetryLog(const QString &tempLogfile);
    bool _checkTelemetrySavePath();

    QGCTemporaryFile * const _tempLogFile = nullptr;

    bool _logSuspendError = false;  ///< true: Logging suspended due to error
    bool _logSuspendReplay = false; ///< true: Logging suspended due to replay
    bool _vehicleWasArmed = false;  ///< true: Vehicle was armed during log sequence

    uint8_t _lastIndex[256][256]{};                             ///< Store the last received sequence ID for each system/component pair
    uint8_t _firstMessage[256][256]{};                          ///< First message flag
    uint64_t _totalReceiveCounter[MAVLINK_COMM_NUM_BUFFERS]{};  ///< The total number of successfully received messages
    uint64_t _totalLossCounter[MAVLINK_COMM_NUM_BUFFERS]{};     ///< Total messages lost during transmission.
    float _runningLossPercent[MAVLINK_COMM_NUM_BUFFERS]{};      ///< Loss rate

    unsigned _currentVersion = 100;
    bool _initialized = false;

    static constexpr const char *_tempLogFileTemplate = "FlightDataXXXXXX"; ///< Template for temporary log file
    static constexpr const char *_logFileExtension = "mavlink";             ///< Extension for log files

    static constexpr uint8_t kMaxCompId = MAV_COMPONENT_ENUM_END - 1;
};
