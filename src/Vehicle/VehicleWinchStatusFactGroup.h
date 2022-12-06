/****************************************************************************
 *
 * (c) TODO
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "FactGroup.h"
#include "QGCMAVLink.h"

class Vehicle;

class VehicleWinchStatusFactGroup : public FactGroup
{
    Q_OBJECT

public:
    VehicleWinchStatusFactGroup(QObject* parent = nullptr);

    Q_PROPERTY(Fact* lineLength      READ lineLength       CONSTANT)
    Q_PROPERTY(Fact* speed           READ speed            CONSTANT)
    Q_PROPERTY(Fact* tension         READ tension          CONSTANT)
    Q_PROPERTY(Fact* voltage         READ voltage          CONSTANT)
    Q_PROPERTY(Fact* current         READ current          CONSTANT)
    Q_PROPERTY(Fact* temperature     READ temperature      CONSTANT)
    Q_PROPERTY(Fact* healthy         READ healthy          CONSTANT)
    Q_PROPERTY(Fact* fullyRetracted  READ fullyRetracted   CONSTANT)
    Q_PROPERTY(Fact* moving          READ moving           CONSTANT)
    Q_PROPERTY(Fact* clutchEngaged   READ clutchEngaged    CONSTANT)
    Q_PROPERTY(Fact* locked          READ locked           CONSTANT)
    Q_PROPERTY(Fact* dropping        READ dropping         CONSTANT)
    Q_PROPERTY(Fact* arresting       READ arresting        CONSTANT)
    Q_PROPERTY(Fact* groundSense     READ groundSense      CONSTANT)
    Q_PROPERTY(Fact* retracting      READ retracting       CONSTANT)
    Q_PROPERTY(Fact* redeliver       READ redeliver        CONSTANT)
    Q_PROPERTY(Fact* abandonLine     READ abandonLine      CONSTANT)
    Q_PROPERTY(Fact* locking         READ locking          CONSTANT)
    Q_PROPERTY(Fact* loadLine        READ loadLine         CONSTANT)
    Q_PROPERTY(Fact* loadPayload     READ loadPayload      CONSTANT)
    Q_PROPERTY(Fact* dropAllowed     READ dropAllowed      CONSTANT)
    Q_PROPERTY(Fact* dropBlocker     READ dropBlocker      CONSTANT)
    Q_PROPERTY(Fact* stale           READ stale            CONSTANT)



    Fact *lineLength      () { return &_lineLengthFact; }
    Fact *speed           () { return &_speedFact; }
    Fact *tension         () { return &_tensionFact; }
    Fact *voltage         () { return &_voltageFact; }
    Fact *current         () { return &_currentFact; }
    Fact *temperature     () { return &_temperatureFact; }
    Fact *healthy         () { return &_healthyFact; }
    Fact *fullyRetracted  () { return &_fullyRetractedFact; }
    Fact *moving          () { return &_movingFact; }
    Fact *clutchEngaged   () { return &_clutchEngagedFact; }
    Fact *locked          () { return &_lockedFact; }
    Fact *dropping        () { return &_droppingFact; }
    Fact *arresting       () { return &_arrestingFact; }
    Fact *groundSense     () { return &_groundSenseFact; }
    Fact *retracting      () { return &_retractingFact; }
    Fact *redeliver       () { return &_redeliverFact; }
    Fact *abandonLine     () { return &_abandonLineFact; }
    Fact *locking         () { return &_lockingFact; }
    Fact *loadLine        () { return &_loadLineFact; }
    Fact *loadPayload     () { return &_loadPayloadFact; }
    Fact *dropAllowed     () { return &_dropAllowedFact; }
    Fact *dropBlocker     () { return &_dropBlockerFact; }
    Fact *stale           () { return &_staleFact; }

    // Overrides from FactGroup
    void handleMessage(Vehicle* vehicle, mavlink_message_t& message) override;

    static const char* _lineLengthFactName;
    static const char* _speedFactName;
    static const char* _tensionFactName;
    static const char* _voltageFactName;
    static const char* _currentFactName;
    static const char* _temperatureFactName;
    static const char* _healthyFactName;
    static const char* _fullyRetractedFactName;
    static const char* _movingFactName;
    static const char* _clutchEngagedFactName;
    static const char* _lockedFactName;
    static const char* _droppingFactName;
    static const char* _arrestingFactName;
    static const char* _groundSenseFactName;
    static const char* _retractingFactName;
    static const char* _redeliverFactName;
    static const char* _abandonLineFactName;
    static const char* _lockingFactName;
    static const char* _loadLineFactName;
    static const char* _loadPayloadFactName;
    static const char* _dropAllowedFactName;
    static const char* _dropBlockerFactName;
    static const char* _staleFactName;

private slots:
    void _updateAllValues() override;

private:
    Fact _lineLengthFact;
    Fact _speedFact;
    Fact _tensionFact;
    Fact _voltageFact;
    Fact _currentFact;
    Fact _temperatureFact;
    Fact _healthyFact;
    Fact _fullyRetractedFact;
    Fact _movingFact;
    Fact _clutchEngagedFact;
    Fact _lockedFact;
    Fact _droppingFact;
    Fact _arrestingFact;
    Fact _groundSenseFact;
    Fact _retractingFact;
    Fact _redeliverFact;
    Fact _abandonLineFact;
    Fact _lockingFact;
    Fact _loadLineFact;
    Fact _loadPayloadFact;
    Fact _dropAllowedFact;
    Fact _dropBlockerFact;
    Fact _staleFact;
    int  _staleCounter;
};
