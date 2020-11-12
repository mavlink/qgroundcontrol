/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "VehicleClockFactGroup.h"
#include "Vehicle.h"

const char* VehicleClockFactGroup::_currentTimeFactName = "currentTime";
const char* VehicleClockFactGroup::_currentDateFactName = "currentDate";

VehicleClockFactGroup::VehicleClockFactGroup(QObject* parent)
    : FactGroup(1000, ":/json/Vehicle/ClockFact.json", parent)
    , _currentTimeFact  (0, _currentTimeFactName,    FactMetaData::valueTypeString)
    , _currentDateFact  (0, _currentDateFactName,    FactMetaData::valueTypeString)
{
    _addFact(&_currentTimeFact, _currentTimeFactName);
    _addFact(&_currentDateFact, _currentDateFactName);

    // Start out as not available "--.--"
    _currentTimeFact.setRawValue(std::numeric_limits<float>::quiet_NaN());
    _currentDateFact.setRawValue(std::numeric_limits<float>::quiet_NaN());
}

void VehicleClockFactGroup::_updateAllValues()
{
    _currentTimeFact.setRawValue(QTime::currentTime().toString());
    _currentDateFact.setRawValue(QDateTime::currentDateTime().toString(QLocale::system().dateFormat(QLocale::ShortFormat)));
    _setTelemetryAvailable(true);

    FactGroup::_updateAllValues();
}
