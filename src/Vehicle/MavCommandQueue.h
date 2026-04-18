#pragma once

#include <functional>

#include <QtCore/QElapsedTimer>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QTimer>

#include "VehicleTypes.h"

class Vehicle;

/// Owns the COMMAND_LONG / COMMAND_INT send/retry/ack pipeline for a single Vehicle.
///
/// Each outbound command is appended to a per-vehicle queue with its retry policy
/// and ack timeout. A periodic timer resends entries whose ack window has expired;
/// incoming COMMAND_ACK messages are matched by (compId, command) and either complete
/// the entry or refresh its timer on MAV_RESULT_IN_PROGRESS.
class MavCommandQueue : public QObject, public VehicleTypes
{
    Q_OBJECT

public:
    explicit MavCommandQueue(Vehicle* vehicle);
    ~MavCommandQueue();

    // Public send API — mirrors Vehicle's historical sendMavCommand* surface.
    void sendCommand           (int compId, MAV_CMD command, bool showError, float param1 = 0.0f, float param2 = 0.0f, float param3 = 0.0f, float param4 = 0.0f, float param5 = 0.0f, float param6 = 0.0f, float param7 = 0.0f);
    void sendCommandDelayed    (int compId, MAV_CMD command, bool showError, int milliseconds, float param1 = 0.0f, float param2 = 0.0f, float param3 = 0.0f, float param4 = 0.0f, float param5 = 0.0f, float param6 = 0.0f, float param7 = 0.0f);
    void sendCommandInt        (int compId, MAV_CMD command, MAV_FRAME frame, bool showError, float param1, float param2, float param3, float param4, double param5, double param6, float param7);

    void sendCommandWithHandler    (const MavCmdAckHandlerInfo_t* ackHandlerInfo, int compId, MAV_CMD command, float param1 = 0.0f, float param2 = 0.0f, float param3 = 0.0f, float param4 = 0.0f, float param5 = 0.0f, float param6 = 0.0f, float param7 = 0.0f);
    void sendCommandIntWithHandler (const MavCmdAckHandlerInfo_t* ackHandlerInfo, int compId, MAV_CMD command, MAV_FRAME frame, float param1 = 0.0f, float param2 = 0.0f, float param3 = 0.0f, float param4 = 0.0f, double param5 = 0.0, double param6 = 0.0, float param7 = 0.0f);

    void sendCommandWithLambdaFallback(std::function<void()> lambda, int compId, MAV_CMD command, bool showError, float param1 = 0.0f, float param2 = 0.0f, float param3 = 0.0f, float param4 = 0.0f, float param5 = 0.0f, float param6 = 0.0f, float param7 = 0.0f);

    /// Full-control entry point used by higher-level coordinators (e.g. RequestMessageCoordinator)
    /// that need to set commandInt/frame/handlers explicitly. Consumers should prefer the
    /// typed send* wrappers above.
    void sendWorker(bool commandInt, bool showError, const MavCmdAckHandlerInfo_t* ackHandlerInfo,
                    int compId, MAV_CMD command, MAV_FRAME frame,
                    float param1, float param2, float param3, float param4, double param5, double param6, float param7);

    /// True if a matching (targetCompId, command) is already queued or awaiting ack.
    bool isPending(int targetCompId, MAV_CMD command) const;

    /// Index of a matching entry in the pending queue, or -1. Exposed for test use.
    int findEntryIndex(int targetCompId, MAV_CMD command) const;

    /// Process a COMMAND_ACK — match it to a pending entry and fire callbacks.
    void handleCommandAck(const mavlink_message_t& message, const mavlink_command_ack_t& ack);

    /// Stop the response timer and clear pending entries without firing callbacks.
    /// Used during vehicle shutdown to prevent post-destruction callbacks.
    void stop();

    static QString failureCodeToString(MavCmdResultFailureCode_t failureCode);
    static void showCommandAckError(const mavlink_command_ack_t& ack);

    // Test tuning knobs.
    static constexpr int kTestAckTimeoutMs = 500;
    static constexpr int kMaxRetryCount = 3;
    static constexpr int kTestMaxWaitMs = kTestAckTimeoutMs * kMaxRetryCount * 2;

signals:
    /// Emitted for every terminal ack that has no user-provided resultHandler.
    void commandResult(int vehicleId, int targetComponent, int command, int ackResult, int failureCode);

private slots:
    void _responseTimeoutCheck();

private:
    typedef struct MavCommandListEntry {
        int                     targetCompId        = 0;
        bool                    useCommandInt       = false;
        MAV_CMD                 command;
        MAV_FRAME               frame;
        float                   rgParam1            = 0;
        float                   rgParam2            = 0;
        float                   rgParam3            = 0;
        float                   rgParam4            = 0;
        double                  rgParam5            = 0;
        double                  rgParam6            = 0;
        float                   rgParam7            = 0;
        bool                    showError           = true;
        MavCmdAckHandlerInfo_t  ackHandlerInfo;
        int                     maxTries            = kMaxRetryCount;
        int                     tryCount            = 0;
        QElapsedTimer           elapsedTimer;
        int                     ackTimeoutMSecs     = 0;
    } MavCommandListEntry_t;

    void _sendFromList(int index);
    static bool _shouldRetry(MAV_CMD command);
    static bool _canBeDuplicated(MAV_CMD command);
    static int _responseCheckIntervalMSecs();
    static int _ackTimeoutMSecs();
    static QString _formatCommand(MAV_CMD command, float param1);

    Vehicle*                         _vehicle = nullptr;
    QList<MavCommandListEntry_t>     _list;
    QTimer                           _responseCheckTimer;
    bool                             _stopped = false;  // set by stop(), gates all send paths

    static constexpr int _ackTimeoutMSecsHighLatency = 120000;
};
