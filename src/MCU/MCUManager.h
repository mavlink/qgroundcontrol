/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#pragma once

#include <QGCToolbox.h>

#include <QString>
#include <QObject>

#include "MAVLinkProtocol.h"

Q_DECLARE_LOGGING_CATEGORY(MCUManagerLog)

/**
 ** class MCUManager
 */
class MCUManager : public QGCTool
{
    Q_OBJECT
public:
    MCUManager(QGCApplication* app, QGCToolbox* toolbox);

    Q_PROPERTY(int    percentRemaining    READ    percentRemaining    NOTIFY    percentRemainingChanged)

    // Override from QGCTool
    virtual void setToolbox(QGCToolbox *toolbox);

    int percentRemaining(void) { return _percentRemaining; }
    
signals:
    void percentRemainingChanged(int percentRemaining);

private:
    void _mavlinkMessageReceived(LinkInterface* link, mavlink_message_t message);
    void _handleMcuData(mavlink_message_t& message);

    MAVLinkProtocol*        _mavlinkProtocol;
    int _percentRemaining;

};

#ifndef HAVE_ENUM_MCU_TYPE
#define HAVE_ENUM_MCU_TYPE
typedef enum MCU_TYPE
{
    MCU_TYPE_MONITOR_REQUEST = 0x01,
    MCU_TYPE_MONITOR_RESPONSE = 0x02,
    MCU_TYPE_JOYSTICK_MODE_SET = 0x03,
    MCU_TYPE_JOYSTICK_MODE_RESPONSE = 0x04,
    MCU_TYPE_BUZZER_ENABLE_SET = 0x05,
    MCU_TYPE_BUZZER_ENABLE_RESPONSE = 0x06,
    MCU_TYPE_KNOB_CALI_REQUEST = 0x07,
    MCU_TYPE_KNOB_CALI_RESPONSE = 0x08,
    MCU_TYPE_VERSION_REQUEST = 0x09,
    MCU_TYPE_VERSION_RESPONSE = 0x0A,
    MCU_TYPE_CAMERA_MODEL_SET = 0x0B,
    MCU_TYPE_CAMERA_MODEL_RESPONSE = 0x0C,
    MCU_TYPE_COACH_MODE_NOTIFY = 0x0D,
    MCU_TYPE_SETTINGS_SYNC = 0x0E,
    MCU_TYPE_COACH_ENABLE_SET = 0x0F,
    MCU_TYPE_COACH_ENABLE_RESPONSE = 0x10,
    MCU_TYPE_GS_SMART_BATTERY_INFO = 0x11,
    MCU_TYPE_WARNING_SOUND_REQUEST = 0x13,
    MCU_TYPE_DRONE_CALIBRATE_REQUEST = 0x14,
    MCU_TYPE_EXTERNAL_MONITOR_REQUEST = 0x15,
    MCU_TYPE_EXTERNAL_MONITOR_RESPONSE1 = 0x16,
    MCU_TYPE_EXTERNAL_MONITOR_RESPONSE2 = 0x17,
    MCU_TYPE_NOTICE_DIALOG = 0x18,
    MCU_TYPE_JOYSTICK_CALIBRATE_REQUEST = 0x19,
    MCU_TYPE_JOYSTICK_CALIBRATE_RESPONSE = 0x20,
} MCU_TYPE;
#endif

#ifndef HAVE_ENUM_MCU_UPGRADE_TYPE
#define HAVE_ENUM_MCU_UPGRADE_TYPE
typedef enum MCU_UPGRADE_TYPE
{
    MCU_UPGRADE_TYPE_NONE = 0,
    MCU_UPGRADE_TYPE_START_UPGRADE_BOOT = 1,
    MCU_UPGRADE_TYPE_END_UPGRADE_BOOT = 2,
    MCU_UPGRADE_TYPE_START_UPGRADE_APP = 3,
    MCU_UPGRADE_TYPE_END_UPGRADE_APP = 4,
    MCU_UPGRADE_TYPE_RESPONSE_OK = 5,
    MCU_UPGRADE_TYPE_RESPONSE_ERROR = 6,
    MCU_UPGRADE_TYPE_SENDING_BOOT = 7,
    MCU_UPGRADE_TYPE_SENDING_APP = 8,
} MCU_UPGRADE_TYPE;
#endif

#ifndef HAVE_ENUM_MCU_NOTICE_TYPE
#define HAVE_ENUM_MCU_NOTICE_TYPE
typedef enum MCU_NOTICE_TYPE
{
    MCU_NOTICE_COACH_PORT_ABNORMAL = 1,
    MCU_NOTICE_EMERGENCY_BACKUP_LINK = 2,
    MCU_NOTICE_NORMAL_LINK = 3,
} MCU_NOTICE_TYPE;
#endif