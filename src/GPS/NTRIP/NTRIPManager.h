#pragma once

#include "NTRIPConnectionStats.h"
#include "NTRIPGgaProvider.h"
#include "NTRIPReconnectPolicy.h"
#include "NTRIPSourceTableController.h"
#include "NTRIPTransport.h"
#include "NTRIPTransportConfig.h"
#include "NTRIPUdpForwarder.h"

#include <functional>

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QTimer>
#include <QtQmlIntegration/QtQmlIntegration>

Q_DECLARE_LOGGING_CATEGORY(NTRIPManagerLog)

#include "RTCMMavlink.h"

class NTRIPSettings;
struct GPSEvent;

class NTRIPManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_MOC_INCLUDE("NTRIPConnectionStats.h")
    Q_MOC_INCLUDE("NTRIPSourceTableController.h")
    Q_PROPERTY(ConnectionStatus connectionStatus READ connectionStatus NOTIFY connectionStatusChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(CasterStatus casterStatus READ casterStatus NOTIFY casterStatusChanged)
    Q_PROPERTY(QString ggaSource READ ggaSource NOTIFY ggaSourceChanged)
    Q_PROPERTY(NTRIPSourceTableController* sourceTableController READ sourceTableController CONSTANT)
    Q_PROPERTY(NTRIPConnectionStats* connectionStats READ connectionStats CONSTANT)

public:
    using TransportFactory = std::function<NTRIPTransport*(const NTRIPTransportConfig&, QObject*)>;
    enum class ConnectionStatus {
        Disconnected,
        Connecting,
        Connected,
        Reconnecting,
        Error
    };
    Q_ENUM(ConnectionStatus)

    enum class CasterStatus { CasterConnected, CasterNoLocation, CasterError };
    Q_ENUM(CasterStatus)

    explicit NTRIPManager(QObject* parent = nullptr);
    ~NTRIPManager() override;

    static NTRIPManager* instance();

    /// Explicit post-construction init. Must be called from QGCApplication after
    /// SettingsManager, GPSManager, and the NTRIP dependencies are constructed —
    /// matches the pattern used by QGCPositionManager::init(). Do not rely on
    /// the singleton constructor for anything that touches other singletons.
    void init();

    ConnectionStatus connectionStatus() const { return _connectionStatus; }
    QString statusMessage() const { return _statusMessage; }
    CasterStatus casterStatus() const { return _casterStatus; }
    QString ggaSource() const { return _ggaProvider.currentSource(); }
    NTRIPSourceTableController* sourceTableController() { return &_sourceTableController; }
    NTRIPConnectionStats* connectionStats() { return &_stats; }

    Q_INVOKABLE void fetchMountpoints();
    Q_INVOKABLE void selectMountpoint(const QString& mountpoint) { _sourceTableController.selectMountpoint(mountpoint); }

    void setTransportFactory(TransportFactory factory) { _transportFactory = std::move(factory); }
    void setRtcmMavlink(RTCMMavlink *mavlink) { _rtcmMavlink = mavlink; }
    void setEventLogger(std::function<void(const GPSEvent&)> logger) { _eventLogger = std::move(logger); }

    void startNTRIP();
    void stopNTRIP();

signals:
    void connectionStatusChanged();
    void statusMessageChanged();
    void casterStatusChanged(CasterStatus status);
    void ggaSourceChanged();

private:
    enum class OperationState { Idle, Starting, Running, Stopping };

    void _transportError(NTRIPError code, const QString& detail);
    void _rtcmDataReceived(const QByteArray& data);
    void _onSettingChanged();
    void _setStatus(ConnectionStatus status, const QString& msg = {});
    void _logEvent(const GPSEvent &event);
    bool _transition(OperationState from, OperationState to);
    bool _isEnabled() const;

    NTRIPGgaProvider _ggaProvider{this};
    NTRIPConnectionStats _stats{this};
    NTRIPReconnectPolicy _reconnectPolicy{this};
    NTRIPUdpForwarder _udpForwarder{this};

    ConnectionStatus _connectionStatus = ConnectionStatus::Disconnected;
    QString _statusMessage;
    CasterStatus _casterStatus = CasterStatus::CasterError;

    TransportFactory _transportFactory;
    QPointer<NTRIPTransport> _transport;
    OperationState _opState = OperationState::Idle;

    QPointer<RTCMMavlink> _rtcmMavlink;
    std::function<void(const GPSEvent&)> _eventLogger;

    NTRIPTransportConfig _runningConfig;
    NTRIPSettings *_settings = nullptr;

    NTRIPSourceTableController _sourceTableController{this};

    static constexpr int kSettingsDebounceMs = 250;
    QTimer _settingsDebounceTimer;
    bool _initialized = false;
};
