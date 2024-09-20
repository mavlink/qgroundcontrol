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

class VehicleEFIFactGroup : public FactGroup
{
    Q_OBJECT

public:
    VehicleEFIFactGroup(QObject* parent = nullptr);

    Q_PROPERTY(Fact* health         READ health         CONSTANT)
    Q_PROPERTY(Fact* ecuIndex       READ ecuIndex       CONSTANT)
    Q_PROPERTY(Fact* rpm            READ rpm            CONSTANT)
    Q_PROPERTY(Fact* fuelConsumed   READ fuelConsumed   CONSTANT)
    Q_PROPERTY(Fact* fuelFlow       READ fuelFlow       CONSTANT)
    Q_PROPERTY(Fact* engineLoad     READ engineLoad     CONSTANT)
    Q_PROPERTY(Fact* throttlePos    READ throttlePos    CONSTANT)
    Q_PROPERTY(Fact* sparkTime      READ sparkTime      CONSTANT)
    Q_PROPERTY(Fact* baroPress      READ baroPress      CONSTANT)
    Q_PROPERTY(Fact* intakePress    READ intakePress    CONSTANT)
    Q_PROPERTY(Fact* intakeTemp     READ intakeTemp     CONSTANT)
    Q_PROPERTY(Fact* cylinderTemp   READ cylinderTemp   CONSTANT)
    Q_PROPERTY(Fact* ignTime        READ ignTime        CONSTANT)
    Q_PROPERTY(Fact* injTime        READ injTime        CONSTANT)
    Q_PROPERTY(Fact* exGasTemp      READ exGasTemp      CONSTANT)
    Q_PROPERTY(Fact* throttleOut    READ throttleOut    CONSTANT)
    Q_PROPERTY(Fact* ptComp         READ ptComp         CONSTANT)
    Q_PROPERTY(Fact* ignVoltage     READ ignVoltage     CONSTANT)

    Fact* health        () { return &_healthFact; }
    Fact* ecuIndex      () { return &_ecuIndexFact; }
    Fact* rpm           () { return &_rpmFact; }
    Fact* fuelConsumed  () { return &_fuelConsumedFact; }
    Fact* fuelFlow      () { return &_fuelFlowFact; }
    Fact* engineLoad    () { return &_engineLoadFact; }
    Fact* throttlePos   () { return &_throttlePosFact; }
    Fact* sparkTime     () { return &_sparkTimeFact; }
    Fact* baroPress     () { return &_baroPressFact; }
    Fact* intakePress   () { return &_intakePressFact; }
    Fact* intakeTemp    () { return &_intakeTempFact; }
    Fact* cylinderTemp  () { return &_cylinderTempFact; }
    Fact* ignTime       () { return &_ignTimeFact; }
    Fact* injTime       () { return &_injTimeFact; }
    Fact* exGasTemp     () { return &_exGasTempFact; }
    Fact* throttleOut   () { return &_throttleOutFact; }
    Fact* ptComp        () { return &_ptCompFact; }
    Fact* ignVoltage    () { return &_ignVoltageFact; }

    // Overrides from FactGroup
    virtual void handleMessage(Vehicle* vehicle, mavlink_message_t& message) override;

private:
    void _handleEFIStatus(mavlink_message_t& message);

    const QString _healthFactName =         QStringLiteral("health");
    const QString _ecuIndexFactName =       QStringLiteral("ecuIndex");
    const QString _rpmFactName =            QStringLiteral("rpm");
    const QString _fuelConsumedFactName =   QStringLiteral("fuelConsumed");
    const QString _fuelFlowFactName =       QStringLiteral("fuelFlow");
    const QString _engineLoadFactName =     QStringLiteral("engineLoad");
    const QString _throttlePosFactName =    QStringLiteral("throttlePos");
    const QString _sparkTimeFactName =      QStringLiteral("sparkTime");
    const QString _baroPressFactName =      QStringLiteral("baroPress");
    const QString _intakePressFactName =    QStringLiteral("intakePress");
    const QString _intakeTempFactName =     QStringLiteral("intakeTemp");
    const QString _cylinderTempFactName =   QStringLiteral("cylinderTemp");
    const QString _ignTimeFactName =        QStringLiteral("ignTime");
    const QString _injTimeFactName =        QStringLiteral("injTime");
    const QString _exGasTempFactName =      QStringLiteral("exGasTemp");
    const QString _throttleOutFactName =    QStringLiteral("throttleOut");
    const QString _ptCompFactName =         QStringLiteral("ptComp");

    Fact _healthFact;
    Fact _ecuIndexFact;
    Fact _rpmFact;
    Fact _fuelConsumedFact;
    Fact _fuelFlowFact;
    Fact _engineLoadFact;
    Fact _throttlePosFact;
    Fact _sparkTimeFact;
    Fact _baroPressFact;
    Fact _intakePressFact;
    Fact _intakeTempFact;
    Fact _cylinderTempFact;
    Fact _ignTimeFact;
    Fact _injTimeFact;
    Fact _exGasTempFact;
    Fact _throttleOutFact;
    Fact _ptCompFact;
    Fact _ignVoltageFact;
};
