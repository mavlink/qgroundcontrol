#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QCache>
#include <QtCore/QObject>
#include <QtCore/QPair>
#include <QtCore/QString>

#include "LinkInterface.h"
#include "MAVLinkEnums.h"
#include "MAVLinkMessageType.h"

class QFile;

/// \brief MAVLink micro air vehicle protocol reference implementation.
///
class MAVLinkProtocol : public QObject
{
    Q_OBJECT

public:
    explicit MAVLinkProtocol(QObject* parent = nullptr);

    ~MAVLinkProtocol();

    static MAVLinkProtocol* instance();

    void init();

    static QString getName() { return QStringLiteral("MAVLink protocol"); }

    quint32 getSystemId() const;

    static int getComponentId() { return MAV_COMP_ID_MISSIONPLANNER; }

    void resetMetadataForLink(LinkInterface* link);

    /// Reset sequence tracking so signing transitions don't inflate loss counters.
    void resetSequenceTracking(LinkInterface* link);

    void suspendLogForReplay(bool suspend) { _logSuspendReplay = suspend; }

    /// Removes telemetry logs left behind by a crash. A log is renamed to .tlog as soon as it qualifies for
    /// saving, so one still in progress never qualified.
    static void removeOrphanedLogFiles();

    void setLoggingEnabledForTest(bool enabled) { _loggingEnabledForTest = enabled; }

signals:
    void vehicleHeartbeatInfo(LinkInterface* link, quint32 vehicleId, int componentId, int vehicleFirmwareType,
                              int vehicleType);

    void messageReceived(LinkInterface* link, const mavlink_message_t& message);

    void mavlinkMessageStatus(quint32 sysid, uint64_t totalSent, uint64_t totalReceived, uint64_t totalLoss,
                              float lossPercent);

public slots:
    void receiveBytes(LinkInterface* link, const QByteArray& data);

    void logSentBytes(const LinkInterface* link, const QByteArray& data);

private slots:
    void _vehicleCountChanged();

private:
    void _logData(LinkInterface* link, const mavlink_message_t& message);
    bool _closeLogFile();
    void _startLogging();
    void _stopLogging();
    bool _logFileInProgress() const;
    /// @return true if the log should be saved under the current settings and armed state
    bool _keepLog() const;
    /// Renames the in-progress log as soon as it qualifies for saving, so that it survives a crash without
    /// any recovery.
    void _promoteLogFileIfKept();
    /// Renames the in-progress log to its final .tlog name, leaving it open if it was open.
    /// @return true if the log has its final name
    bool _promoteLogFile();

    void _forward(const mavlink_message_t& message);
    void _forwardSupport(const mavlink_message_t& message);

    void _updateCounters(uint8_t mavlinkChannel, const mavlink_message_t& message);
    bool _updateStatus(LinkInterface* link, const SharedLinkInterfacePtr linkPtr, uint8_t mavlinkChannel,
                       const mavlink_message_t& message);

    /// @return What is wrong with saveDirPath as the telemetry log save path, empty if it is usable
    static QString _telemetrySavePathError(const QString& saveDirPath);

    QFile* _logFile = nullptr;

    bool _logSuspendError = false;      ///< Logging failed and stays off until the last vehicle is gone
    bool _logStartErrorShown = false;
    bool _logKeepCheckPending = false;  ///< The log has not been checked for saving since it opened
    bool _logPromoteFailed = false;
    bool _logFinalizePending = false;   ///< A log was opened and not yet saved or deleted by _stopLogging()
    bool _loggingEnabledForTest = false;
    bool _logSuspendReplay = false;
    bool _vehicleWasArmed = false;

    /// Per-(channel, sysid, compid) last sequence ID. Channel-scoped so traffic on link A doesn't perturb expected
    /// sequence on link B (which has independent sequence histories from the same vehicle).
    QCache<QPair<quint32, uint8_t>, uint8_t> _lastIndex[MAVLINK_COMM_NUM_BUFFERS];
    uint64_t _totalReceiveCounter[MAVLINK_COMM_NUM_BUFFERS]{};
    uint64_t _totalLossCounter[MAVLINK_COMM_NUM_BUFFERS]{};
    float _runningLossPercent[MAVLINK_COMM_NUM_BUFFERS]{};

    bool _initialized = false;

    /// A log is recorded in the telemetry save directory as "<recording start time>.tlog.inprogress".
    static constexpr const char* _logStartTimeFormat = "yyyy-MM-dd hh-mm-ss";
    static constexpr const char* _inProgressLogSuffix = "inprogress";

    static constexpr uint8_t kMaxCompId = MAV_COMPONENT_ENUM_END - 1;
};
