/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "GimbalController.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GimbalLog, "qgc.gimbal.gimbal")

Gimbal::Gimbal(GimbalController *parent)
    : FactGroup(100, parent, QStringLiteral(":/json/Vehicle/GimbalFact.json"))
{
    _initFacts();
}

Gimbal::Gimbal(const Gimbal &other)
    : FactGroup(100, other.parent(), ":/json/Vehicle/GimbalFact.json")
{
    _initFacts();
    *this = other;
}

const Gimbal& Gimbal::operator=(const Gimbal &other)
{
    _requestInformationRetries = other._requestInformationRetries;
    _requestStatusRetries      = other._requestStatusRetries;
    _requestAttitudeRetries    = other._requestAttitudeRetries;
    _receivedInformation       = other._receivedInformation;
    _receivedStatus            = other._receivedStatus;
    _receivedAttitude          = other._receivedAttitude;
    _isComplete                = other._isComplete;
    _retracted                 = other._retracted;
    _neutral                   = other._neutral;
    _haveControl               = other._haveControl;
    _othersHaveControl         = other._othersHaveControl;
    _absoluteRollFact          = other._absoluteRollFact;
    _absolutePitchFact         = other._absolutePitchFact;
    _bodyYawFact               = other._bodyYawFact;
    _absoluteYawFact           = other._absoluteYawFact;
    _deviceIdFact              = other._deviceIdFact;
    _yawLock                   = other._yawLock;
    _haveControl               = other._haveControl;
    _othersHaveControl         = other._othersHaveControl;

    return *this;
}

void Gimbal::_initFacts()
{
    _absoluteRollFact =     Fact(0, _absoluteRollFactName,  FactMetaData::valueTypeFloat);
    _absolutePitchFact =    Fact(0, _absolutePitchFactName, FactMetaData::valueTypeFloat);
    _bodyYawFact =          Fact(0, _bodyYawFactName,       FactMetaData::valueTypeFloat);
    _absoluteYawFact =      Fact(0, _absoluteYawFactName,   FactMetaData::valueTypeFloat);
    _deviceIdFact =         Fact(0, _deviceIdFactName,      FactMetaData::valueTypeUint8);
    _managerCompidFact =    Fact(0, _managerCompidFactName, FactMetaData::valueTypeUint8);

    _addFact(&_absoluteRollFact,    _absoluteRollFactName);
    _addFact(&_absolutePitchFact,   _absolutePitchFactName);
    _addFact(&_bodyYawFact,         _bodyYawFactName);
    _addFact(&_absoluteYawFact,     _absoluteYawFactName);
    _addFact(&_deviceIdFact,        _deviceIdFactName);
    _addFact(&_managerCompidFact,   _managerCompidFactName);

    _absoluteRollFact.setRawValue(0.0f);
    _absolutePitchFact.setRawValue(0.0f);
    _bodyYawFact.setRawValue(0.0f);
    _absoluteYawFact.setRawValue(0.0f);
    _deviceIdFact.setRawValue(0);
    _managerCompidFact.setRawValue(0);
}
