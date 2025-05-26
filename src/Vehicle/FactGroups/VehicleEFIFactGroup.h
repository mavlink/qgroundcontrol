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

class VehicleEFIFactGroup : public FactGroup
{
    Q_OBJECT
    Q_PROPERTY(Fact *health         READ health         CONSTANT)
    Q_PROPERTY(Fact *ecuIndex       READ ecuIndex       CONSTANT)
    Q_PROPERTY(Fact *rpm            READ rpm            CONSTANT)
    Q_PROPERTY(Fact *fuelConsumed   READ fuelConsumed   CONSTANT)
    Q_PROPERTY(Fact *fuelFlow       READ fuelFlow       CONSTANT)
    Q_PROPERTY(Fact *engineLoad     READ engineLoad     CONSTANT)
    Q_PROPERTY(Fact *throttlePos    READ throttlePos    CONSTANT)
    Q_PROPERTY(Fact *sparkTime      READ sparkTime      CONSTANT)
    Q_PROPERTY(Fact *baroPress      READ baroPress      CONSTANT)
    Q_PROPERTY(Fact *intakePress    READ intakePress    CONSTANT)
    Q_PROPERTY(Fact *intakeTemp     READ intakeTemp     CONSTANT)
    Q_PROPERTY(Fact *cylinderTemp   READ cylinderTemp   CONSTANT)
    Q_PROPERTY(Fact *ignTime        READ ignTime        CONSTANT)
    Q_PROPERTY(Fact *injTime        READ injTime        CONSTANT)
    Q_PROPERTY(Fact *exGasTemp      READ exGasTemp      CONSTANT)
    Q_PROPERTY(Fact *throttleOut    READ throttleOut    CONSTANT)
    Q_PROPERTY(Fact *ptComp         READ ptComp         CONSTANT)
    Q_PROPERTY(Fact *ignVoltage     READ ignVoltage     CONSTANT)

public:
    explicit VehicleEFIFactGroup(QObject *parent = nullptr);

    Fact *health() { return &_healthFact; }
    Fact *ecuIndex() { return &_ecuIndexFact; }
    Fact *rpm() { return &_rpmFact; }
    Fact *fuelConsumed() { return &_fuelConsumedFact; }
    Fact *fuelFlow() { return &_fuelFlowFact; }
    Fact *engineLoad() { return &_engineLoadFact; }
    Fact *throttlePos() { return &_throttlePosFact; }
    Fact *sparkTime() { return &_sparkTimeFact; }
    Fact *baroPress() { return &_baroPressFact; }
    Fact *intakePress() { return &_intakePressFact; }
    Fact *intakeTemp() { return &_intakeTempFact; }
    Fact *cylinderTemp() { return &_cylinderTempFact; }
    Fact *ignTime() { return &_ignTimeFact; }
    Fact *injTime() { return &_injTimeFact; }
    Fact *exGasTemp() { return &_exGasTempFact; }
    Fact *throttleOut() { return &_throttleOutFact; }
    Fact *ptComp() { return &_ptCompFact; }
    Fact *ignVoltage() { return &_ignVoltageFact; }

    // Overrides from FactGroup
    void handleMessage(Vehicle *vehicle, const mavlink_message_t &message) final;

private:
    void _handleEFIStatus(const mavlink_message_t &message);

    Fact _healthFact = Fact(0, QStringLiteral("health"), FactMetaData::valueTypeInt8);
    Fact _ecuIndexFact = Fact(0, QStringLiteral("ecuIndex"), FactMetaData::valueTypeFloat);
    Fact _rpmFact = Fact(0, QStringLiteral("rpm"), FactMetaData::valueTypeFloat);
    Fact _fuelConsumedFact = Fact(0, QStringLiteral("fuelConsumed"), FactMetaData::valueTypeFloat);
    Fact _fuelFlowFact = Fact(0, QStringLiteral("fuelFlow"), FactMetaData::valueTypeFloat);
    Fact _engineLoadFact = Fact(0, QStringLiteral("engineLoad"), FactMetaData::valueTypeFloat);
    Fact _throttlePosFact = Fact(0, QStringLiteral("throttlePos"), FactMetaData::valueTypeFloat);
    Fact _sparkTimeFact = Fact(0, QStringLiteral("sparkTime"), FactMetaData::valueTypeFloat);
    Fact _baroPressFact = Fact(0, QStringLiteral("baroPress"), FactMetaData::valueTypeFloat);
    Fact _intakePressFact = Fact(0, QStringLiteral("intakePress"), FactMetaData::valueTypeFloat);
    Fact _intakeTempFact = Fact(0, QStringLiteral("intakeTemp"), FactMetaData::valueTypeFloat);
    Fact _cylinderTempFact = Fact(0, QStringLiteral("cylinderTemp"), FactMetaData::valueTypeFloat);
    Fact _ignTimeFact = Fact(0, QStringLiteral("ignTime"), FactMetaData::valueTypeFloat);
    Fact _injTimeFact = Fact(0, QStringLiteral("injTime"), FactMetaData::valueTypeFloat);
    Fact _exGasTempFact = Fact(0, QStringLiteral("exGasTemp"), FactMetaData::valueTypeFloat);
    Fact _throttleOutFact = Fact(0, QStringLiteral("throttleOut"), FactMetaData::valueTypeFloat);
    Fact _ptCompFact = Fact(0, QStringLiteral("ptComp"), FactMetaData::valueTypeFloat);
    Fact _ignVoltageFact = Fact(0, QStringLiteral("ignVoltage"), FactMetaData::valueTypeFloat);
};
