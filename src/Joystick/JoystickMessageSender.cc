/****************************************************************************
 *
 * Copyright (C) 2018 Pinecone Inc. All rights reserved.
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "JoystickManager.h"
#include "MAVLinkProtocol.h"
#include "QGCApplication.h"
#include "UDPLink.h"
#include "JoystickMessageSender.h"
#include "KeyConfiguration.h"

QGC_LOGGING_CATEGORY(JoystickMessageSenderLog, "JoystickMessageSenderLog")

uint16_t JoystickMessageSender::_sbus0ChannelValues[12] = { 0 };
uint16_t JoystickMessageSender::_sbus1ChannelValues[16] = { 0 };

JoystickMessageSender::JoystickMessageSender(JoystickManager* joystickManager)
    : QObject()
    , _activeJoystick(NULL)
    , _joystickManager(joystickManager)
    , _udpLink(NULL)
    , _joystickPortNumber(16666)
    , _joystickCompId(66)
    , _remoteHostIp("192.168.0.10")
    , _mavlinkChannel(0)
    , _channelCount(16)
{
    connect(joystickManager, &JoystickManager::activeJoystickChanged, this, &JoystickMessageSender::_activeJoystickChanged);
}

JoystickMessageSender::~JoystickMessageSender()
{
}

void JoystickMessageSender::_setupJoystickLink()
{
    UDPConfiguration* config = new UDPConfiguration(QString("Joystick"));
    config->setLocalPort(_joystickPortNumber);
    config->addHost(_remoteHostIp, _joystickPortNumber);
    LinkManager* linkMgr = qgcApp()->toolbox()->linkManager();
    config->setDynamic(true);
    SharedLinkConfigurationPointer linkConfig = linkMgr->addConfiguration(config);
    _udpLink = new UDPLink(linkConfig);
    _udpLink->_connect();

    _mavlinkChannel = qgcApp()->toolbox()->linkManager()->_reserveMavlinkChannel();
    if (_mavlinkChannel == 0) {
        qWarning() << "No mavlink channels available";
        return;
    }
    // use Mavlink 2.0
    mavlink_status_t* mavlinkStatus = mavlink_get_channel_status(_mavlinkChannel);
    mavlinkStatus->flags &= ~MAVLINK_STATUS_FLAG_OUT_MAVLINK1;
}

void JoystickMessageSender::_handleManualControl(float roll, float pitch, float yaw, float thrust, float wheel, quint16 buttons, int joystickMode)
{
    if (joystickMode != Vehicle::JoystickModeRC || _mavlinkChannel == 0) {
        return;
    }
    // Store the previous manual commands
    static float manualRollAngle = 0.0;
    static float manualPitchAngle = 0.0;
    static float manualYawAngle = 0.0;
    static float manualThrust = 0.0;
    static float manualWheel = 0.0;
    static quint16 manualButtons = 0;
    static quint8 countSinceLastTransmission = 0; // Track how many calls to this function have occurred since the last MAVLink transmission

    // Transmit the external setpoints only if they've changed OR if it's been a little bit since they were last transmit. To make sure there aren't issues with
    // response rate, we make sure that a message is transmit when the commands have changed, then one more time, and then switch to the lower transmission rate
    // if no command inputs have changed.

    // The default transmission rate is 25Hz, but when no inputs have changed it drops down to 5Hz.
    bool sendCommand = false;
    if (countSinceLastTransmission++ >= 5) {
        sendCommand = true;
        countSinceLastTransmission = 0;
    } else if ((!qIsNaN(roll) && roll != manualRollAngle) || (!qIsNaN(pitch) && pitch != manualPitchAngle) ||
             (!qIsNaN(yaw) && yaw != manualYawAngle) || (!qIsNaN(thrust) && thrust != manualThrust) ||
             (!qIsNaN(wheel) && wheel != manualWheel) || buttons != manualButtons) {
        sendCommand = true;

        // Ensure that another message will be sent the next time this function is called
        countSinceLastTransmission = 10;
    }

    if (!sendCommand) {
        return;
    }

    // Now if we should trigger an update, let's do that
    // Save the new manual control inputs
    manualRollAngle = roll;
    manualPitchAngle = pitch;
    manualYawAngle = yaw;
    manualThrust = thrust;
    manualThrust = wheel;
    manualButtons = buttons;

    int sbus, channel, channelValue;
    if(KeyConfiguration::getScrollWheelSetting(&sbus, &channel)) {
        channelValue = (int)(1000 + wheel * 1000.f);
        setChannelValue(sbus, channel, channelValue);
    }

    // Store scaling values for all 3 axes
    const float axesScaling = 1.0 * 1000.0;

    const float ch1 = (roll+1) * axesScaling;
    const float ch2 = (-pitch+1) * axesScaling;
    const float ch3 = (yaw+1) * axesScaling;
    const float ch4 = thrust * axesScaling;

#if 0
    uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
    mavlink_message_t message;
    MAVLinkProtocol* mavlink = qgcApp()->toolbox()->mavlinkProtocol();
    mavlink_msg_rc_channels_pack_chan(mavlink->getSystemId(), _joystickCompId,
                                 _mavlinkChannel,
                                 &message,
                                 0, 0,
                                 ch1, ch2, ch3, ch4,
                                 _sbus0ChannelValues[0], _sbus0ChannelValues[1],
                                 _sbus0ChannelValues[2], _sbus0ChannelValues[3],
                                 _sbus0ChannelValues[4], _sbus0ChannelValues[5],
                                 _sbus0ChannelValues[6], _sbus0ChannelValues[7],
                                 _sbus0ChannelValues[8], _sbus0ChannelValues[9],
                                 _sbus0ChannelValues[10], _sbus0ChannelValues[11],
                                 0, 0, 255);
    int len = mavlink_msg_to_send_buffer(buffer, &message);
    _udpLink->writeBytesSafe((const char*)buffer, len);

    mavlink_msg_rc_channels_pack_chan(mavlink->getSystemId(), _joystickCompId,
                                 _mavlinkChannel,
                                 &message,
                                 0, 1,
                                 _sbus1ChannelValues[0], _sbus1ChannelValues[1],
                                 _sbus1ChannelValues[2], _sbus1ChannelValues[3],
                                 _sbus1ChannelValues[4], _sbus1ChannelValues[5],
                                 _sbus1ChannelValues[6], _sbus1ChannelValues[7],
                                 _sbus1ChannelValues[8], _sbus1ChannelValues[9],
                                 _sbus1ChannelValues[10], _sbus1ChannelValues[11],
                                 _sbus1ChannelValues[12], _sbus1ChannelValues[13],
                                 _sbus1ChannelValues[14], _sbus1ChannelValues[15],
                                 0, 0, 255);

    len = mavlink_msg_to_send_buffer(buffer, &message);
    _udpLink->writeBytesSafe((const char*)buffer, len);
#endif
}

void JoystickMessageSender::_activeJoystickChanged(Joystick* joystick)
{
    if (_activeJoystick) {
        disconnect(_activeJoystick, &Joystick::manualControl, this, &JoystickMessageSender::_handleManualControl);
        _activeJoystick = NULL;
        if (_mavlinkChannel > 0) {
            qgcApp()->toolbox()->linkManager()->_freeMavlinkChannel(_mavlinkChannel);
        }
    }
    if (joystick) {
        if(_udpLink == NULL) {
            _setupJoystickLink();
        }
        _activeJoystick = joystick;
        connect(_activeJoystick, &Joystick::manualControl, this, &JoystickMessageSender::_handleManualControl);
    }
}

QVariantList JoystickMessageSender::sbusChannelStatus()
{
    KeyConfiguration* conf;
    QVariantList list;

    for(int i = 0; i < 2; i++) {
        QString status;
        conf = _joystickManager->getKeyConfiguration(i);
        for(int j = 0; j < conf->channelCount(); j++) {
            if(conf->getControlMode(j + conf->getChannelMinNum()) > 1) {
                status += QString("C%1: %2/%3 ").arg(j + conf->getChannelMinNum(), 2, 10, QChar('0'))
                                                .arg(conf->getSeqInChannel(j + conf->getChannelMinNum(), i == 0 ? _sbus0ChannelValues[j] : _sbus1ChannelValues[j]))
                                                .arg(conf->getChannelValueCount(j + conf->getChannelMinNum()));
            } else if(conf->getControlMode(j + conf->getChannelMinNum()) == 1) {//scroll wheel
                status += QString("C%1: ").arg(j + conf->getChannelMinNum(), 2, 10, QChar('0')) + "SW ";
            }
        }
        list += QVariant::fromValue(status);
    }
    return list;
}

int JoystickMessageSender::getChannelValue(int sbus, int ch)
{
    if(sbus == 1) {
        return _sbus0ChannelValues[ch - 5];
    } else if(sbus == 2) {
        return _sbus1ChannelValues[ch - 1];
    }

    return 0;
}

void JoystickMessageSender::setChannelValue(int sbus, int ch, uint16_t value)
{
    if (ch > _channelCount) {
        qWarning() << "invalid channel id" << ch;
        return;
    }
    if(sbus == 1) {
        _sbus0ChannelValues[ch - 5] = value;
    } else if(sbus == 2) {
        _sbus1ChannelValues[ch - 1] = value;
    }
    emit sbusChannelStatusChanged();
}

int JoystickMessageSender::channelCount()
{
    return _channelCount;
}
