/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QLoggingCategory>

#include "FactGroup.h"

Q_DECLARE_LOGGING_CATEGORY(GimbalLog)

class GimbalController;

class Gimbal : public FactGroup
{
    Q_OBJECT
    Q_PROPERTY(Fact     *absoluteRoll           READ absoluteRoll               CONSTANT)
    Q_PROPERTY(Fact     *absolutePitch          READ absolutePitch              CONSTANT)
    Q_PROPERTY(Fact     *bodyYaw                READ bodyYaw                    CONSTANT)
    Q_PROPERTY(Fact     *absoluteYaw            READ absoluteYaw                CONSTANT)
    Q_PROPERTY(Fact     *deviceId               READ deviceId                   CONSTANT)
    Q_PROPERTY(Fact     *managerCompid          READ managerCompid              CONSTANT)
    Q_PROPERTY(float    pitchRate               READ pitchRate                  NOTIFY pitchRateChanged)
    Q_PROPERTY(float    yawRate                 READ yawRate                    NOTIFY yawRateChanged)
    Q_PROPERTY(bool     yawLock                 READ yawLock                    NOTIFY yawLockChanged)
    Q_PROPERTY(bool     retracted               READ retracted                  NOTIFY retractedChanged)
    Q_PROPERTY(bool     gimbalHaveControl       READ gimbalHaveControl          NOTIFY gimbalHaveControlChanged)
    Q_PROPERTY(bool     gimbalOthersHaveControl READ gimbalOthersHaveControl    NOTIFY gimbalOthersHaveControlChanged)

    friend class GimbalController;

public:
    Gimbal(GimbalController *parent);
    Gimbal(const Gimbal &other);
    const Gimbal &operator=(const Gimbal &other);
    ~Gimbal();

    Fact *absoluteRoll() { return &_absoluteRollFact; }
    Fact *absolutePitch() { return &_absolutePitchFact; }
    Fact *bodyYaw() { return &_bodyYawFact; }
    Fact *absoluteYaw() { return &_absoluteYawFact; }
    Fact *deviceId() { return &_deviceIdFact; }
    Fact *managerCompid() { return &_managerCompidFact; }

    float pitchRate() const { return _pitchRate; }
    float yawRate() const { return _yawRate; }
    bool yawLock() const { return _yawLock; }
    bool retracted() const { return _retracted; }
    bool gimbalHaveControl() const { return _haveControl; }
    bool gimbalOthersHaveControl() const { return _othersHaveControl; }

    void setAbsoluteRoll(float absRoll) { absoluteRoll()->setRawValue(absRoll); }
    void setAbsolutePitch(float absPitch) { absolutePitch()->setRawValue(absPitch); }
    void setBodyYaw(float yaw) { bodyYaw()->setRawValue(yaw); }
    void setAbsoluteYaw(float absYaw) { absoluteYaw()->setRawValue(absYaw); }
    void setDeviceId(uint id) { deviceId()->setRawValue(id); }
    void setManagerCompid(uint id) { managerCompid()->setRawValue(id); }

    void setPitchRate(float pitchRate) { if (pitchRate != _pitchRate) { _pitchRate = pitchRate; emit pitchRateChanged(); } }
    void setYawRate(float yawRate) { if (yawRate != _yawRate) { _yawRate = yawRate; emit yawRateChanged(); } }
    void setYawLock(bool yawLock) { if (yawLock != _yawLock) { _yawLock = yawLock; emit yawLockChanged(); } }
    void setRetracted(bool retracted) { if (retracted != _retracted) { _retracted = retracted; emit retractedChanged(); } }
    void setGimbalHaveControl(bool set) { if (set != _haveControl) { _haveControl = set; emit gimbalHaveControlChanged(); } }
    void setGimbalOthersHaveControl(bool set) { if (set != _othersHaveControl) { _othersHaveControl = set; emit gimbalOthersHaveControlChanged(); } }

signals:
    void pitchRateChanged();
    void yawRateChanged();
    void yawLockChanged();
    void retractedChanged();
    void gimbalHaveControlChanged();
    void gimbalOthersHaveControlChanged();

private:
    void _initFacts();

    unsigned _requestInformationRetries = 3;
    unsigned _requestStatusRetries = 6;
    unsigned _requestAttitudeRetries = 3;
    bool _receivedInformation = false;
    bool _receivedStatus = false;
    bool _receivedAttitude = false;
    bool _isComplete = false;
    bool _neutral = false;

    Fact _absoluteRollFact = Fact(0, QStringLiteral("gimbalRoll"), FactMetaData::valueTypeFloat);
    Fact _absolutePitchFact = Fact(0, QStringLiteral("gimbalPitch"), FactMetaData::valueTypeFloat);
    Fact _bodyYawFact = Fact(0, QStringLiteral("gimbalYaw"), FactMetaData::valueTypeFloat);
    Fact _absoluteYawFact = Fact(0, QStringLiteral("gimbalAzimuth"), FactMetaData::valueTypeFloat);
    Fact _deviceIdFact = Fact(0, QStringLiteral("deviceId"), FactMetaData::valueTypeUint8); ///< Component ID of gimbal device (or 1-6 for non-MAVLink gimbal)
    Fact _managerCompidFact = Fact(0, QStringLiteral("managerCompid"), FactMetaData::valueTypeUint8);

    float _pitchRate = 0.f;
    float _yawRate = 0.f;
    bool _yawLock = false;
    bool _retracted = false;
    bool _haveControl = false;
    bool _othersHaveControl = false;
};
