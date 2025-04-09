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

class VehicleEscStatusFactGroup : public FactGroup
{
    Q_OBJECT
    Q_PROPERTY(Fact *index          READ index          CONSTANT)
    Q_PROPERTY(Fact *rpmFirst       READ rpmFirst       CONSTANT)
    Q_PROPERTY(Fact *rpmSecond      READ rpmSecond      CONSTANT)
    Q_PROPERTY(Fact *rpmThird       READ rpmThird       CONSTANT)
    Q_PROPERTY(Fact *rpmFourth      READ rpmFourth      CONSTANT)
    Q_PROPERTY(Fact *currentFirst   READ currentFirst   CONSTANT)
    Q_PROPERTY(Fact *currentSecond  READ currentSecond  CONSTANT)
    Q_PROPERTY(Fact *currentThird   READ currentThird   CONSTANT)
    Q_PROPERTY(Fact *currentFourth  READ currentFourth  CONSTANT)
    Q_PROPERTY(Fact *voltageFirst   READ voltageFirst   CONSTANT)
    Q_PROPERTY(Fact *voltageSecond  READ voltageSecond  CONSTANT)
    Q_PROPERTY(Fact *voltageThird   READ voltageThird   CONSTANT)
    Q_PROPERTY(Fact *voltageFourth  READ voltageFourth  CONSTANT)

public:
    explicit VehicleEscStatusFactGroup(QObject *parent = nullptr);

    Fact *index() { return &_indexFact; }
    Fact *rpmFirst() { return &_rpmFirstFact; }
    Fact *rpmSecond() { return &_rpmSecondFact; }
    Fact *rpmThird() { return &_rpmThirdFact; }
    Fact *rpmFourth() { return &_rpmFourthFact; }

    Fact *currentFirst() { return &_currentFirstFact; }
    Fact *currentSecond() { return &_currentSecondFact; }
    Fact *currentThird() { return &_currentThirdFact; }
    Fact *currentFourth() { return &_currentFourthFact; }

    Fact *voltageFirst() { return &_voltageFirstFact; }
    Fact *voltageSecond() { return &_voltageSecondFact; }
    Fact *voltageThird() { return &_voltageThirdFact; }
    Fact *voltageFourth() { return &_voltageFourthFact; }

    // Overrides from FactGroup
    void handleMessage(Vehicle *vehicle, const mavlink_message_t &message) final;

private:
    Fact _indexFact = Fact(0, QStringLiteral("index"), FactMetaData::valueTypeUint8);
    Fact _rpmFirstFact = Fact(0, QStringLiteral("rpm1"), FactMetaData::valueTypeFloat);
    Fact _rpmSecondFact = Fact(0, QStringLiteral("rpm2"), FactMetaData::valueTypeFloat);
    Fact _rpmThirdFact = Fact(0, QStringLiteral("rpm3"), FactMetaData::valueTypeFloat);
    Fact _rpmFourthFact = Fact(0, QStringLiteral("rpm4"), FactMetaData::valueTypeFloat);

    Fact _currentFirstFact = Fact(0, QStringLiteral("current1"), FactMetaData::valueTypeFloat);
    Fact _currentSecondFact = Fact(0, QStringLiteral("current2"), FactMetaData::valueTypeFloat);
    Fact _currentThirdFact = Fact(0, QStringLiteral("current3"), FactMetaData::valueTypeFloat);
    Fact _currentFourthFact = Fact(0, QStringLiteral("current4"), FactMetaData::valueTypeFloat);

    Fact _voltageFirstFact = Fact(0, QStringLiteral("voltage1"), FactMetaData::valueTypeFloat);
    Fact _voltageSecondFact = Fact(0, QStringLiteral("voltage2"), FactMetaData::valueTypeFloat);
    Fact _voltageThirdFact = Fact(0, QStringLiteral("voltage3"), FactMetaData::valueTypeFloat);
    Fact _voltageFourthFact = Fact(0, QStringLiteral("voltage4"), FactMetaData::valueTypeFloat);
};
