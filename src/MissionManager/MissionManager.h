/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
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

#ifndef MissionManager_H
#define MissionManager_H

#include <QObject>
#include <QLoggingCategory>
#include <QThread>
#include <QMutex>
#include <QTimer>

#include "QmlObjectListModel.h"
#include "QGCMAVLink.h"
#include "QGCLoggingCategory.h"
#include "LinkInterface.h"

class Vehicle;

Q_DECLARE_LOGGING_CATEGORY(MissionManagerLog)

class MissionManager : public QObject
{
    Q_OBJECT
    
public:
    /// @param uas Uas which this set of facts is associated with
    MissionManager(Vehicle* vehicle);
    ~MissionManager();
    
    Q_PROPERTY(bool                 inProgress      READ inProgress     NOTIFY inProgressChanged)
    Q_PROPERTY(QmlObjectListModel*  missionItems    READ missionItems   CONSTANT)
    
    // Property accessors
    
    bool inProgress(void);
    QmlObjectListModel* missionItems(void) { return &_missionItems; }
    
    // C++ methods
    
    void requestMissionItems(void);
    
    /// Writes the specified set of mission items to the vehicle
    ///     @oaram missionItems Items to send to vehicle
    void writeMissionItems(const QmlObjectListModel& missionItems);
    
    /// Returns a copy of the current set of mission items. Caller is responsible for
    /// freeing returned object.
    QmlObjectListModel* copyMissionItems(void);
    
    /// Error codes returned in error signal
    typedef enum {
        InternalError,
        AckTimeoutError,        ///< Timed out waiting for response from vehicle
        ProtocolOrderError,     ///< Incorrect protocol sequence from vehicle
        RequestRangeError,      ///< Vehicle requested item out of range
        ItemMismatchError,      ///< Vehicle returned item with seq # different than requested
        VehicleError,           ///< Vehicle returned error
        MissingRequestsError,   ///< Vehicle did not request all items during write sequence
        MaxRetryExceeded,       ///< Retry failed
    } ErrorCode_t;

    // These values are public so the unit test can set appropriate signal wait times
    static const int _ackTimeoutMilliseconds= 2000;
    static const int _maxRetryCount = 5;
    
signals:
    void newMissionItemsAvailable(void);
    void inProgressChanged(bool inProgress);
    void error(int errorCode, const QString& errorMsg);
    
private slots:
    void _mavlinkMessageReceived(const mavlink_message_t& message);
    void _ackTimeout(void);
    
private:
    typedef enum {
        AckNone,            ///< State machine is idle
        AckMissionCount,    ///< MISSION_COUNT message expected
        AckMissionItem,     ///< MISSION_ITEM expected
        AckMissionRequest,  ///< MISSION_REQUEST is expected, or MISSION_ACK to end sequence
    } AckType_t;
    
    void _startAckTimeout(AckType_t ack);
    bool _stopAckTimeout(AckType_t expectedAck);
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
    void _finishTransaction(bool success);

private:
    Vehicle*            _vehicle;
    LinkInterface*      _dedicatedLink;
    
    QTimer*             _ackTimeoutTimer;
    AckType_t           _retryAck;
    int                 _requestItemRetryCount;
    
    bool        _readTransactionInProgress;
    bool        _writeTransactionInProgress;
    QList<int>  _itemIndicesToWrite;    ///< List of mission items which still need to be written to vehicle
    QList<int>  _itemIndicesToRead;     ///< List of mission items which still need to be requested from vehicle
    
    QMutex _dataMutex;
    
    QmlObjectListModel  _missionItems;
};

#endif
