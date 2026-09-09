#pragma once

#include <QtCore/QChronoTimer>
#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtQmlIntegration/QtQmlIntegration>

#include "NTRIPConnectionStats.h"
#include "NTRIPGgaProvider.h"
#include "NTRIPSession.h"
#include "NTRIPSourceTableController.h"
#include "UdpForwarder.h"

Q_DECLARE_LOGGING_CATEGORY(NTRIPManagerLog)
class NTRIPSettings;

/// Settings and QML facade; NTRIPSession owns the caster connection lifecycle.
class NTRIPManager : public QObject
{
    Q_OBJECT
    friend class NTRIPManagerTest;
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_MOC_INCLUDE("NTRIPConnectionStats.h")
    Q_MOC_INCLUDE("NTRIPSourceTableController.h")
    Q_PROPERTY(ConnectionStatus connectionStatus READ connectionStatus NOTIFY connectionStatusChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(QString securityWarning READ securityWarning NOTIFY securityWarningChanged)
    Q_PROPERTY(CasterStatus casterStatus READ casterStatus NOTIFY casterStatusChanged)
    Q_PROPERTY(QString ggaSource READ ggaSource NOTIFY ggaSourceChanged)
    Q_PROPERTY(NTRIPSourceTableController* sourceTableController READ sourceTableController CONSTANT)
    Q_PROPERTY(NTRIPConnectionStats* connectionStats READ connectionStats CONSTANT)

public:
    enum class ConnectionStatus
    {
        Disconnected = 0,
        Connecting = 1,
        Connected = 2,
        Reconnecting = 3,
        Error = 4
    };
    Q_ENUM(ConnectionStatus)
    enum class CasterStatus
    {
        CasterConnected,
        CasterNoLocation,
        CasterError
    };
    Q_ENUM(CasterStatus)

    explicit NTRIPManager(QObject* parent = nullptr);
    ~NTRIPManager() override;
    static NTRIPManager* instance();
    void init();

    ConnectionStatus connectionStatus() const { return _connectionStatus; }
    QString statusMessage() const { return _statusMessage; }
    QString securityWarning() const { return _securityWarning; }
    CasterStatus casterStatus() const { return _casterStatus; }
    QString ggaSource() const { return _ggaProvider.currentSource(); }

    QString correctionSourceId() const { return _session.sourceId(); }
    NTRIPSourceTableController* sourceTableController() { return &_sourceTableController; }
    NTRIPConnectionStats* connectionStats() { return &_stats; }

    Q_INVOKABLE void fetchMountpoints();

    Q_INVOKABLE void selectMountpoint(const QString& mountpoint)
    {
        _sourceTableController.selectMountpoint(mountpoint);
    }

    void setTransportForTest(NTRIPStream* stream) { _session.setStreamForTest(stream); }
    void startNTRIP();
    void stopNTRIP();

signals:
    void rtcmDataReceived(const QByteArray& data);
    void correctionReceived(const QByteArray& data, int messageId, bool filtered);
    void correctionReceivedAt(const QByteArray& data, int messageId, bool filtered, qint64 receivedAtMs);
    void correctionSessionStarted();
    void correctionSessionEnded();
    void connectionStatusChanged();
    void statusMessageChanged();
    void securityWarningChanged();
    void casterStatusChanged(CasterStatus status);
    void ggaSourceChanged();

private:
    void _onSessionState(NTRIPSession::State state, const QString& message);
    void _onCorrection(const QByteArray& data, int messageId, bool filtered, qint64 receivedAtMs);
    void _applyUdpForwarderConfig(const NTRIPTransportConfig& config);
    void _onPlaintextCredentialsWarning();
    void _setSecurityWarning(const QString& warning);
    void _onSettingChanged();
    bool _isEnabled() const;

    NTRIPSession _session;
    NTRIPGgaProvider _ggaProvider{this};
    NTRIPConnectionStats _stats{this};
    UdpForwarder _udpForwarder{this};
    NTRIPSourceTableController _sourceTableController{this};
    QChronoTimer _settingsDebounceTimer{this};
    NTRIPTransportConfig _runningConfig;
    QPointer<NTRIPSettings> _settings;
    ConnectionStatus _connectionStatus = ConnectionStatus::Disconnected;
    CasterStatus _casterStatus = CasterStatus::CasterError;
    QString _statusMessage;
    QString _securityWarning;
    quint64 _revision = 0;
    bool _initialized = false;
};
