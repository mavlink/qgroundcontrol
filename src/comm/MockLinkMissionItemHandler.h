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

#ifndef MockLinkMissionItemHandler_H
#define MockLinkMissionItemHandler_H

#include <QObject>
#include <QMap>

#include "QGCMAVLink.h"
#include "QGCLoggingCategory.h"

class MockLink;

Q_DECLARE_LOGGING_CATEGORY(MockLinkMissionItemHandlerLog)

class MockLinkMissionItemHandler : public QObject
{
    Q_OBJECT

public:
    MockLinkMissionItemHandler(MockLink* mockLink);

    /// @brief Called to handle mission item related messages. All messages should be passed to this method.
    ///         It will handle the appropriate set.
    /// @return true: message handled
    bool handleMessage(const mavlink_message_t& msg);

private:
    void _handleMissionRequestList(const mavlink_message_t& msg);
    void _handleMissionRequest(const mavlink_message_t& msg);
    void _handleMissionItem(const mavlink_message_t& msg);
    void _handleMissionCount(const mavlink_message_t& msg);
    void _requestNextMissionItem(int sequenceNumber);

private:
    MockLink* _mockLink;
    
    int _writeSequenceCount;    ///< Numbers of items about to be written
    int _writeSequenceIndex;    ///< Current index being reqested
    
    typedef QMap<uint16_t, mavlink_mission_item_t>   MissionList_t;
    MissionList_t   _missionItems;
};

#endif
