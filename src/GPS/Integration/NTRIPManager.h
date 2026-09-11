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

    quint64 correctionAttemptId() const { return _session.activeAttemptId(); }
    NTRIPSourceTableController* sourceTableController() { return &_sourceTableController; }
    NTRIPConnectionStats* connectionStats() { return &_stats; }

    Q_INVOKABLE void fetchMountpoints();

    Q_INVOKABLE void selectMountpoint(const QString& mountpoint)
    {
        _sourceTableController.selectMountpoint(mountpoint);
    }

    void setTransportForTest(NTRIPStream* stream) { _session.setStreamForTest(stream); }

    void setPositionProvider(NTRIPGgaProvider::PositionSource source, NTRIPGgaProvider::PositionProvider provider)
    {
        _ggaProvider.setPositionProvider(source, std::move(provider));
    }
    void startNTRIP();
    void stopNTRIP();

signals:
    void correctionReceivedAt(const QByteArray& data, int messageId, bool filtered, qint64 receivedAtMs,
                              quint64 attemptId);
    void correctionRejectedAt(const QByteArray& data, int messageId, qint64 receivedAtMs, quint64 attemptId);
    void correctionSessionStarted(quint64 attemptId, const QString& sourceId);
    void correctionSessionEnded(quint64 attemptId);
    void connectionStatusChanged();
    void statusMessageChanged();
    void securityWarningChanged();
    void casterStatusChanged(CasterStatus status);
    void ggaSourceChanged();

private:
    void _onSessionState(NTRIPSession::State state, const QString& message);
    void _onCorrection(const QByteArray& data, int messageId, bool filtered, qint64 receivedAtMs, quint64 attemptId);
    void _onPlaintextCredentialsWarning();
    void _setSecurityWarning(const QString& warning);
    void _onSettingChanged();
    bool _isEnabled() const;

    NTRIPSession _session;
    NTRIPGgaProvider _ggaProvider{this};
    NTRIPConnectionStats _stats{this};
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
