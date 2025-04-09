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

class VehicleBatteryFactGroup : public FactGroup
{
    Q_OBJECT
    Q_PROPERTY(Fact *id                 READ id                 CONSTANT)
    Q_PROPERTY(Fact *function           READ function           CONSTANT)
    Q_PROPERTY(Fact *type               READ type               CONSTANT)
    Q_PROPERTY(Fact *temperature        READ temperature        CONSTANT)
    Q_PROPERTY(Fact *voltage            READ voltage            CONSTANT)
    Q_PROPERTY(Fact *current            READ current            CONSTANT)
    Q_PROPERTY(Fact *mahConsumed        READ mahConsumed        CONSTANT)
    Q_PROPERTY(Fact *percentRemaining   READ percentRemaining   CONSTANT)
    Q_PROPERTY(Fact *timeRemaining      READ timeRemaining      CONSTANT)
    Q_PROPERTY(Fact *timeRemainingStr   READ timeRemainingStr   CONSTANT)
    Q_PROPERTY(Fact *chargeState        READ chargeState        CONSTANT)
    Q_PROPERTY(Fact *instantPower       READ instantPower       CONSTANT)

public:
    explicit VehicleBatteryFactGroup(uint8_t batteryId, QObject *parent = nullptr);

    Fact *id() { return &_batteryIdFact; }
    Fact *function() { return &_batteryFunctionFact; }
    Fact *type() { return &_batteryTypeFact; }
    Fact *voltage() { return &_voltageFact; }
    Fact *percentRemaining() { return &_percentRemainingFact; }
    Fact *mahConsumed() { return &_mahConsumedFact; }
    Fact *current() { return &_currentFact; }
    Fact *temperature() { return &_temperatureFact; }
    Fact *instantPower() { return &_instantPowerFact; }
    Fact *timeRemaining() { return &_timeRemainingFact; }
    Fact *timeRemainingStr() { return &_timeRemainingStrFact; }
    Fact *chargeState() { return &_chargeStateFact; }

    /// Creates a new fact group for the battery id as needed and updates the Vehicle with it
    static void handleMessageForFactGroupCreation(Vehicle *vehicle, const mavlink_message_t &message);

    // Overrides from FactGroup
    void handleMessage(Vehicle *vehicle, const mavlink_message_t &message) final;

private slots:
    void _timeRemainingChanged(const QVariant &value);

private:
    static void _handleHighLatency(Vehicle *vehicle, const mavlink_message_t &message);
    static void _handleHighLatency2(Vehicle *vehicle, const mavlink_message_t &message);
    static void _handleBatteryStatus(Vehicle *vehicle, const mavlink_message_t &message);
    static VehicleBatteryFactGroup *_findOrAddBatteryGroupById(Vehicle *vehicle, uint8_t batteryId);

    static constexpr const char *_batteryFactGroupNamePrefix = "battery";

    Fact _batteryIdFact = Fact(0, QStringLiteral("id"), FactMetaData::valueTypeUint8);
    Fact _batteryFunctionFact = Fact(0, QStringLiteral("batteryFunction"), FactMetaData::valueTypeUint8);
    Fact _batteryTypeFact = Fact(0, QStringLiteral("batteryType"), FactMetaData::valueTypeUint8);
    Fact _voltageFact = Fact(0, QStringLiteral("voltage"), FactMetaData::valueTypeDouble);
    Fact _currentFact = Fact(0, QStringLiteral("current"), FactMetaData::valueTypeDouble);
    Fact _mahConsumedFact = Fact(0, QStringLiteral("mahConsumed"), FactMetaData::valueTypeDouble);
    Fact _temperatureFact = Fact(0, QStringLiteral("temperature"), FactMetaData::valueTypeDouble);
    Fact _percentRemainingFact = Fact(0, QStringLiteral("percentRemaining"), FactMetaData::valueTypeDouble);
    Fact _timeRemainingFact = Fact(0, QStringLiteral("timeRemaining"), FactMetaData::valueTypeDouble);
    Fact _timeRemainingStrFact = Fact(0, QStringLiteral("timeRemainingStr"), FactMetaData::valueTypeString);
    Fact _chargeStateFact = Fact(0, QStringLiteral("chargeState"), FactMetaData::valueTypeUint8);
    Fact _instantPowerFact = Fact(0, QStringLiteral("instantPower"), FactMetaData::valueTypeDouble);
};
