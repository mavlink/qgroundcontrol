/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "VehicleClockFactGroup.h"
#include "QGCApplication.h"

VehicleClockFactGroup::VehicleClockFactGroup(QObject *parent)
    : FactGroup(1000, QStringLiteral(":/json/Vehicle/ClockFact.json"), parent)
{
    _addFact(&_currentTimeFact);
    _addFact(&_currentUTCTimeFact);
    _addFact(&_currentDateFact);

    _currentTimeFact.setRawValue(QTime().toString());
    _currentUTCTimeFact.setRawValue(std::numeric_limits<float>::quiet_NaN());
    _currentDateFact.setRawValue(std::numeric_limits<float>::quiet_NaN());
}

void VehicleClockFactGroup::_updateAllValues()
{
    currentTime()->setRawValue(QTime::currentTime().toString());
    currentUTCTime()->setRawValue(QDateTime::currentDateTimeUtc().time().toString());
    currentDate()->setRawValue(QDateTime::currentDateTime().toString(qgcApp()->getCurrentLanguage().dateFormat(QLocale::ShortFormat)));

    _setTelemetryAvailable(true);

    FactGroup::_updateAllValues();
}
