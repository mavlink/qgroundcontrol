/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "Gimbal.h"
#include "GimbalController.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GimbalLog, "qgc.gimbal.gimbal")

Gimbal::Gimbal(GimbalController *parent)
    : FactGroup(1000, QStringLiteral(":/json/Vehicle/GimbalFact.json"), parent)
{
    // qCDebug(GimbalLog) << Q_FUNC_INFO << this;

    _initFacts();
}

Gimbal::Gimbal(const Gimbal &other)
    : FactGroup(1000, QStringLiteral(":/json/Vehicle/GimbalFact.json"), other.parent())
{
    _initFacts();
    *this = other;
}

Gimbal::~Gimbal()
{
    // qCDebug(GimbalLog) << Q_FUNC_INFO << this;
}

const Gimbal &Gimbal::operator=(const Gimbal &other)
{
    _requestInformationRetries = other._requestInformationRetries;
    _requestStatusRetries = other._requestStatusRetries;
    _requestAttitudeRetries = other._requestAttitudeRetries;
    _receivedInformation = other._receivedInformation;
    _receivedStatus = other._receivedStatus;
    _receivedAttitude = other._receivedAttitude;
    _isComplete = other._isComplete;
    _retracted = other._retracted;
    _neutral = other._neutral;
    _haveControl = other._haveControl;
    _othersHaveControl = other._othersHaveControl;
    _absoluteRollFact = other._absoluteRollFact;
    _absolutePitchFact = other._absolutePitchFact;
    _bodyYawFact = other._bodyYawFact;
    _absoluteYawFact = other._absoluteYawFact;
    _deviceIdFact = other._deviceIdFact;
    _yawLock = other._yawLock;
    _haveControl = other._haveControl;
    _othersHaveControl = other._othersHaveControl;

    return *this;
}

void Gimbal::_initFacts()
{
    _addFact(&_absoluteRollFact);
    _addFact(&_absolutePitchFact);
    _addFact(&_bodyYawFact);
    _addFact(&_absoluteYawFact);
    _addFact(&_deviceIdFact);
    _addFact(&_managerCompidFact);

    _absoluteRollFact.setRawValue(0.0f);
    _absolutePitchFact.setRawValue(0.0f);
    _bodyYawFact.setRawValue(0.0f);
    _absoluteYawFact.setRawValue(0.0f);
    _deviceIdFact.setRawValue(0);
    _managerCompidFact.setRawValue(0);
}
