/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "PlanManager.h"

Q_DECLARE_LOGGING_CATEGORY(MissionManagerLog)

class MissionManager : public PlanManager
{
    Q_OBJECT
    
public:
    MissionManager(Vehicle* vehicle);
    ~MissionManager();
        
    /// Current mission item as reported by MISSION_CURRENT
    int currentIndex(void) const { return _currentMissionIndex; }

    /// Last current mission item reported while in Mission flight mode
    int lastCurrentIndex(void) const { return _lastCurrentIndex; }

    /// Writes the specified set mission items to the vehicle as an ArduPilot guided mode mission item.
    ///     @param gotoCoord Coordinate to move to
    ///     @param altChangeOnly true: only altitude change, false: lat/lon/alt change
    void writeArduPilotGuidedMissionItem(const QGeoCoordinate& gotoCoord, bool altChangeOnly);

    /// Generates a new mission which starts from the specified index. It will include all the CMD_DO items
    /// from mission start to resumeIndex in the generate mission.
    void generateResumeMission(int resumeIndex);

private slots:
    void _mavlinkMessageReceived(const mavlink_message_t& message);

private:
    void _handleHighLatency(const mavlink_message_t& message);
    void _handleHighLatency2(const mavlink_message_t& message);
    void _handleMissionCurrent(const mavlink_message_t& message);
    void _updateMissionIndex(int index);
    void _handleHeartbeat(const mavlink_message_t& message);

    int _cachedLastCurrentIndex;
};
