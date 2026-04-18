#pragma once

#include "FactGroup.h"

/// Radio link telemetry decoded from MAVLINK_MSG_ID_RADIO_STATUS.
///
/// Consolidates the seven scalars that the 3DR-style telemetry radios publish:
/// local/remote RSSI (dBm), local/remote noise floor (dBm), RX error counters,
/// errors fixed, and the TX buffer fill percentage. The 3DR Si1k raw-to-dBm
/// conversion from the datasheet is applied here (detected via the '3'/'D'
/// sysid/compid marker).
class RadioStatusFactGroup : public FactGroup
{
    Q_OBJECT
    Q_PROPERTY(Fact *lrssi      READ lrssi      CONSTANT)
    Q_PROPERTY(Fact *rrssi      READ rrssi      CONSTANT)
    Q_PROPERTY(Fact *rxErrors   READ rxErrors   CONSTANT)
    Q_PROPERTY(Fact *fixed      READ fixed      CONSTANT)
    Q_PROPERTY(Fact *txBuffer   READ txBuffer   CONSTANT)
    Q_PROPERTY(Fact *lNoise     READ lNoise     CONSTANT)
    Q_PROPERTY(Fact *rNoise     READ rNoise     CONSTANT)

public:
    explicit RadioStatusFactGroup(QObject *parent = nullptr);

    Fact *lrssi()    { return &_lrssiFact; }
    Fact *rrssi()    { return &_rrssiFact; }
    Fact *rxErrors() { return &_rxErrorsFact; }
    Fact *fixed()    { return &_fixedFact; }
    Fact *txBuffer() { return &_txBufferFact; }
    Fact *lNoise()   { return &_lNoiseFact; }
    Fact *rNoise()   { return &_rNoiseFact; }

    void handleMessage(Vehicle *vehicle, const mavlink_message_t &message) final;

private:
    void _handleRadioStatus(const mavlink_message_t &message);

    Fact _lrssiFact    = Fact(0, QStringLiteral("lrssi"),    FactMetaData::valueTypeInt16);
    Fact _rrssiFact    = Fact(0, QStringLiteral("rrssi"),    FactMetaData::valueTypeInt16);
    Fact _rxErrorsFact = Fact(0, QStringLiteral("rxErrors"), FactMetaData::valueTypeUint32);
    Fact _fixedFact    = Fact(0, QStringLiteral("fixed"),    FactMetaData::valueTypeUint32);
    Fact _txBufferFact = Fact(0, QStringLiteral("txBuffer"), FactMetaData::valueTypeUint32);
    Fact _lNoiseFact   = Fact(0, QStringLiteral("lNoise"),   FactMetaData::valueTypeInt16);
    Fact _rNoiseFact   = Fact(0, QStringLiteral("rNoise"),   FactMetaData::valueTypeInt16);
};
