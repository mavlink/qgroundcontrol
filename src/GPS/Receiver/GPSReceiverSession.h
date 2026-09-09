#pragma once

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QSet>

#include "GPSByteStream.h"
#include "GPSProvider.h"

/// Owns a receiver attempt and retires cancelled workers without blocking the UI.
class GPSReceiverSession : public QObject
{
    Q_OBJECT

    friend class GPSReceiverSessionTest;
    friend class GPSReceiverTest;
    friend class GPSRtkStateTest;

public:
    explicit GPSReceiverSession(QObject* parent = nullptr);
    ~GPSReceiverSession() override;

    void start(GPSType type, GPSProvider::TransportFactory factory, const GPSReceiverConfig& config);
    void stop();
    /// Join all workers during final application shutdown, without an event loop.
    void shutdown();

    bool ready() const { return _ready; }

    bool hasReceiver() const { return !_provider.isNull(); }

    bool stopping() const { return !_retiring.isEmpty(); }

    quint64 sessionId() const { return _generation; }

    const GPSReceiverConfig& config() const { return _config; }

    QIODevice* nmeaDevice() const { return _nmeaStream.get(); }

    const GPSReceiverCapabilities& capabilities() const { return _capabilities; }

    QString errorDetail() const { return _errorDetail; }

signals:
    void receiverTypeChanged(GPSType type);
    void configurationStarted();
    void receiverReady();
    void disconnected();
    void connectionError(GPSConnectionError error);
    void connectionErrorDetail(GPSConnectionError error, const QString& detail);
    void capabilitiesUpdated(const GPSReceiverCapabilities& capabilities);
    void stateChanged();
    void positionReceived(const GPSObservation& observation);
    void satellitesReceived(const GPSSatelliteObservation& observation);
    void relativePositionReceived(const GPSRelativeObservation& observation);
    void rtcmReceived(const QByteArray& data);
    void rtcmFrameReceived(const QByteArray& data, qint64 receivedAtMs);
    void surveyInReceived(const GPSSurveyInStatus& status);

private:
    QPointer<GPSProvider> _provider;
    std::unique_ptr<GPSByteStream> _nmeaStream;
    QSet<GPSProvider*> _workers;
    QSet<GPSProvider*> _started;
    QSet<GPSProvider*> _retiring;
    quint64 _generation = 0;
    bool _ready = false;
    bool _shutdown = false;
    GPSReceiverConfig _config;
    GPSReceiverCapabilities _capabilities;
    QString _errorDetail;
};
