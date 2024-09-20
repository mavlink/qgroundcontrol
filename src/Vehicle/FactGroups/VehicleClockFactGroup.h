/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "FactGroup.h"
#include "QGCMAVLink.h"

class Vehicle;

class VehicleClockFactGroup : public FactGroup
{
    Q_OBJECT

public:
    VehicleClockFactGroup(QObject* parent = nullptr);

    Q_PROPERTY(Fact* currentTime READ currentTime CONSTANT)
    Q_PROPERTY(Fact* currentUTCTime READ currentUTCTime CONSTANT)
    Q_PROPERTY(Fact* currentDate READ currentDate CONSTANT)

    Fact* currentTime () { return &_currentTimeFact; }
    Fact* currentUTCTime () { return &_currentUTCTimeFact; }
    Fact* currentDate () { return &_currentDateFact; }



private slots:
    void _updateAllValues() override;

private:
    const QString _currentTimeFactName = QStringLiteral("currentTime");
    const QString _currentUTCTimeFactName = QStringLiteral("currentUTCTime");
    const QString _currentDateFactName = QStringLiteral("currentDate");

    Fact            _currentTimeFact;
    Fact            _currentUTCTimeFact;
    Fact            _currentDateFact;
};
