#pragma once

#include <QLoggingCategory>
#include "FactGroup.h"
#include "QGCMAVLink.h"

Q_DECLARE_LOGGING_CATEGORY(VehicleLTEFactGroupLog)

class VehicleLTEFactGroup : public FactGroup
{
    Q_OBJECT

public:
    explicit VehicleLTEFactGroup(QObject* parent = nullptr);

    Q_PROPERTY(Fact* rssi               READ rssi              CONSTANT)
    Q_PROPERTY(Fact* rsrq               READ rsrq              CONSTANT)
    Q_PROPERTY(Fact* rsrp               READ rsrp              CONSTANT)
    Q_PROPERTY(Fact* rssnr              READ rssnr             CONSTANT)
    Q_PROPERTY(Fact* earfcn             READ earfcn            CONSTANT)
    Q_PROPERTY(Fact* lteTx              READ lteTx             CONSTANT)
    Q_PROPERTY(Fact* isRoutable         READ isRoutable        CONSTANT)
    Q_PROPERTY(Fact* latencyMs          READ latencyMs         CONSTANT)
    Q_PROPERTY(Fact* lossPercent        READ lossPercent       CONSTANT)

    Fact* rssi              () { return &_rssiFact; }
    Fact* rsrq              () { return &_rsrqFact; }
    Fact* rsrp              () { return &_rsrpFact; }
    Fact* rssnr             () { return &_rssnrFact; }
    Fact* earfcn            () { return &_earfcnFact; }
    Fact* lteTx             () { return &_lteTxFact; }
    Fact* isRoutable        () { return &_isRoutableFact; }
    Fact* latencyMs         () { return &_latencyMsFact; }
    Fact* lossPercent       () { return &_lossPercentFact; }

    void handleMessage(Vehicle* vehicle, const mavlink_message_t message);

protected:
    const QString _rssiFactName =                QStringLiteral("rssi");
    const QString _rsrqFactName =                QStringLiteral("rsrq");
    const QString _rsrpFactName =                QStringLiteral("rsrp");
    const QString _rssnrFactName =               QStringLiteral("rssnr");
    const QString _earfcnFactName =              QStringLiteral("earfcn");
    const QString _lteTxFactName =               QStringLiteral("ltetx");
    const QString _isRoutableFactName =          QStringLiteral("isRoutable");
    const QString _latencyMsFactName =           QStringLiteral("latencyMs");
    const QString _lossPercentFactName =         QStringLiteral("lossPercent");

    Fact _rsrqFact;
    Fact _rsrpFact;
    Fact _rssiFact;
    Fact _rssnrFact;
    Fact _earfcnFact;
    Fact _lteTxFact;
    Fact _isRoutableFact;
    Fact _latencyMsFact;
    Fact _lossPercentFact;

private:
    void _handleCommandLong                 (mavlink_message_t message);
};
