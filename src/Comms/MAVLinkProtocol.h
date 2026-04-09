#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QObject>
#include <QtCore/QString>

#include "LinkInterface.h"
#include "MAVLinkEnums.h"
#include "MAVLinkMessageType.h"

class QFile;

/// MAVLink micro air vehicle protocol reference implementation.
class MAVLinkProtocol : public QObject
{
    Q_OBJECT

public:
    explicit MAVLinkProtocol(QObject* parent = nullptr);

    ~MAVLinkProtocol();

    static MAVLinkProtocol* instance();

    void init();

    static QString getName() { return QStringLiteral("MAVLink protocol"); }

    int getSystemId() const;

    static int getComponentId() { return MAV_COMP_ID_MISSIONPLANNER; }

    void resetMetadataForLink(LinkInterface* link);

    /// Reset sequence tracking so signing transitions don't inflate loss counters.
    void resetSequenceTracking(LinkInterface* link);

    void suspendLogForReplay(bool suspend) { _logSuspendReplay = suspend; }

    void checkForLostLogFiles();

signals:
    void vehicleHeartbeatInfo(LinkInterface* link, int vehicleId, int componentId, int vehicleFirmwareType,
                              int vehicleType);

    void messageReceived(LinkInterface* link, const mavlink_message_t& message);

    void mavlinkMessageStatus(int sysid, uint64_t totalSent, uint64_t totalReceived, uint64_t totalLoss,
                              float lossPercent);

public slots:
    void receiveBytes(LinkInterface* link, const QByteArray& data);

    void logSentBytes(const LinkInterface* link, const QByteArray& data);

    static void deleteTempLogFiles();

private slots:
    void _vehicleCountChanged();

private:
    void _logData(LinkInterface* link, const mavlink_message_t& message);
    bool _closeLogFile();
    void _startLogging();
    void _stopLogging();

    void _forward(const mavlink_message_t& message);
    void _forwardSupport(const mavlink_message_t& message);

    void _updateCounters(uint8_t mavlinkChannel, const mavlink_message_t& message);
    bool _updateStatus(LinkInterface* link, const SharedLinkInterfacePtr linkPtr, uint8_t mavlinkChannel,
                       const mavlink_message_t& message);

    void _saveTelemetryLog(const QString& tempLogfile);
    bool _checkTelemetrySavePath();

    QFile* _tempLogFile = nullptr;

    bool _logSuspendError = false;
    bool _logSuspendReplay = false;
    bool _vehicleWasArmed = false;

    uint8_t _lastIndex[256][256]{};  ///< Store the last received sequence ID for each system/component pair

    QSet<QPair<uint8_t, uint8_t>> _firstMessageSeen[MAVLINK_COMM_NUM_BUFFERS];
    uint64_t _totalReceiveCounter[MAVLINK_COMM_NUM_BUFFERS]{};
    uint64_t _totalLossCounter[MAVLINK_COMM_NUM_BUFFERS]{};
    float _runningLossPercent[MAVLINK_COMM_NUM_BUFFERS]{};

    bool _initialized = false;

    static constexpr const char* _tempLogFileTemplate = "FlightDataXXXXXX";
    static constexpr const char* _logFileExtension = "mavlink";

    static constexpr uint8_t kMaxCompId = MAV_COMPONENT_ENUM_END - 1;
};
