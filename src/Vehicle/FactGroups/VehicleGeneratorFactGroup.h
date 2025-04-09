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

class VehicleGeneratorFactGroup : public FactGroup
{
    Q_OBJECT
    Q_PROPERTY(Fact *status                     READ status             CONSTANT)
    Q_PROPERTY(Fact *genSpeed                   READ genSpeed           CONSTANT)
    Q_PROPERTY(Fact *batteryCurrent             READ batteryCurrent     CONSTANT)
    Q_PROPERTY(Fact *loadCurrent                READ loadCurrent        CONSTANT)
    Q_PROPERTY(Fact *powerGenerated             READ powerGenerated     CONSTANT)
    Q_PROPERTY(Fact *busVoltage                 READ busVoltage         CONSTANT)
    Q_PROPERTY(Fact *rectifierTemp              READ rectifierTemp      CONSTANT)
    Q_PROPERTY(Fact *batCurrentSetpoint         READ batCurrentSetpoint CONSTANT)
    Q_PROPERTY(Fact *genTemp                    READ genTemp            CONSTANT)
    Q_PROPERTY(Fact *runtime                    READ runtime            CONSTANT)
    Q_PROPERTY(Fact *timeMaintenance            READ timeMaintenance    CONSTANT)
    Q_PROPERTY(QVariantList flagsListGenerator  READ flagsListGenerator NOTIFY flagsListGeneratorChanged)

public:
    explicit VehicleGeneratorFactGroup(QObject *parent = nullptr);

    Fact *status() { return &_statusFact; }
    Fact *genSpeed() { return &_genSpeedFact; }
    Fact *batteryCurrent() { return &_batteryCurrentFact; }
    Fact *loadCurrent() { return &_loadCurrentFact; }
    Fact *powerGenerated() { return &_powerGeneratedFact; }
    Fact *busVoltage() { return &_busVoltageFact; }
    Fact *rectifierTemp() { return &_rectifierTempFact; }
    Fact *batCurrentSetpoint() { return &_batCurrentSetpointFact; }
    Fact *genTemp() { return &_genTempFact; }
    Fact *runtime() { return &_runtimeFact; }
    Fact *timeMaintenance() { return &_timeMaintenanceFact; }
    QVariantList &flagsListGenerator() {return _flagsListGenerator; }

    // Overrides from FactGroup
    void handleMessage(Vehicle *vehicle, const mavlink_message_t &message) final;

signals:
    void flagsListGeneratorChanged();

private slots:
    void _updateGeneratorFlags(const QVariant &value);

private:
    void _handleGeneratorStatus(const mavlink_message_t &message);

    Fact _statusFact = Fact(0, QStringLiteral("status"), FactMetaData::valueTypeUint64);
    Fact _genSpeedFact = Fact(0, QStringLiteral("genSpeed"), FactMetaData::valueTypeUint16);
    Fact _batteryCurrentFact = Fact(0, QStringLiteral("batteryCurrent"), FactMetaData::valueTypeFloat);
    Fact _loadCurrentFact = Fact(0, QStringLiteral("loadCurrent"), FactMetaData::valueTypeFloat);
    Fact _powerGeneratedFact = Fact(0, QStringLiteral("powerGenerated"), FactMetaData::valueTypeFloat);
    Fact _busVoltageFact = Fact(0, QStringLiteral("busVoltage"), FactMetaData::valueTypeFloat);
    Fact _rectifierTempFact = Fact(0, QStringLiteral("rectifierTemp"), FactMetaData::valueTypeInt16);
    Fact _batCurrentSetpointFact = Fact(0, QStringLiteral("batCurrentSetpoint"), FactMetaData::valueTypeFloat);
    Fact _genTempFact = Fact(0, QStringLiteral("genTemp"), FactMetaData::valueTypeInt16);
    Fact _runtimeFact = Fact(0, QStringLiteral("runtime"), FactMetaData::valueTypeUint32);
    Fact _timeMaintenanceFact = Fact(0, QStringLiteral("timeMaintenance"), FactMetaData::valueTypeInt32);

    QVariantList _flagsListGenerator;
    int _prevFlag = 0;
};
