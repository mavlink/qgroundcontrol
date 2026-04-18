#pragma once

#include "NTRIPHttpTransport.h"

#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QUdpSocket>
#include <QtPositioning/QGeoCoordinate>
#include <QtQmlIntegration/QtQmlIntegration>

class RTCMMavlink;
class NTRIPSettings;
class Vehicle;
class MultiVehicleManager;
class NTRIPSourceTableModel;
class NTRIPSourceTableFetcher;
class QmlObjectListModel;

class NTRIPManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_MOC_INCLUDE("QmlObjectListModel.h")
    Q_PROPERTY(ConnectionStatus connectionStatus READ connectionStatus NOTIFY connectionStatusChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(CasterStatus casterStatus READ casterStatus NOTIFY casterStatusChanged)
    Q_PROPERTY(QString ggaSource READ ggaSource NOTIFY ggaSourceChanged)
    Q_PROPERTY(QmlObjectListModel* mountpointModel READ mountpointModel NOTIFY mountpointModelChanged)
    Q_PROPERTY(MountpointFetchStatus mountpointFetchStatus READ mountpointFetchStatus NOTIFY mountpointFetchStatusChanged)
    Q_PROPERTY(QString mountpointFetchError READ mountpointFetchError NOTIFY mountpointFetchErrorChanged)
    Q_PROPERTY(quint64 bytesReceived READ bytesReceived NOTIFY bytesReceivedChanged)
    Q_PROPERTY(quint32 messagesReceived READ messagesReceived NOTIFY messagesReceivedChanged)
    Q_PROPERTY(double dataRateBytesPerSec READ dataRateBytesPerSec NOTIFY dataRateChanged)

public:
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

    enum class MountpointFetchStatus {
        FetchIdle,
        FetchInProgress,
        FetchSuccess,
        FetchError
    };
    Q_ENUM(MountpointFetchStatus)

    static constexpr int kMinReconnectMs = 1000;
    static constexpr int kMaxReconnectMs = 30000;
    static constexpr int kMaxReconnectAttempts = 100;
    static constexpr int kSourceTableCacheTtlMs = 60000;

    explicit NTRIPManager(QObject* parent = nullptr);
    ~NTRIPManager() override;

    static NTRIPManager* instance();

    ConnectionStatus connectionStatus() const { return _connectionStatus; }
    QString statusMessage() const { return _statusMessage; }
    CasterStatus casterStatus() const { return _casterStatus; }
    QString ggaSource() const { return _ggaSource; }
    QmlObjectListModel* mountpointModel() const;
    MountpointFetchStatus mountpointFetchStatus() const { return _mountpointFetchStatus; }
    QString mountpointFetchError() const { return _mountpointFetchError; }
    quint64 bytesReceived() const { return _bytesReceived; }
    quint32 messagesReceived() const { return _messagesReceived; }
    double dataRateBytesPerSec() const { return _dataRateBytesPerSec; }

    Q_INVOKABLE void fetchMountpoints();
    Q_INVOKABLE void selectMountpoint(const QString& mountpoint);

    void startNTRIP();
    void stopNTRIP();

    static QByteArray makeGGA(const QGeoCoordinate& coord, double altitude_msl);

signals:
    void connectionStatusChanged();
    void statusMessageChanged();
    void casterStatusChanged(CasterStatus status);
    void ggaSourceChanged();
    void mountpointFetchStatusChanged();
    void mountpointModelChanged();
    void mountpointFetchErrorChanged();
    void bytesReceivedChanged();
    void messagesReceivedChanged();
    void dataRateChanged();

private:
    void _tcpError(const QString& errorMsg);
    void _rtcmDataReceived(const QByteArray& data);
    void _onSettingChanged();
    void _sendGGA();
    void _scheduleReconnect();
    void _setStatus(ConnectionStatus status, const QString& msg = {});

    NTRIPTransportConfig _configFromSettings() const;
    QPair<QGeoCoordinate, QString> _getBestPosition() const;

    QTimer* _ggaTimer = nullptr;

    ConnectionStatus _connectionStatus = ConnectionStatus::Disconnected;
    QString _statusMessage;
    QString _ggaSource;

    NTRIPHttpTransport* _transport = nullptr;
    RTCMMavlink* _rtcmMavlink = nullptr;
    QTimer* _reconnectTimer = nullptr;
    int _reconnectAttempts = 0;
    bool _startStopBusy = false;

    CasterStatus _casterStatus = CasterStatus::CasterError;

    QUdpSocket* _udpSocket = nullptr;
    QHostAddress _udpTargetAddress;
    quint16 _udpTargetPort = 0;
    bool _udpForwardEnabled = false;

    NTRIPTransportConfig _runningConfig;
    bool _runningUdpForward = false;
    QString _runningUdpAddr;
    quint16 _runningUdpPort = 0;

    NTRIPSourceTableModel* _sourceTableModel = nullptr;
    NTRIPSourceTableFetcher* _sourceTableFetcher = nullptr;
    MountpointFetchStatus _mountpointFetchStatus = MountpointFetchStatus::FetchIdle;
    QString _mountpointFetchError;

    quint64 _bytesReceived = 0;
    quint32 _messagesReceived = 0;
    double _dataRateBytesPerSec = 0.0;
    quint64 _dataRatePrevBytes = 0;
    QTimer* _dataRateTimer = nullptr;

    qint64 _sourceTableFetchedAtMs = 0;
};
