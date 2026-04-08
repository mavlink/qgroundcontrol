#pragma once

#include "FactGroupListModel.h"

class BatteryFactGroupListModel : public FactGroupListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

public:
    explicit BatteryFactGroupListModel(QObject* parent = nullptr);

protected:
    // Overrides from FactGroupListModel
    bool _shouldHandleMessage(const mavlink_message_t &message, QList<uint32_t> &ids) const final;
    FactGroupWithId *_createFactGroupWithId(uint32_t id) final;
};

class BatteryFactGroup : public FactGroupWithId
{
    Q_OBJECT
    Q_PROPERTY(Fact *function                       READ function                       CONSTANT)
    Q_PROPERTY(Fact *type                           READ type                           CONSTANT)
    Q_PROPERTY(Fact *temperature                    READ temperature                    CONSTANT)
    Q_PROPERTY(Fact *voltage                        READ voltage                        CONSTANT)
    Q_PROPERTY(Fact *current                        READ current                        CONSTANT)
    Q_PROPERTY(Fact *mahConsumed                    READ mahConsumed                    CONSTANT)
    Q_PROPERTY(Fact *percentRemaining               READ percentRemaining               CONSTANT)
    Q_PROPERTY(Fact *timeRemaining                  READ timeRemaining                  CONSTANT)
    Q_PROPERTY(Fact *timeRemainingStr               READ timeRemainingStr               CONSTANT)
    Q_PROPERTY(Fact *chargeState                    READ chargeState                    CONSTANT)
    Q_PROPERTY(Fact *instantPower                   READ instantPower                   CONSTANT)
    // BATTERY_STATUS_V2 facts
    Q_PROPERTY(Fact *capacityRemaining              READ capacityRemaining              CONSTANT)
    Q_PROPERTY(Fact *capacityRemainingIsInferred    READ capacityRemainingIsInferred    CONSTANT)
    Q_PROPERTY(Fact *statusFlags                    READ statusFlags                    CONSTANT)
    // BATTERY_INFO facts
    Q_PROPERTY(Fact *batteryName                    READ batteryName                    CONSTANT)
    Q_PROPERTY(Fact *serialNumber                   READ serialNumber                   CONSTANT)
    Q_PROPERTY(Fact *manufactureDate                READ manufactureDate                CONSTANT)
    Q_PROPERTY(Fact *fullChargeCapacity             READ fullChargeCapacity             CONSTANT)
    Q_PROPERTY(Fact *designCapacity                 READ designCapacity                 CONSTANT)
    Q_PROPERTY(Fact *nominalVoltage                 READ nominalVoltage                 CONSTANT)
    Q_PROPERTY(Fact *dischargeMinimumVoltage        READ dischargeMinimumVoltage        CONSTANT)
    Q_PROPERTY(Fact *chargingMinimumVoltage         READ chargingMinimumVoltage         CONSTANT)
    Q_PROPERTY(Fact *restingMinimumVoltage          READ restingMinimumVoltage          CONSTANT)
    Q_PROPERTY(Fact *chargingMaximumVoltage         READ chargingMaximumVoltage         CONSTANT)
    Q_PROPERTY(Fact *chargingMaximumCurrent         READ chargingMaximumCurrent         CONSTANT)
    Q_PROPERTY(Fact *dischargeMaximumCurrent        READ dischargeMaximumCurrent        CONSTANT)
    Q_PROPERTY(Fact *dischargeMaximumBurstCurrent   READ dischargeMaximumBurstCurrent   CONSTANT)
    Q_PROPERTY(Fact *cycleCount                     READ cycleCount                     CONSTANT)
    Q_PROPERTY(Fact *weight                         READ weight                         CONSTANT)
    Q_PROPERTY(Fact *stateOfHealth                  READ stateOfHealth                  CONSTANT)
    Q_PROPERTY(Fact *cellsInSeries                  READ cellsInSeries                  CONSTANT)

public:
    explicit BatteryFactGroup(uint32_t batteryId, QObject *parent = nullptr);

    Fact *function()                     { return &_batteryFunctionFact; }
    Fact *type()                         { return &_batteryTypeFact; }
    Fact *voltage()                      { return &_voltageFact; }
    Fact *percentRemaining()             { return &_percentRemainingFact; }
    Fact *mahConsumed()                  { return &_mahConsumedFact; }
    Fact *current()                      { return &_currentFact; }
    Fact *temperature()                  { return &_temperatureFact; }
    Fact *instantPower()                 { return &_instantPowerFact; }
    Fact *timeRemaining()                { return &_timeRemainingFact; }
    Fact *timeRemainingStr()             { return &_timeRemainingStrFact; }
    Fact *chargeState()                  { return &_chargeStateFact; }
    Fact *capacityRemaining()            { return &_capacityRemainingFact; }
    Fact *capacityRemainingIsInferred()  { return &_capacityRemainingIsInferredFact; }
    Fact *statusFlags()                  { return &_statusFlagsFact; }
    Fact *batteryName()                  { return &_batteryNameFact; }
    Fact *serialNumber()                 { return &_serialNumberFact; }
    Fact *manufactureDate()              { return &_manufactureDateFact; }
    Fact *fullChargeCapacity()           { return &_fullChargeCapacityFact; }
    Fact *designCapacity()               { return &_designCapacityFact; }
    Fact *nominalVoltage()               { return &_nominalVoltageFact; }
    Fact *dischargeMinimumVoltage()      { return &_dischargeMinimumVoltageFact; }
    Fact *chargingMinimumVoltage()       { return &_chargingMinimumVoltageFact; }
    Fact *restingMinimumVoltage()        { return &_restingMinimumVoltageFact; }
    Fact *chargingMaximumVoltage()       { return &_chargingMaximumVoltageFact; }
    Fact *chargingMaximumCurrent()       { return &_chargingMaximumCurrentFact; }
    Fact *dischargeMaximumCurrent()      { return &_dischargeMaximumCurrentFact; }
    Fact *dischargeMaximumBurstCurrent() { return &_dischargeMaximumBurstCurrentFact; }
    Fact *cycleCount()                   { return &_cycleCountFact; }
    Fact *weight()                       { return &_weightFact; }
    Fact *stateOfHealth()                { return &_stateOfHealthFact; }
    Fact *cellsInSeries()                { return &_cellsInSeriesFact; }

    // Overrides from FactGroup
    void handleMessage(Vehicle *vehicle, const mavlink_message_t &message) final;

private slots:
    void _timeRemainingChanged(const QVariant &value);

private:
    void _handleHighLatency(Vehicle *vehicle, const mavlink_message_t &message);
    void _handleHighLatency2(Vehicle *vehicle, const mavlink_message_t &message);
    void _handleBatteryStatus(Vehicle *vehicle, const mavlink_message_t &message);
    void _handleBatteryStatusV2(Vehicle *vehicle, const mavlink_message_t &message);
    void _handleBatteryInfo(Vehicle *vehicle, const mavlink_message_t &message);

    // BATTERY_STATUS / shared facts
    Fact _batteryFunctionFact               = Fact(0, QStringLiteral("batteryFunction"),               FactMetaData::valueTypeUint8);
    Fact _batteryTypeFact                   = Fact(0, QStringLiteral("batteryType"),                   FactMetaData::valueTypeUint8);
    Fact _voltageFact                       = Fact(0, QStringLiteral("voltage"),                       FactMetaData::valueTypeDouble);
    Fact _currentFact                       = Fact(0, QStringLiteral("current"),                       FactMetaData::valueTypeDouble);
    Fact _mahConsumedFact                   = Fact(0, QStringLiteral("mahConsumed"),                   FactMetaData::valueTypeDouble);
    Fact _temperatureFact                   = Fact(0, QStringLiteral("temperature"),                   FactMetaData::valueTypeDouble);
    Fact _percentRemainingFact              = Fact(0, QStringLiteral("percentRemaining"),              FactMetaData::valueTypeDouble);
    Fact _timeRemainingFact                 = Fact(0, QStringLiteral("timeRemaining"),                 FactMetaData::valueTypeDouble);
    Fact _timeRemainingStrFact              = Fact(0, QStringLiteral("timeRemainingStr"),              FactMetaData::valueTypeString);
    Fact _chargeStateFact                   = Fact(0, QStringLiteral("chargeState"),                   FactMetaData::valueTypeUint8);
    Fact _instantPowerFact                  = Fact(0, QStringLiteral("instantPower"),                  FactMetaData::valueTypeDouble);
    // BATTERY_STATUS_V2 facts
    Fact _capacityRemainingFact             = Fact(0, QStringLiteral("capacityRemaining"),             FactMetaData::valueTypeDouble);
    Fact _capacityRemainingIsInferredFact   = Fact(0, QStringLiteral("capacityRemainingIsInferred"),   FactMetaData::valueTypeBool);
    Fact _statusFlagsFact                   = Fact(0, QStringLiteral("statusFlags"),                   FactMetaData::valueTypeUint32);
    // BATTERY_INFO facts
    Fact _batteryNameFact                   = Fact(0, QStringLiteral("batteryName"),                   FactMetaData::valueTypeString);
    Fact _serialNumberFact                  = Fact(0, QStringLiteral("serialNumber"),                  FactMetaData::valueTypeString);
    Fact _manufactureDateFact               = Fact(0, QStringLiteral("manufactureDate"),               FactMetaData::valueTypeString);
    Fact _fullChargeCapacityFact            = Fact(0, QStringLiteral("fullChargeCapacity"),            FactMetaData::valueTypeDouble);
    Fact _designCapacityFact                = Fact(0, QStringLiteral("designCapacity"),                FactMetaData::valueTypeDouble);
    Fact _nominalVoltageFact                = Fact(0, QStringLiteral("nominalVoltage"),                FactMetaData::valueTypeDouble);
    Fact _dischargeMinimumVoltageFact       = Fact(0, QStringLiteral("dischargeMinimumVoltage"),       FactMetaData::valueTypeDouble);
    Fact _chargingMinimumVoltageFact        = Fact(0, QStringLiteral("chargingMinimumVoltage"),        FactMetaData::valueTypeDouble);
    Fact _restingMinimumVoltageFact         = Fact(0, QStringLiteral("restingMinimumVoltage"),         FactMetaData::valueTypeDouble);
    Fact _chargingMaximumVoltageFact        = Fact(0, QStringLiteral("chargingMaximumVoltage"),        FactMetaData::valueTypeDouble);
    Fact _chargingMaximumCurrentFact        = Fact(0, QStringLiteral("chargingMaximumCurrent"),        FactMetaData::valueTypeDouble);
    Fact _dischargeMaximumCurrentFact       = Fact(0, QStringLiteral("dischargeMaximumCurrent"),       FactMetaData::valueTypeDouble);
    Fact _dischargeMaximumBurstCurrentFact  = Fact(0, QStringLiteral("dischargeMaximumBurstCurrent"),  FactMetaData::valueTypeDouble);
    Fact _cycleCountFact                    = Fact(0, QStringLiteral("cycleCount"),                    FactMetaData::valueTypeDouble);
    Fact _weightFact                        = Fact(0, QStringLiteral("weight"),                        FactMetaData::valueTypeDouble);
    Fact _stateOfHealthFact                 = Fact(0, QStringLiteral("stateOfHealth"),                 FactMetaData::valueTypeDouble);
    Fact _cellsInSeriesFact                 = Fact(0, QStringLiteral("cellsInSeries"),                 FactMetaData::valueTypeDouble);
};
