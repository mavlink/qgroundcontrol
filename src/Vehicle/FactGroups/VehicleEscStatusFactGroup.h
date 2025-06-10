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
    Q_PROPERTY(Fact *count          READ count          CONSTANT)
    Q_PROPERTY(Fact *info           READ info           CONSTANT)
    Q_PROPERTY(Fact *rpmFirst       READ rpmFirst       CONSTANT)
    Q_PROPERTY(Fact *rpmSecond      READ rpmSecond      CONSTANT)
    Q_PROPERTY(Fact *rpmThird       READ rpmThird       CONSTANT)
    Q_PROPERTY(Fact *rpmFourth      READ rpmFourth      CONSTANT)
    Q_PROPERTY(Fact *rpmFifth       READ rpmFifth       CONSTANT)
    Q_PROPERTY(Fact *rpmSixth       READ rpmSixth       CONSTANT)
    Q_PROPERTY(Fact *rpmSeventh     READ rpmSeventh     CONSTANT)
    Q_PROPERTY(Fact *rpmEighth      READ rpmEighth      CONSTANT)
    Q_PROPERTY(Fact *currentFirst   READ currentFirst   CONSTANT)
    Q_PROPERTY(Fact *currentSecond  READ currentSecond  CONSTANT)
    Q_PROPERTY(Fact *currentThird   READ currentThird   CONSTANT)
    Q_PROPERTY(Fact *currentFourth  READ currentFourth  CONSTANT)
    Q_PROPERTY(Fact *currentFifth   READ currentFifth   CONSTANT)
    Q_PROPERTY(Fact *currentSixth   READ currentSixth   CONSTANT)
    Q_PROPERTY(Fact *currentSeventh READ currentSeventh CONSTANT)
    Q_PROPERTY(Fact *currentEighth  READ currentEighth  CONSTANT)
    Q_PROPERTY(Fact *voltageFirst   READ voltageFirst   CONSTANT)
    Q_PROPERTY(Fact *voltageSecond  READ voltageSecond  CONSTANT)
    Q_PROPERTY(Fact *voltageThird   READ voltageThird   CONSTANT)
    Q_PROPERTY(Fact *voltageFourth  READ voltageFourth  CONSTANT)
    Q_PROPERTY(Fact *voltageFifth   READ voltageFifth   CONSTANT)
    Q_PROPERTY(Fact *voltageSixth   READ voltageSixth   CONSTANT)
    Q_PROPERTY(Fact *voltageSeventh READ voltageSeventh CONSTANT)
    Q_PROPERTY(Fact *voltageEighth  READ voltageEighth  CONSTANT)
    Q_PROPERTY(Fact *temperatureFirst   READ temperatureFirst   CONSTANT)
    Q_PROPERTY(Fact *temperatureSecond  READ temperatureSecond  CONSTANT)
    Q_PROPERTY(Fact *temperatureThird   READ temperatureThird   CONSTANT)
    Q_PROPERTY(Fact *temperatureFourth  READ temperatureFourth  CONSTANT)
    Q_PROPERTY(Fact *temperatureFifth   READ temperatureFifth   CONSTANT)
    Q_PROPERTY(Fact *temperatureSixth   READ temperatureSixth   CONSTANT)
    Q_PROPERTY(Fact *temperatureSeventh READ temperatureSeventh CONSTANT)
    Q_PROPERTY(Fact *temperatureEighth  READ temperatureEighth  CONSTANT)
    Q_PROPERTY(Fact *errorCountFirst    READ errorCountFirst    CONSTANT)
    Q_PROPERTY(Fact *errorCountSecond   READ errorCountSecond   CONSTANT)
    Q_PROPERTY(Fact *errorCountThird    READ errorCountThird    CONSTANT)
    Q_PROPERTY(Fact *errorCountFourth   READ errorCountFourth   CONSTANT)
    Q_PROPERTY(Fact *errorCountFifth    READ errorCountFifth    CONSTANT)
    Q_PROPERTY(Fact *errorCountSixth    READ errorCountSixth    CONSTANT)
    Q_PROPERTY(Fact *errorCountSeventh  READ errorCountSeventh  CONSTANT)
    Q_PROPERTY(Fact *errorCountEighth   READ errorCountEighth   CONSTANT)

public:
    explicit VehicleEscStatusFactGroup(QObject *parent = nullptr);

    Fact *index() { return &_indexFact; }
    Fact *count() { return &_countFact; }
    Fact *info() { return &_infoFact; }
    Fact *rpmFirst() { return &_rpmFirstFact; }
    Fact *rpmSecond() { return &_rpmSecondFact; }
    Fact *rpmThird() { return &_rpmThirdFact; }
    Fact *rpmFourth() { return &_rpmFourthFact; }
    Fact *rpmFifth() { return &_rpmFifthFact; }
    Fact *rpmSixth() { return &_rpmSixthFact; }
    Fact *rpmSeventh() { return &_rpmSeventhFact; }
    Fact *rpmEighth() { return &_rpmEighthFact; }

    Fact *currentFirst() { return &_currentFirstFact; }
    Fact *currentSecond() { return &_currentSecondFact; }
    Fact *currentThird() { return &_currentThirdFact; }
    Fact *currentFourth() { return &_currentFourthFact; }
    Fact *currentFifth() { return &_currentFifthFact; }
    Fact *currentSixth() { return &_currentSixthFact; }
    Fact *currentSeventh() { return &_currentSeventhFact; }
    Fact *currentEighth() { return &_currentEighthFact; }

    Fact *voltageFirst() { return &_voltageFirstFact; }
    Fact *voltageSecond() { return &_voltageSecondFact; }
    Fact *voltageThird() { return &_voltageThirdFact; }
    Fact *voltageFourth() { return &_voltageFourthFact; }
    Fact *voltageFifth() { return &_voltageFifthFact; }
    Fact *voltageSixth() { return &_voltageSixthFact; }
    Fact *voltageSeventh() { return &_voltageSeventhFact; }
    Fact *voltageEighth() { return &_voltageEighthFact; }

    Fact *temperatureFirst() { return &_temperatureFirstFact; }
    Fact *temperatureSecond() { return &_temperatureSecondFact; }
    Fact *temperatureThird() { return &_temperatureThirdFact; }
    Fact *temperatureFourth() { return &_temperatureFourthFact; }
    Fact *temperatureFifth() { return &_temperatureFifthFact; }
    Fact *temperatureSixth() { return &_temperatureSixthFact; }
    Fact *temperatureSeventh() { return &_temperatureSeventhFact; }
    Fact *temperatureEighth() { return &_temperatureEighthFact; }

    Fact *errorCountFirst() { return &_errorCountFirstFact; }
    Fact *errorCountSecond() { return &_errorCountSecondFact; }
    Fact *errorCountThird() { return &_errorCountThirdFact; }
    Fact *errorCountFourth() { return &_errorCountFourthFact; }
    Fact *errorCountFifth() { return &_errorCountFifthFact; }
    Fact *errorCountSixth() { return &_errorCountSixthFact; }
    Fact *errorCountSeventh() { return &_errorCountSeventhFact; }
    Fact *errorCountEighth() { return &_errorCountEighthFact; }

    // Overrides from FactGroup
    void handleMessage(Vehicle *vehicle, const mavlink_message_t &message) final;

private:
    Fact _indexFact = Fact(0, QStringLiteral("index"), FactMetaData::valueTypeUint8);
    Fact _countFact = Fact(0, QStringLiteral("count"), FactMetaData::valueTypeUint8);
    Fact _infoFact = Fact(0, QStringLiteral("info"), FactMetaData::valueTypeUint64);
    Fact _rpmFirstFact = Fact(0, QStringLiteral("rpm1"), FactMetaData::valueTypeFloat);
    Fact _rpmSecondFact = Fact(0, QStringLiteral("rpm2"), FactMetaData::valueTypeFloat);
    Fact _rpmThirdFact = Fact(0, QStringLiteral("rpm3"), FactMetaData::valueTypeFloat);
    Fact _rpmFourthFact = Fact(0, QStringLiteral("rpm4"), FactMetaData::valueTypeFloat);
    Fact _rpmFifthFact = Fact(0, QStringLiteral("rpm5"), FactMetaData::valueTypeFloat);
    Fact _rpmSixthFact = Fact(0, QStringLiteral("rpm6"), FactMetaData::valueTypeFloat);
    Fact _rpmSeventhFact = Fact(0, QStringLiteral("rpm7"), FactMetaData::valueTypeFloat);
    Fact _rpmEighthFact = Fact(0, QStringLiteral("rpm8"), FactMetaData::valueTypeFloat);

    Fact _currentFirstFact = Fact(0, QStringLiteral("current1"), FactMetaData::valueTypeFloat);
    Fact _currentSecondFact = Fact(0, QStringLiteral("current2"), FactMetaData::valueTypeFloat);
    Fact _currentThirdFact = Fact(0, QStringLiteral("current3"), FactMetaData::valueTypeFloat);
    Fact _currentFourthFact = Fact(0, QStringLiteral("current4"), FactMetaData::valueTypeFloat);
    Fact _currentFifthFact = Fact(0, QStringLiteral("current5"), FactMetaData::valueTypeFloat);
    Fact _currentSixthFact = Fact(0, QStringLiteral("current6"), FactMetaData::valueTypeFloat);
    Fact _currentSeventhFact = Fact(0, QStringLiteral("current7"), FactMetaData::valueTypeFloat);
    Fact _currentEighthFact = Fact(0, QStringLiteral("current8"), FactMetaData::valueTypeFloat);

    Fact _voltageFirstFact = Fact(0, QStringLiteral("voltage1"), FactMetaData::valueTypeFloat);
    Fact _voltageSecondFact = Fact(0, QStringLiteral("voltage2"), FactMetaData::valueTypeFloat);
    Fact _voltageThirdFact = Fact(0, QStringLiteral("voltage3"), FactMetaData::valueTypeFloat);
    Fact _voltageFourthFact = Fact(0, QStringLiteral("voltage4"), FactMetaData::valueTypeFloat);
    Fact _voltageFifthFact = Fact(0, QStringLiteral("voltage5"), FactMetaData::valueTypeFloat);
    Fact _voltageSixthFact = Fact(0, QStringLiteral("voltage6"), FactMetaData::valueTypeFloat);
    Fact _voltageSeventhFact = Fact(0, QStringLiteral("voltage7"), FactMetaData::valueTypeFloat);
    Fact _voltageEighthFact = Fact(0, QStringLiteral("voltage8"), FactMetaData::valueTypeFloat);

    Fact _temperatureFirstFact = Fact(0, QStringLiteral("temperature1"), FactMetaData::valueTypeFloat);
    Fact _temperatureSecondFact = Fact(0, QStringLiteral("temperature2"), FactMetaData::valueTypeFloat);
    Fact _temperatureThirdFact = Fact(0, QStringLiteral("temperature3"), FactMetaData::valueTypeFloat);
    Fact _temperatureFourthFact = Fact(0, QStringLiteral("temperature4"), FactMetaData::valueTypeFloat);
    Fact _temperatureFifthFact = Fact(0, QStringLiteral("temperature5"), FactMetaData::valueTypeFloat);
    Fact _temperatureSixthFact = Fact(0, QStringLiteral("temperature6"), FactMetaData::valueTypeFloat);
    Fact _temperatureSeventhFact = Fact(0, QStringLiteral("temperature7"), FactMetaData::valueTypeFloat);
    Fact _temperatureEighthFact = Fact(0, QStringLiteral("temperature8"), FactMetaData::valueTypeFloat);

    Fact _errorCountFirstFact = Fact(0, QStringLiteral("errorCount1"), FactMetaData::valueTypeUint32);
    Fact _errorCountSecondFact = Fact(0, QStringLiteral("errorCount2"), FactMetaData::valueTypeUint32);
    Fact _errorCountThirdFact = Fact(0, QStringLiteral("errorCount3"), FactMetaData::valueTypeUint32);
    Fact _errorCountFourthFact = Fact(0, QStringLiteral("errorCount4"), FactMetaData::valueTypeUint32);
    Fact _errorCountFifthFact = Fact(0, QStringLiteral("errorCount5"), FactMetaData::valueTypeUint32);
    Fact _errorCountSixthFact = Fact(0, QStringLiteral("errorCount6"), FactMetaData::valueTypeUint32);
    Fact _errorCountSeventhFact = Fact(0, QStringLiteral("errorCount7"), FactMetaData::valueTypeUint32);
    Fact _errorCountEighthFact = Fact(0, QStringLiteral("errorCount8"), FactMetaData::valueTypeUint32);
};
