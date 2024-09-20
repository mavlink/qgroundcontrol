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

class VehicleGeneratorFactGroup : public FactGroup
{
    Q_OBJECT

public:
    VehicleGeneratorFactGroup(QObject* parent = nullptr);

    Q_PROPERTY(Fact* status             READ status             CONSTANT)
    Q_PROPERTY(Fact* genSpeed           READ genSpeed           CONSTANT)
    Q_PROPERTY(Fact* batteryCurrent     READ batteryCurrent     CONSTANT)
    Q_PROPERTY(Fact* loadCurrent        READ loadCurrent        CONSTANT)
    Q_PROPERTY(Fact* powerGenerated     READ powerGenerated     CONSTANT)
    Q_PROPERTY(Fact* busVoltage         READ busVoltage         CONSTANT)
    Q_PROPERTY(Fact* rectifierTemp      READ rectifierTemp      CONSTANT)
    Q_PROPERTY(Fact* batCurrentSetpoint READ batCurrentSetpoint CONSTANT)
    Q_PROPERTY(Fact* genTemp            READ genTemp            CONSTANT)
    Q_PROPERTY(Fact* runtime            READ runtime            CONSTANT)
    Q_PROPERTY(Fact* timeMaintenance    READ timeMaintenance    CONSTANT)
    Q_PROPERTY(QVariantList flagsListGenerator   READ flagsListGenerator  NOTIFY flagsListGeneratorChanged)

    Fact* status                () { return &_statusFact; }
    Fact* genSpeed              () { return &_genSpeedFact; }
    Fact* batteryCurrent        () { return &_batteryCurrentFact; }
    Fact* loadCurrent           () { return &_loadCurrentFact; }
    Fact* powerGenerated        () { return &_powerGeneratedFact; }
    Fact* busVoltage            () { return &_busVoltageFact; }
    Fact* rectifierTemp         () { return &_rectifierTempFact; }
    Fact* batCurrentSetpoint    () { return &_batCurrentSetpointFact; }
    Fact* genTemp               () { return &_genTempFact; }
    Fact* runtime               () { return &_runtimeFact; }
    Fact* timeMaintenance       () { return &_timeMaintenanceFact; }
    QVariantList& flagsListGenerator() {return _flagsListGenerator; }

    // Overrides from FactGroup
    virtual void handleMessage(Vehicle* vehicle, mavlink_message_t& message) override;

signals:
    void flagsListGeneratorChanged();

protected:
    void _handleGeneratorStatus(mavlink_message_t& message);
    void _updateGeneratorFlags();

    const QString _statusFactName =                QStringLiteral("status");
    const QString _genSpeedFactName =              QStringLiteral("genSpeed");
    const QString _batteryCurrentFactName =        QStringLiteral("batteryCurrent");
    const QString _loadCurrentFactName =           QStringLiteral("loadCurrent");
    const QString _powerGeneratedFactName =        QStringLiteral("powerGenerated");
    const QString _busVoltageFactName =            QStringLiteral("busVoltage");
    const QString _rectifierTempFactName =         QStringLiteral("rectifierTemp");
    const QString _batCurrentSetpointFactName =    QStringLiteral("batCurrentSetpoint");
    const QString _genTempFactName =               QStringLiteral("genTemp");
    const QString _runtimeFactName =               QStringLiteral("runtime");
    const QString _timeMaintenanceFactName =       QStringLiteral("timeMaintenance");

    Fact _statusFact;
    Fact _genSpeedFact;
    Fact _batteryCurrentFact;
    Fact _loadCurrentFact;
    Fact _powerGeneratedFact;
    Fact _busVoltageFact;
    Fact _rectifierTempFact;
    Fact _batCurrentSetpointFact;
    Fact _genTempFact;
    Fact _runtimeFact;
    Fact _timeMaintenanceFact;

    QVariantList _flagsListGenerator;
    int _prevFlag;
};
