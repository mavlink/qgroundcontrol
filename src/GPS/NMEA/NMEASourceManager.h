#pragma once

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QTimer>
#include <QtQmlIntegration/QtQmlIntegration>

#include <deque>
#include <functional>
#include <memory>

#include "GPSConnectionState.h"
#include "GPSReceiverProfile.h"
#include "GPSRuntimeScheduler.h"
#include "NMEAConnectionAttempt.h"
#include "NMEADecoderSession.h"

#ifndef QGC_NO_SERIAL_LINK
#include "SerialPortManager.h"
#endif

class QGeoPositionInfoSource;
class QIODevice;

/// Owns one NMEA connection and both decoders; consumers independently borrow observations.
class NMEASourceManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_PROPERTY(GPSConnectionState::State connectionState READ connectionState NOTIFY stateChanged)
    Q_PROPERTY(GPSSourceHealth* health READ health CONSTANT)
    Q_PROPERTY(bool active READ active NOTIFY stateChanged)
    Q_PROPERTY(QString status READ status NOTIFY stateChanged)
    Q_PROPERTY(int satellitesInViewCount READ satellitesInViewCount NOTIFY satellitesChanged)
    Q_PROPERTY(int satellitesInUseCount READ satellitesInUseCount NOTIFY satellitesChanged)
    friend class NMEASourceManagerTest;

public:
    explicit NMEASourceManager(QObject* parent = nullptr, GPSRuntimeScheduler* scheduler = nullptr);
    void setProfile(const GPSReceiverProfile& profile);
    void setAutoConnect(bool enabled);
    void setSuspended(bool suspended);
    ~NMEASourceManager() override;

    void setRecordingBuffer(const std::shared_ptr<GPSRecordingBuffer>& buffer) { _recordingBuffer = buffer; }
    void update();
    void stop();
    void shutdown();
    bool connectSource();
    void disconnectSource();
#ifndef QGC_NO_SERIAL_LINK
    /// An explicit null inventory disables serial discovery for this source.
    void setSerialDiscovery(SerialPortManager* serialPorts);
#endif

    QGeoPositionInfoSource* positionSource() const;

    GPSSourceHealth* health() { return _decoder.health(); }

    bool active() const { return _connection.active(); }

    GPSConnectionState::State connectionState() const { return _connection.state(); }

    QString status() const { return _status; }

    /// Counts are -1 until fresh satellite information is available.
    int satellitesInViewCount() const { return _decoder.health()->satellitesInViewCount(); }

    int satellitesInUseCount() const { return _decoder.health()->satellitesInUseCount(); }

    GPSSatelliteObservation satelliteObservation() const { return _decoder.satelliteObservation(); }
    quint64 sessionId() const { return _decoder.sessionId(); }

signals:
    void positionSourceChanged();
    void stateChanged();
    void satellitesChanged();
    void satellitesReceived(const GPSSatelliteObservation& observation);

private:
    bool _shouldConnect() const;
    void _dispatch(std::function<void()> command);
    void _update();
    void _stop();
    void _closeDevice();
    void _setStatus(const QString& status);
    void _scheduleUpdate();
    void _updateSerialRouting();
    bool _installSource(QIODevice* device);
    void _startAttempt();
    void _openAttempt();
    void _uninstallSource();
    void _attemptFailed(const QString& detail);
    void _notifyState();

    std::shared_ptr<GPSRecordingBuffer> _recordingBuffer;
    QPointer<GPSRuntimeScheduler> _scheduler;
    GPSRuntimeScheduler::TaskId _updateTask = 0;
    bool _automatic = false;
    bool _suspended = false;
    bool _stopped = false;
    GPSReceiverProfile _profile;
    quint64 _attemptGeneration = 0;
    std::unique_ptr<NMEAConnectionAttempt> _attempt;
    NMEADecoderSession _decoder;
    QTimer _udpActivityTimer;
    GPSConnectionState _connection;
    GPSProvider::TransportFactory _receiverFactory;
    bool _sourceAvailable = false;
    bool _dispatching = false;
    bool _stateNotificationPending = false;
    bool _shutdown = false;
    std::deque<std::function<void()>> _commands;
    QString _status;
#ifndef QGC_NO_SERIAL_LINK
    QPointer<SerialPortManager> _serialPorts;
    SerialPortManager::ReservationPtr _autoConnectExclusion;
#endif
};
