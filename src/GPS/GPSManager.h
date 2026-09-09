#pragma once

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtQmlIntegration/QtQmlIntegration>

#include <functional>
#include <memory>

#include "GPSCorrectionManager.h"
#include "GPSReceiverAutoConnect.h"
#include "GPSRecordingController.h"
#include "GPSRelativePositionModel.h"
#include "GPSSatelliteModel.h"
#include "NMEASourceManager.h"

class GPSReceiver;
class GPSBaseStationState;
class GPSPositionSourceRegistration;
class QGCPositionManager;
class QTimer;
class SettingsManager;
class NTRIPManager;

class GPSManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_PROPERTY(QVariantList receiverSettings READ receiverSettings NOTIFY receiverSettingsChanged)
    Q_PROPERTY(QVariantList configurationReport READ configurationReport NOTIFY configurationReportChanged)
    Q_PROPERTY(bool configurationReportActive READ configurationReportActive NOTIFY configurationReportChanged)
    Q_PROPERTY(GPSRecordingController* recordingController READ recordingController CONSTANT)
    Q_PROPERTY(GPSSatelliteModel* satelliteModel READ satelliteModel CONSTANT)
    Q_PROPERTY(GPSSatelliteModel* nmeaSatelliteModel READ nmeaSatelliteModel CONSTANT)
    Q_PROPERTY(GPSRelativePositionModel* relativePositionModel READ relativePositionModel CONSTANT)
    Q_PROPERTY(NMEASourceManager* nmeaConnection READ nmeaConnection CONSTANT)
    Q_PROPERTY(GPSReceiverAutoConnect* rtkConnection READ rtkConnection CONSTANT)
    Q_PROPERTY(GPSCorrectionManager* corrections READ corrections CONSTANT)
    Q_PROPERTY(bool networkRtkActive READ networkRtkActive NOTIFY networkRtkActiveChanged)
    Q_PROPERTY(
        bool networkRtkAutoConnectPaused READ networkRtkAutoConnectPaused NOTIFY networkRtkAutoConnectPausedChanged)

    friend class GPSManagerTest;

public:
    GPSManager(QObject *parent = nullptr);
    GPSManager(SettingsManager& settings, QGCPositionManager* positionManager,
               std::function<bool()> connectionsSuspended, QObject* parent = nullptr);
    ~GPSManager();

    static GPSManager *instance();

    void init(NTRIPManager* ntrip = nullptr);
    void shutdown();

    GPSReceiver* receiver() const { return _receiver; }

    GPSReceiverSession* receiverSession() { return &_receiverSession; }

    NMEASourceManager* nmeaConnection() const { return _nmeaSources; }

    GPSReceiverAutoConnect* rtkConnection() const { return _receiverAutoConnect; }

    GPSCorrectionManager* corrections() { return &_corrections; }

    QVariantList receiverSettings() const;
    QVariantList configurationReport() const;

    bool configurationReportActive() const { return _receiverSession.configurationReport().active; }

    GPSRecordingController* recordingController() { return &_recording; }

    GPSSatelliteModel* satelliteModel() { return &_satellites; }

    GPSSatelliteModel* nmeaSatelliteModel() { return &_nmeaSatellites; }

    GPSRelativePositionModel* relativePositionModel() { return &_relativePosition; }

    Q_INVOKABLE bool connectNmea();
    Q_INVOKABLE void disconnectNmea();
    Q_INVOKABLE bool connectRtk();
    Q_INVOKABLE void disconnectRtk();

    bool networkRtkActive() const;
    bool networkRtkAutoConnectPaused() const;

    Q_INVOKABLE bool connectNetworkRtk();
    Q_INVOKABLE void disconnectNetworkRtk();

signals:
    void configurationReportChanged();
    void receiverSettingsChanged();
    void networkRtkActiveChanged();
    void networkRtkAutoConnectPausedChanged();

private:
    void _updateConnections();
    void _updateReceiverSettings(bool restart = false);
    void _updatePositionSource();
    void _updateNmeaPositionSource();
    void _updatePositionSourceMode();
    void _updateNmeaSatellites();
    void _updateCorrectionSettings();
    void _updateNtripUdpOutput();
    SettingsManager& _settings;
    std::function<bool()> _connectionsSuspended;
    QPointer<NTRIPManager> _ntrip;
    quint64 _correctionDestinationSession = 0;
    quint64 _ntripAttemptId = 0;
    bool _shutdown = false;
    quint64 _receiverSettingsRevision = 0;
    quint64 _receiverRegistrationRevision = 0;
    quint64 _nmeaRegistrationRevision = 0;
    QPointer<QGeoPositionInfoSource> _registeredReceiverSource;
    QPointer<QGeoPositionInfoSource> _registeredNmeaSource;
    std::unique_ptr<GPSPositionSourceRegistration> _receiverRegistration;
    std::unique_ptr<GPSPositionSourceRegistration> _nmeaRegistration;
    QPointer<QGCPositionManager> _positionManager;
    GPSCorrectionManager _corrections;
    GPSRecordingController _recording;
    GPSSatelliteModel _satellites;
    GPSSatelliteModel _nmeaSatellites;
    GPSRelativePositionModel _relativePosition;
    GPSReceiverSession _receiverSession;
    QTimer* _connectionTimer = nullptr;
    NMEASourceManager* _nmeaSources = nullptr;
    GPSReceiverAutoConnect* _receiverAutoConnect = nullptr;
    GPSReceiver* _receiver = nullptr;
    GPSBaseStationState* _baseStationState = nullptr;
};
