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

class VehicleClockFactGroup : public FactGroup
{
    Q_OBJECT
    Q_PROPERTY(Fact *currentTime    READ currentTime    CONSTANT)
    Q_PROPERTY(Fact *currentUTCTime READ currentUTCTime CONSTANT)
    Q_PROPERTY(Fact *currentDate    READ currentDate    CONSTANT)

public:
    explicit VehicleClockFactGroup(QObject *parent = nullptr);

    Fact *currentTime() { return &_currentTimeFact; }
    Fact *currentUTCTime() { return &_currentUTCTimeFact; }
    Fact *currentDate() { return &_currentDateFact; }

private slots:
    void _updateAllValues() final;

private:
    Fact _currentTimeFact = Fact(0, QStringLiteral("currentTime"), FactMetaData::valueTypeString);
    Fact _currentUTCTimeFact = Fact(0, QStringLiteral("currentUTCTime"), FactMetaData::valueTypeString);
    Fact _currentDateFact = Fact(0, QStringLiteral("currentDate"), FactMetaData::valueTypeString);
};
