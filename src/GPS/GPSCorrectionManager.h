#pragma once

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtQmlIntegration/QtQmlIntegration>

#include "RTCMMavlink.h"
#include "RTCMUdpInput.h"

class NTRIPSettings;

/// Owns the shared MAVLink sequence domain and UDP correction input for all GPS sources.
class GPSCorrectionManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_PROPERTY(RTCMMavlink* rtcmMavlink READ rtcmMavlink CONSTANT)

    friend class GPSCorrectionManagerTest;

public:
    explicit GPSCorrectionManager(QObject* parent = nullptr);
    ~GPSCorrectionManager() override;

    void init(NTRIPSettings* settings);
    void shutdown();

    RTCMMavlink* rtcmMavlink() { return &_rtcmMavlink; }

public slots:
    void forwardCorrections(const QByteArray& data);

private:
    void _applyUdpInputSettings();

    RTCMMavlink _rtcmMavlink;
    RTCMUdpInput _udpInput;
    QPointer<NTRIPSettings> _settings;
    bool _shutdown = false;
};
