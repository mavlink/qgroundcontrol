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
    Q_PROPERTY(bool                 canEdit         READ canEdit        NOTIFY  canEditChanged)
    
    // Property accessors
    
    bool inProgress(void) { return _retryAck != AckNone; }
    QmlObjectListModel* missionItems(void) { return &_missionItems; }
    bool canEdit(void) { return _canEdit; }
    
    // C++ methods
    
    void requestMissionItems(void);
    
    /// Writes the specified set of mission items to the vehicle
    void writeMissionItems(const QmlObjectListModel& missionItems);
    
    /// Returns a copy of the current set of mission items. Caller is responsible for
    /// freeing returned object.
    QmlObjectListModel* copyMissionItems(void);

signals:
    // Public signals
    void canEditChanged(bool canEdit);
    void newMissionItemsAvailable(void);
    void inProgressChanged(bool inProgress);
    
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
    
    void _startAckTimeout(AckType_t ack, const mavlink_message_t& message);
    bool _stopAckTimeout(AckType_t expectedAck);
    void _sendTransactionComplete(void);
    void _handleMissionCount(const mavlink_message_t& message);
    void _handleMissionItem(const mavlink_message_t& message);
    void _handleMissionRequest(const mavlink_message_t& message);
    void _handleMissionAck(const mavlink_message_t& message);
    void _requestNextMissionItem(int sequenceNumber);
    void _clearMissionItems(void);

private:
    Vehicle*            _vehicle;
    
    int                 _cMissionItems;     ///< Mission items on vehicle
    bool                _canEdit;           ///< true: Mission items are editable in the ui

    QTimer*             _ackTimeoutTimer;
    AckType_t           _retryAck;
    mavlink_message_t   _retryMessage;
    int                 _retryCount;
    
    int                 _expectedSequenceNumber;
    
    QMutex _dataMutex;
    
    QmlObjectListModel  _missionItems;
    
    static const int _ackTimeoutMilliseconds= 1000;
    static const int _maxRetryCount = 5;
};

#endif