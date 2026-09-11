#pragma once

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtQmlIntegration/QtQmlIntegration>

#include <functional>
#include <memory>

#include "GPSCorrectionManager.h"
#include "GPSPositionSourceRegistration.h"
#include "GPSReceiverAutoConnect.h"
#include "GPSRecordingController.h"
#include "GPSRelativePositionModel.h"
#include "GPSSatelliteModel.h"
#include "NMEASourceManager.h"

class GPSReceiver;
class GPSSourceHealth;
class VehicleGPSPositionProvider;
class GPSBaseStationState;
class QGCPositionManager;
class SettingsManager;
class NTRIPManager;

class GPSManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_PROPERTY(bool canSaveBaseReference READ canSaveBaseReference NOTIFY baseReferenceSaveStateChanged)
    Q_PROPERTY(QString baseReferenceSaveError READ baseReferenceSaveError NOTIFY baseReferenceSaveStateChanged)
    Q_PROPERTY(QVariantList receiverSettings READ receiverSettings NOTIFY receiverSettingsChanged)
    Q_PROPERTY(QVariantList configurationReport READ configurationReport NOTIFY configurationReportChanged)
    Q_PROPERTY(bool configurationReportActive READ configurationReportActive NOTIFY configurationReportChanged)
    Q_PROPERTY(GPSRecordingController* recordingController READ recordingController CONSTANT)
    Q_PROPERTY(GPSSatelliteModel* satelliteModel READ satelliteModel CONSTANT)
    Q_PROPERTY(GPSSatelliteModel* nmeaSatelliteModel READ nmeaSatelliteModel CONSTANT)
    Q_PROPERTY(GPSRelativePositionModel* relativePositionModel READ relativePositionModel CONSTANT)
    Q_PROPERTY(NMEASourceManager* nmeaConnection READ nmeaConnection CONSTANT)
    Q_PROPERTY(GPSReceiverAutoConnect* receiverConnection READ receiverConnection CONSTANT)
    Q_PROPERTY(GPSCorrectionManager* corrections READ corrections CONSTANT)

    friend class GPSManagerTest;

public:
    GPSManager(QObject* parent = nullptr);
    GPSManager(SettingsManager& settings, QGCPositionManager* positionManager,
               std::function<bool()> connectionsSuspended, QObject* parent = nullptr);
    ~GPSManager();

    static GPSManager* instance();

    void init(NTRIPManager* ntrip = nullptr);
    void shutdown();

    GPSReceiver* receiver() const { return _receiver; }

    GPSReceiverSession* receiverSession() { return &_receiverSession; }

    NMEASourceManager* nmeaConnection() const { return _nmeaSources; }

    GPSReceiverAutoConnect* receiverConnection() const { return _receiverAutoConnect; }

    GPSCorrectionManager* corrections() { return &_corrections; }

    QVariantList receiverSettings() const;
    QVariantList configurationReport() const;

    bool configurationReportActive() const { return _receiverSession.configurationReport().active; }

    GPSRecordingController* recordingController() { return &_recording; }

    GPSSatelliteModel* satelliteModel() { return &_satellites; }

    GPSSatelliteModel* nmeaSatelliteModel() { return &_nmeaSatellites; }

    GPSRelativePositionModel* relativePositionModel() { return &_relativePosition; }

    bool canSaveBaseReference() const;
    QString baseReferenceSaveError() const;
    Q_INVOKABLE bool saveBaseReference();

    Q_INVOKABLE bool connectNmea();
    Q_INVOKABLE void disconnectNmea();
    Q_INVOKABLE bool connectReceiver();
    Q_INVOKABLE void disconnectReceiver();

signals:
    void baseReferenceSaveStateChanged();
    void configurationReportChanged();
    void receiverSettingsChanged();

private:
    void _updateConnections();
    void _updateReceiverSettings(bool restart = false);
    void _updateNmeaSettings();
    void _updatePositionSource();
    void _updateNmeaPositionSource();
    void _updatePositionSourceMode();
    void _updateNmeaSatellites();
    void _updateCorrectionSettings();
    void _startReceiverCorrections();
    void _stopReceiverCorrections();
    void _startNtripCorrections(quint64 attemptId, const QString& sourceId);
    void _stopNtripCorrections(quint64 attemptId);
    void _updateNtripUdpOutput();
    SettingsManager& _settings;
    std::function<bool()> _connectionsSuspended;
    QPointer<NTRIPManager> _ntrip;
    VehicleGPSPositionProvider* _vehiclePositionProvider = nullptr;
    quint64 _correctionDestinationSession = 0;
    quint64 _ntripAttemptId = 0;
    quint64 _receiverCorrectionAttemptId = 0;
    quint64 _receiverCorrectionRevision = 0;
    quint64 _ntripCorrectionRevision = 0;
    bool _savingBaseReference = false;
    bool _baseReferenceRestartPending = false;
    bool _shutdown = false;
    bool _initialized = false;
    quint64 _nmeaSettingsRevision = 0;
    quint64 _correctionSettingsRevision = 0;
    quint64 _receiverSettingsRevision = 0;

    struct PositionBinding
    {
        QPointer<QObject> source;
        quint64 session = 0;
        quint64 revision = 0;
        GPSPositionSourceRegistration registration;
    };

    PositionBinding _receiverBinding;
    PositionBinding _nmeaBinding;
    void _updatePositionBinding(PositionBinding& binding, int kind, QObject* source, GPSSourceHealth* health,
                                quint64 session);
    QPointer<QGCPositionManager> _positionManager;
    GPSCorrectionManager _corrections;
    GPSCorrectionSourceRegistration _receiverCorrectionSource;
    GPSCorrectionSourceRegistration _ntripCorrectionSource;
    GPSRecordingController _recording;
    GPSSatelliteModel _satellites;
    GPSSatelliteModel _nmeaSatellites;
    GPSRelativePositionModel _relativePosition;
    GPSReceiverSession _receiverSession;
    NMEASourceManager* _nmeaSources = nullptr;
    GPSReceiverAutoConnect* _receiverAutoConnect = nullptr;
    GPSReceiver* _receiver = nullptr;
    GPSBaseStationState* _baseStationState = nullptr;
};
