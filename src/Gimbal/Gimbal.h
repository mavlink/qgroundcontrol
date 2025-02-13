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
    Q_PROPERTY(Fact *absoluteRoll               READ absoluteRoll               CONSTANT)
    Q_PROPERTY(Fact *absolutePitch              READ absolutePitch              CONSTANT)
    Q_PROPERTY(Fact *bodyYaw                    READ bodyYaw                    CONSTANT)
    Q_PROPERTY(Fact *absoluteYaw                READ absoluteYaw                CONSTANT)
    Q_PROPERTY(Fact *deviceId                   READ deviceId                   CONSTANT)
    Q_PROPERTY(Fact *managerCompid              READ managerCompid              CONSTANT)
    Q_PROPERTY(float pitchRate                  READ pitchRate                  CONSTANT)
    Q_PROPERTY(float yawRate                    READ yawRate                    CONSTANT)
    Q_PROPERTY(bool  yawLock                    READ yawLock                    NOTIFY yawLockChanged)
    Q_PROPERTY(bool  retracted                  READ retracted                  NOTIFY retractedChanged)
    Q_PROPERTY(bool  gimbalHaveControl          READ gimbalHaveControl          NOTIFY gimbalHaveControlChanged)
    Q_PROPERTY(bool  gimbalOthersHaveControl    READ gimbalOthersHaveControl    NOTIFY gimbalOthersHaveControlChanged)

    friend class GimbalController; // so it can set private members of gimbal, it is the only class that will need to modify them

public:
    Gimbal(GimbalController *parent);
    Gimbal(const Gimbal &other);
    const Gimbal& operator=(const Gimbal &other);

    Fact *absoluteRoll() const { return &_absoluteRollFact; }
    Fact *absolutePitch() const { return &_absolutePitchFact; }
    Fact *bodyYaw() const { return &_bodyYawFact; }
    Fact *absoluteYaw() const { return &_absoluteYawFact; }
    Fact *deviceId() const { return &_deviceIdFact; }
    Fact *managerCompid() const { return &_managerCompidFact; }
    float pitchRate() const { return _pitchRate; }
    float yawRate() const { return _yawRate; }
    bool  yawLock() const { return _yawLock; }
    bool  retracted() const { return _retracted; }
    bool  gimbalHaveControl() const { return _haveControl; }
    bool  gimbalOthersHaveControl() const { return _othersHaveControl; }

    void  setAbsoluteRoll(float absoluteRoll) { _absoluteRollFact.setRawValue(absoluteRoll); }
    void  setAbsolutePitch(float absolutePitch) { _absolutePitchFact.setRawValue(absolutePitch); }
    void  setBodyYaw(float bodyYaw) { _bodyYawFact.setRawValue(bodyYaw); }
    void  setAbsoluteYaw(float absoluteYaw) { _absoluteYawFact.setRawValue(absoluteYaw); }
    void  setDeviceId(uint id) { _deviceIdFact.setRawValue(id); }
    void  setManagerCompid(uint id) { _managerCompidFact.setRawValue(id); }
    void  setPitchRate(float pitchRate) { _pitchRate = pitchRate; }
    void  setYawRate(float yawRate) { _yawRate = yawRate; }
    void  setYawLock(bool yawLock) { _yawLock = yawLock; emit yawLockChanged(); }
    void  setRetracted(bool retracted) { _retracted = retracted; emit retractedChanged(); }
    void  setGimbalHaveControl(bool set) { _haveControl = set; emit gimbalHaveControlChanged(); }
    void  setGimbalOthersHaveControl(bool set) { _othersHaveControl = set; emit gimbalOthersHaveControlChanged(); }

signals:
    void yawLockChanged();
    void retractedChanged();
    void gimbalHaveControlChanged();
    void gimbalOthersHaveControlChanged();

private:
    void _initFacts(); // To be called EXCLUSIVELY in Gimbal constructors

    // Private members only accesed by friend class GimbalController
    unsigned _requestInformationRetries = 3;
    unsigned _requestStatusRetries = 6;
    unsigned _requestAttitudeRetries = 3;
    bool _receivedInformation = false;
    bool _receivedStatus = false;
    bool _receivedAttitude = false;
    bool _isComplete = false;
    bool _neutral = false;

    // Q_PROPERTIES
    Fact _absoluteRollFact;
    Fact _absolutePitchFact;
    Fact _bodyYawFact;
    Fact _absoluteYawFact;
    Fact _deviceIdFact; // Component ID of gimbal device (or 1-6 for non-MAVLink gimbal)
    Fact _managerCompidFact;
    float _pitchRate = 0.f;
    float _yawRate = 0.f;
    bool _yawLock = false;
    bool _retracted = false;
    bool _haveControl = false;
    bool _othersHaveControl = false;

    // Fact names
    static constexpr const char *_absoluteRollFactName = "gimbalRoll";
    static constexpr const char *_absolutePitchFactName = "gimbalPitch";
    static constexpr const char *_bodyYawFactName = "gimbalYaw";
    static constexpr const char *_absoluteYawFactName = "gimbalAzimuth";
    static constexpr const char *_deviceIdFactName = "deviceId";
    static constexpr const char *_managerCompidFactName = "managerCompid";
};
