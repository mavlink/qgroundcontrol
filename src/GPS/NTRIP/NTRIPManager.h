#pragma once

#include "NTRIPConnectionStats.h"
#include "NTRIPGgaProvider.h"
#include "NTRIPReconnectPolicy.h"
#include "NTRIPSourceTableController.h"
#include "NTRIPTransportConfig.h"
#include "NTRIPUdpForwarder.h"

#include <functional>

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtQmlIntegration/QtQmlIntegration>

Q_DECLARE_LOGGING_CATEGORY(NTRIPManagerLog)

enum class NTRIPError;
class NTRIPSettings;

class NTRIPTransport;

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

    ConnectionStatus connectionStatus() const { return _connectionStatus; }
    QString statusMessage() const { return _statusMessage; }
    CasterStatus casterStatus() const { return _casterStatus; }
    QString ggaSource() const { return _ggaProvider.currentSource(); }
    NTRIPSourceTableController* sourceTableController() { return &_sourceTableController; }
    NTRIPConnectionStats* connectionStats() { return &_stats; }

    Q_INVOKABLE void fetchMountpoints();
    Q_INVOKABLE void selectMountpoint(const QString& mountpoint) { _sourceTableController.selectMountpoint(mountpoint); }

    void setTransportFactory(TransportFactory factory) { _transportFactory = std::move(factory); }

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
    bool _transition(OperationState from, OperationState to);

    NTRIPGgaProvider _ggaProvider{this};
    NTRIPConnectionStats _stats{this};
    NTRIPReconnectPolicy _reconnectPolicy{this};
    NTRIPUdpForwarder _udpForwarder{this};

    ConnectionStatus _connectionStatus = ConnectionStatus::Disconnected;
    QString _statusMessage;
    CasterStatus _casterStatus = CasterStatus::CasterError;

    TransportFactory _transportFactory;
    NTRIPTransport* _transport = nullptr;
    OperationState _opState = OperationState::Idle;

    NTRIPTransportConfig _runningConfig;

    NTRIPSourceTableController _sourceTableController{this};

    static constexpr int kSettingsDebounceMs = 250;
    QTimer _settingsDebounceTimer;
};
