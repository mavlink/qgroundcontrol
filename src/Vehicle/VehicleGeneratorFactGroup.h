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

    static const char* _statusFactName;
    static const char* _genSpeedFactName;
    static const char* _batteryCurrentFactName;
    static const char* _loadCurrentFactName;
    static const char* _powerGeneratedFactName;
    static const char* _busVoltageFactName;
    static const char* _rectifierTempFactName;
    static const char* _batCurrentSetpointFactName;
    static const char* _genTempFactName;
    static const char* _runtimeFactName;
    static const char* _timeMaintenanceFactName;

signals:
    void flagsListGeneratorChanged();

protected:
    void _handleGeneratorStatus(mavlink_message_t& message);
    void _updateGeneratorFlags();

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
