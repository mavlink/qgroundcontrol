/****************************************************************************
 *
 * USV Payload Fact Group
 * 无人船载荷数据组 - 用于管理水质监测遥测数据
 *
 ****************************************************************************/

#pragma once

#include "FactSystem/FactGroup.h"
#include <QtCore/QTimer>

class USVPayloadFactGroup : public FactGroup
{
    Q_OBJECT
    Q_PROPERTY(Fact *voltage    READ voltage    CONSTANT)
    Q_PROPERTY(Fact *absorbance READ absorbance CONSTANT)
    Q_PROPERTY(Fact *pumpX      READ pumpX      CONSTANT)
    Q_PROPERTY(Fact *pumpY      READ pumpY      CONSTANT)
    Q_PROPERTY(Fact *pumpZ      READ pumpZ      CONSTANT)
    Q_PROPERTY(Fact *pumpA      READ pumpA      CONSTANT)
    Q_PROPERTY(Fact *status     READ status     CONSTANT)
    Q_PROPERTY(Fact *linkActive   READ linkActive   CONSTANT)
    Q_PROPERTY(Fact *packetCount  READ packetCount  CONSTANT)

public:
    explicit USVPayloadFactGroup(QObject *parent = nullptr);

    Fact *voltage()    { return &_voltageFact; }
    Fact *absorbance() { return &_absorbanceFact; }
    Fact *pumpX()      { return &_pumpXFact; }
    Fact *pumpY()      { return &_pumpYFact; }
    Fact *pumpZ()      { return &_pumpZFact; }
    Fact *pumpA()      { return &_pumpAFact; }
    Fact *status()     { return &_statusFact; }
    Fact *linkActive()   { return &_linkActiveFact; }
    Fact *packetCount()  { return &_packetCountFact; }

    // Overrides from FactGroup
    void handleMessage(Vehicle *vehicle, const mavlink_message_t &message) final;

private slots:
    void _telemetryTimeout();

private:
    void _handleNamedValueFloat(const mavlink_message_t &message);

    Fact _voltageFact    = Fact(0, QStringLiteral("voltage"),    FactMetaData::valueTypeFloat);
    Fact _absorbanceFact = Fact(0, QStringLiteral("absorbance"), FactMetaData::valueTypeFloat);
    Fact _pumpXFact      = Fact(0, QStringLiteral("pumpX"),      FactMetaData::valueTypeFloat);
    Fact _pumpYFact      = Fact(0, QStringLiteral("pumpY"),      FactMetaData::valueTypeFloat);
    Fact _pumpZFact      = Fact(0, QStringLiteral("pumpZ"),      FactMetaData::valueTypeFloat);
    Fact _pumpAFact      = Fact(0, QStringLiteral("pumpA"),      FactMetaData::valueTypeFloat);
    Fact _statusFact     = Fact(0, QStringLiteral("status"),     FactMetaData::valueTypeUint32);
    Fact _linkActiveFact = Fact(0, QStringLiteral("linkActive"), FactMetaData::valueTypeUint32);
    Fact _packetCountFact = Fact(0, QStringLiteral("packetCount"), FactMetaData::valueTypeFloat);
    QTimer _timeoutTimer;
    static constexpr int _timeoutMsecs = 5000;
};
