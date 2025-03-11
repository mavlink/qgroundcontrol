#pragma once

#include "FactGroup.h"
#include "QGCMAVLink.h"

class VehicleRepeaterFactGroup : public FactGroup
{
    Q_OBJECT

public:
    explicit VehicleRepeaterFactGroup(QObject* parent = nullptr);

    Q_PROPERTY(Fact* rssi                            READ rssi                           CONSTANT)
    Q_PROPERTY(Fact* rssnr                           READ rssnr                          CONSTANT)
    Q_PROPERTY(Fact* freq                            READ freq                           CONSTANT)
    Q_PROPERTY(Fact* videoKbps                       READ videoKbps                      CONSTANT)
    Q_PROPERTY(Fact* mavlinkToDroneKbps              READ mavlinkToDroneKbps             CONSTANT)
    Q_PROPERTY(Fact* mavlinkFromDroneKbps            READ mavlinkFromDroneKbps           CONSTANT)
    Q_PROPERTY(Fact* isDroneConnected                READ isDroneConnected               CONSTANT)
    Q_PROPERTY(Fact* isDroneStats                    READ isDroneStats                   CONSTANT)
    Q_PROPERTY(Fact* isRoutable         READ isRoutable        CONSTANT)
    Q_PROPERTY(Fact* latencyMs          READ latencyMs         CONSTANT)
    Q_PROPERTY(Fact* lossPercent        READ lossPercent       CONSTANT)

    Fact* rssi                             () { return &_rssiFact; }
    Fact* rssnr                            () { return &_rssnrFact; }
    Fact* freq                             () { return &_freqFact; }
    Fact* videoKbps                        () { return &_videoKbpsFact; }
    Fact* mavlinkToDroneKbps               () { return &_mavlinkToDroneKbpsFact; }
    Fact* mavlinkFromDroneKbps             () { return &_mavlinkFromDroneKbpsFact; }
    Fact* isDroneConnected                 () { return &_isDroneConnectedFact; }
    Fact* isDroneStats                     () { return &_isDroneStatsFact; }
    Fact* isRoutable        () { return &_isRoutableFact; }
    Fact* latencyMs         () { return &_latencyMsFact; }
    Fact* lossPercent       () { return &_lossPercentFact; }

    void handleMessage(Vehicle *vehicle, const mavlink_message_t &message) override;

protected:
    const QString _rssiFactName =                          QStringLiteral("rssi");
    const QString _rssnrFactName =                         QStringLiteral("rssnr");
    const QString _freqFactName =                          QStringLiteral("freq");
    const QString _videoKbpsFactName =                     QStringLiteral("videoKbps");
    const QString _mavlinkToDroneKbpsFactName =            QStringLiteral("mavlinkToDroneKbps");
    const QString _mavlinkFromDroneKbpsFactName =          QStringLiteral("mavlinkFromDroneKbps");
    const QString _isDroneConnectedFactName =              QStringLiteral("isDroneConnected");
    const QString _isDroneStatsFactName =                  QStringLiteral("isDroneStats");
    const QString _isRoutableFactName =          QStringLiteral("isRoutable");
    const QString _latencyMsFactName =           QStringLiteral("latencyMs");
    const QString _lossPercentFactName =         QStringLiteral("lossPercent");

    Fact _rssiFact;
    Fact _rssnrFact;
    Fact _freqFact;
    Fact _videoKbpsFact;
    Fact _mavlinkToDroneKbpsFact;
    Fact _mavlinkFromDroneKbpsFact;
    Fact _isDroneConnectedFact;
    Fact _isDroneStatsFact;
    Fact _isRoutableFact;
    Fact _latencyMsFact;
    Fact _lossPercentFact;

private:
    void _handleCommandLong                 (const mavlink_message_t &message);
};
