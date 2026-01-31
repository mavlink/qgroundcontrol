#pragma once

#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtCore/QLoggingCategory>

#include "MissionItem.h"
#include "QGCMAVLink.h"

class Vehicle;
class PlanManagerStateMachine;

Q_DECLARE_LOGGING_CATEGORY(PlanManagerLog)

/// The PlanManager class is the base class for the Mission, GeoFence and Rally Point managers. All of which use the
/// new mavlink v2 mission protocol.
class PlanManager : public QObject
{
    Q_OBJECT
    friend class PlanManagerStateMachine;

public:
    PlanManager(Vehicle* vehicle, MAV_MISSION_TYPE planType);
    ~PlanManager();

    bool inProgress(void) const;
    const QList<MissionItem*>& missionItems(void) { return _missionItems; }

    /// Current mission item as reported by MISSION_CURRENT
    int currentIndex(void) const { return _currentMissionIndex; }

    /// Last current mission item reported while in Mission flight mode
    int lastCurrentIndex(void) const { return _lastCurrentIndex; }

    /// Load the mission items from the vehicle
    ///     Signals newMissionItemsAvailable when done
    void loadFromVehicle(void);

    /// Writes the specified set of mission items to the vehicle
    /// IMPORTANT NOTE: PlanManager will take control of the MissionItem objects with the missionItems list. It will free them when done.
    ///     @param missionItems Items to send to vehicle
    ///     Signals sendComplete when done
    void writeMissionItems(const QList<MissionItem*>& missionItems);

    /// Removes all mission items from vehicle
    ///     Signals removeAllComplete when done
    void removeAll(void);

    /// Error codes returned in error signal
    typedef enum {
        InternalError,
        AckTimeoutError,        ///< Timed out waiting for response from vehicle
        ProtocolError,          ///< Incorrect protocol sequence from vehicle
        RequestRangeError,      ///< Vehicle requested item out of range
        ItemMismatchError,      ///< Vehicle returned item with seq # different than requested
        VehicleAckError,        ///< Vehicle returned error in ack
        MissingRequestsError,   ///< Vehicle did not request all items during write sequence
        MaxRetryExceeded,       ///< Retry failed
        MissionTypeMismatch,    ///< MAV_MISSION_TYPE does not match _planType
    } ErrorCode_t;

    // These values are public so the unit test can set appropriate signal wait times
    // When passively waiting for a mission process, use a longer timeout.
    static const int _ackTimeoutMilliseconds = 1500;
    // When actively retrying to request mission items, use a shorter timeout instead.
    static const int _retryTimeoutMilliseconds = 250;
    static const int _maxRetryCount = 5;

signals:
    void newMissionItemsAvailable   (bool removeAllRequested);
    void inProgressChanged          (bool inProgress);
    void error                      (int errorCode, const QString& errorMsg);
    void currentIndexChanged        (int currentIndex);
    void lastCurrentIndexChanged    (int lastCurrentIndex);
    void progressPctChanged         (double progressPercentPct);
    void removeAllComplete          (bool error);
    void sendComplete               (bool error);
    void resumeMissionReady         (void);
    void resumeMissionUploadFail    (void);

private slots:
    void _mavlinkMessageReceived(const mavlink_message_t& message);

protected:
    void _sendError(ErrorCode_t errorCode, const QString& errorMsg);
    QString _missionResultToString(MAV_MISSION_RESULT result);
    void _clearAndDeleteMissionItems(void);
    void _clearAndDeleteWriteMissionItems(void);
    QString _lastMissionReqestString(MAV_MISSION_RESULT result);
    void _connectToMavlink(void);
    void _disconnectFromMavlink(void);
    QString _planTypeString(void);

    // Called by state machine on transaction completion
    void _onReadComplete(bool success);
    void _onWriteComplete(bool success);
    void _onRemoveAllComplete(bool success);

protected:
    Vehicle*            _vehicle =              nullptr;
    MAV_MISSION_TYPE    _planType;

    bool                _resumeMission = false;
    int                 _lastMissionRequest = -1;    ///< Index of item last requested by MISSION_REQUEST

    QList<MissionItem*> _missionItems;          ///< Set of mission items on vehicle
    QList<MissionItem*> _writeMissionItems;     ///< Set of mission items currently being written to vehicle
    int                 _currentMissionIndex = -1;
    int                 _lastCurrentIndex = -1;

    PlanManagerStateMachine* _stateMachine = nullptr;
};
