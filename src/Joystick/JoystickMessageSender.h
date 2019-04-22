/****************************************************************************
 *
 * Copyright (C) 2018 Pinecone Inc. All rights reserved.
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "QGCLoggingCategory.h"
#include "Joystick.h"

class JoystickManager;
class UDPLink;

Q_DECLARE_LOGGING_CATEGORY(JoystickMessageSenderLog)

class JoystickMessageSender : public QObject
{
    Q_OBJECT

public:
    JoystickMessageSender(JoystickManager* joystickManager);
    ~JoystickMessageSender();

    Q_PROPERTY(int channelCount READ channelCount CONSTANT)
    Q_PROPERTY(QVariantList sbusChannelStatus READ sbusChannelStatus NOTIFY sbusChannelStatusChanged)

    static int getChannelValue(int sbus, int ch);

    void setChannelValue(int sbus, int ch, uint16_t value);
    int channelCount();
    QVariantList sbusChannelStatus();

signals:
    void sbusChannelStatusChanged();

private slots:
    void _handleManualControl(float roll, float pitch, float yaw, float thrust, float wheel, quint16 buttons, int joystickMode);
    void _activeJoystickChanged(Joystick* joystick);
    void _setupJoystickLink();

private:
    Joystick* _activeJoystick;
    JoystickManager* _joystickManager;
    UDPLink* _udpLink;
    uint _joystickPortNumber;
    uint8_t _joystickCompId;
    QString _remoteHostIp;
    int _mavlinkChannel;
    int _channelCount;
    static uint16_t _sbus0ChannelValues[12];//sbus0
    static uint16_t _sbus1ChannelValues[16];//sbus1
};
