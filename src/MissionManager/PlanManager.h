/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QObject>
#include <QLoggingCategory>
#include <QTimer>

#include "MissionItem.h"
#include "QGCMAVLink.h"
#include "QGCLoggingCategory.h"
#include "LinkInterface.h"

class Vehicle;
class MissionCommandTree;

Q_DECLARE_LOGGING_CATEGORY(PlanManagerLog)

/// The PlanManager class is the base class for the Mission, GeoFence and Rally Point managers. All of which use the
/// new mavlink v2 mission protocol.
class PlanManager : public QObject
{
    Q_OBJECT

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
    void progressPct                (double progressPercentPct);
    void removeAllComplete          (bool error);
    void sendComplete               (bool error);
    void resumeMissionReady         (void);
    void resumeMissionUploadFail    (void);

private slots:
    void _mavlinkMessageReceived(const mavlink_message_t& message);
    void _ackTimeout(void);

protected:
    typedef enum {
        AckNone,            ///< State machine is idle
        AckMissionCount,    ///< MISSION_COUNT message expected
        AckMissionItem,     ///< MISSION_ITEM expected
        AckMissionRequest,  ///< MISSION_REQUEST is expected, or MISSION_ACK to end sequence
        AckMissionClearAll, ///< MISSION_CLEAR_ALL sent, MISSION_ACK is expected
        AckGuidedItem,      ///< MISSION_ACK expected in response to ArduPilot guided mode single item send
    } AckType_t;

    typedef enum {
        TransactionNone,
        TransactionRead,
        TransactionWrite,
        TransactionRemoveAll
    } TransactionType_t;

    void _startAckTimeout(AckType_t ack);
    bool _checkForExpectedAck(AckType_t receivedAck);
    void _readTransactionComplete(void);
    void _handleMissionCount(const mavlink_message_t& message);
    void _handleMissionItem(const mavlink_message_t& message);
    void _handleMissionRequest(const mavlink_message_t& message);
    void _handleMissionAck(const mavlink_message_t& message);
    void _requestNextMissionItem(void);
    void _clearMissionItems(void);
    void _sendError(ErrorCode_t errorCode, const QString& errorMsg);
    QString _ackTypeToString(AckType_t ackType);
    QString _missionResultToString(MAV_MISSION_RESULT result);
    void _finishTransaction(bool success, bool apmGuidedItemWrite = false);
    void _requestList(void);
    void _writeMissionCount(void);
    void _writeMissionItemsWorker(void);
    void _clearAndDeleteMissionItems(void);
    void _clearAndDeleteWriteMissionItems(void);
    QString _lastMissionReqestString(MAV_MISSION_RESULT result);
    void _removeAllWorker(void);
    void _connectToMavlink(void);
    void _disconnectFromMavlink(void);
    QString _planTypeString(void);

protected:
    Vehicle*            _vehicle =              nullptr;
    MissionCommandTree* _missionCommandTree =   nullptr;
    MAV_MISSION_TYPE    _planType;

    QTimer*             _ackTimeoutTimer =      nullptr;
    AckType_t           _expectedAck;
    int                 _retryCount;

    TransactionType_t   _transactionInProgress;
    bool                _resumeMission;
    QList<int>          _itemIndicesToWrite;    ///< List of mission items which still need to be written to vehicle
    QList<int>          _itemIndicesToRead;     ///< List of mission items which still need to be requested from vehicle
    int                 _lastMissionRequest;    ///< Index of item last requested by MISSION_REQUEST
    int                 _missionItemCountToRead;///< Count of all mission items to read

    QList<MissionItem*> _missionItems;          ///< Set of mission items on vehicle
    QList<MissionItem*> _writeMissionItems;     ///< Set of mission items currently being written to vehicle
    int                 _currentMissionIndex;
    int                 _lastCurrentIndex;

private:
    void _setTransactionInProgress(TransactionType_t type);
};
