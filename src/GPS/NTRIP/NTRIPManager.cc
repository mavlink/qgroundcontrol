#include "NTRIPManager.h"
#include "NTRIPHttpTransport.h"
#include "NTRIPSourceTable.h"
#include "NTRIPSettings.h"
#include "QmlObjectListModel.h"
#include "Fact.h"
#include "FactGroup.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "RTCMMavlink.h"
#include "SettingsManager.h"

#include "MultiVehicleManager.h"
#include "PositionManager.h"
#include "Vehicle.h"
#include <QtPositioning/QGeoCoordinate>
#include <QtCore/QDateTime>

#include <QtCore/QApplicationStatic>
#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QtGlobal>

QGC_LOGGING_CATEGORY(NTRIPManagerLog, "GPS.NTRIPManager")

Q_APPLICATION_STATIC(NTRIPManager, _ntripManagerInstance);

NTRIPManager* NTRIPManager::instance()
{
    return _ntripManagerInstance();
}

NTRIPManager::NTRIPManager(QObject* parent)
    : QObject(parent)
{
    qCDebug(NTRIPManagerLog) << "NTRIPManager created";

    _rtcmMavlink = qgcApp() ? qgcApp()->findChild<RTCMMavlink*>() : nullptr;
    if (!_rtcmMavlink) {
        QObject* parentObj = qgcApp() ? static_cast<QObject*>(qgcApp()) : static_cast<QObject*>(this);
        _rtcmMavlink = new RTCMMavlink(parentObj);
        _rtcmMavlink->setObjectName(QStringLiteral("RTCMMavlink"));
    }

    NTRIPSettings* settings = SettingsManager::instance()->ntripSettings();
    if (settings) {
        auto connectSetting = [this](Fact* fact) {
            if (fact) {
                connect(fact, &Fact::rawValueChanged, this, &NTRIPManager::_onSettingChanged);
            }
        };
        connectSetting(settings->ntripServerConnectEnabled());
        connectSetting(settings->ntripServerHostAddress());
        connectSetting(settings->ntripServerPort());
        connectSetting(settings->ntripUsername());
        connectSetting(settings->ntripPassword());
        connectSetting(settings->ntripMountpoint());
        connectSetting(settings->ntripWhitelist());
        connectSetting(settings->ntripUseTls());
        connectSetting(settings->ntripUdpForwardEnabled());
        connectSetting(settings->ntripUdpTargetAddress());
        connectSetting(settings->ntripUdpTargetPort());
    }

    // Check initial state (handles connect-on-start)
    QTimer::singleShot(0, this, &NTRIPManager::_onSettingChanged);

    _ggaTimer = new QTimer(this);
    _ggaTimer->setInterval(5000);
    connect(_ggaTimer, &QTimer::timeout, this, &NTRIPManager::_sendGGA);

    _dataRateTimer = new QTimer(this);
    _dataRateTimer->setInterval(1000);
    connect(_dataRateTimer, &QTimer::timeout, this, [this]() {
        const double rate = static_cast<double>(_bytesReceived - _dataRatePrevBytes);
        _dataRatePrevBytes = _bytesReceived;
        if (!qFuzzyCompare(_dataRateBytesPerSec + 1.0, rate + 1.0)) {
            _dataRateBytesPerSec = rate;
            emit dataRateChanged();
        }
        emit bytesReceivedChanged();
        emit messagesReceivedChanged();
    });

    connect(qApp, &QCoreApplication::aboutToQuit, this, &NTRIPManager::stopNTRIP, Qt::QueuedConnection);
}

NTRIPManager::~NTRIPManager()
{
    qCDebug(NTRIPManagerLog) << "NTRIPManager destroyed";
    stopNTRIP();
}

void NTRIPManager::_setStatus(ConnectionStatus status, const QString& msg)
{
    bool changed = false;

    if (_connectionStatus != status) {
        _connectionStatus = status;
        changed = true;
        emit connectionStatusChanged();
    }

    if (_statusMessage != msg) {
        _statusMessage = msg;
        changed = true;
        emit statusMessageChanged();
    }

    if (changed) {
        qCDebug(NTRIPManagerLog) << "NTRIP status:" << static_cast<int>(status) << msg;
    }
}

NTRIPTransportConfig NTRIPManager::_configFromSettings() const
{
    NTRIPTransportConfig config;
    NTRIPSettings* settings = SettingsManager::instance()->ntripSettings();
    if (!settings) {
        return config;
    }

    if (settings->ntripServerHostAddress()) config.host = settings->ntripServerHostAddress()->rawValue().toString();
    if (settings->ntripServerPort())        config.port = settings->ntripServerPort()->rawValue().toInt();
    if (settings->ntripUsername())          config.username = settings->ntripUsername()->rawValue().toString();
    if (settings->ntripPassword())          config.password = settings->ntripPassword()->rawValue().toString();
    if (settings->ntripMountpoint())        config.mountpoint = settings->ntripMountpoint()->rawValue().toString();
    if (settings->ntripWhitelist())         config.whitelist = settings->ntripWhitelist()->rawValue().toString();
    if (settings->ntripUseTls())            config.useTls = settings->ntripUseTls()->rawValue().toBool();

    return config;
}

void NTRIPManager::startNTRIP()
{
    if (_startStopBusy || _transport) {
        if (_startStopBusy) {
            qCWarning(NTRIPManagerLog) << "startNTRIP called while start/stop in progress";
        }
        return;
    }
    _startStopBusy = true;

    if (_reconnectTimer) {
        _reconnectTimer->stop();
    }

    NTRIPTransportConfig config = _configFromSettings();

    NTRIPSettings* settings = SettingsManager::instance()->ntripSettings();
    if (settings && settings->ntripUdpForwardEnabled()) {
        _udpForwardEnabled = settings->ntripUdpForwardEnabled()->rawValue().toBool();
    }

    if (_udpForwardEnabled && settings) {
        const QString udpAddr = settings->ntripUdpTargetAddress()->rawValue().toString();
        const quint16 udpPort = static_cast<quint16>(settings->ntripUdpTargetPort()->rawValue().toUInt());
        const QHostAddress parsedAddr(udpAddr);
        if (!udpAddr.isEmpty() && !parsedAddr.isNull() && udpPort > 0) {
            _udpTargetAddress = parsedAddr;
            _udpTargetPort = udpPort;
            if (!_udpSocket) {
                _udpSocket = new QUdpSocket(this);
            }
            qCDebug(NTRIPManagerLog) << "NTRIP UDP forwarding enabled:" << udpAddr << ":" << udpPort;
        } else {
            qCWarning(NTRIPManagerLog) << "NTRIP UDP forwarding enabled but address/port invalid";
            _udpForwardEnabled = false;
        }
    }

    if (config.host.isEmpty()) {
        qCWarning(NTRIPManagerLog) << "NTRIP host address is empty";
        _setStatus(ConnectionStatus::Error, tr("No host address"));
        _startStopBusy = false;
        return;
    }

    if (config.port <= 0 || config.port > 65535) {
        qCWarning(NTRIPManagerLog) << "NTRIP port is invalid:" << config.port;
        _setStatus(ConnectionStatus::Error, tr("Invalid port"));
        _startStopBusy = false;
        return;
    }

    qCDebug(NTRIPManagerLog) << "startNTRIP: host=" << config.host << " port=" << config.port
                             << " mount=" << config.mountpoint;

    _setStatus(ConnectionStatus::Connecting, tr("Connecting to %1:%2...").arg(config.host).arg(config.port));

    _bytesReceived = 0;
    _messagesReceived = 0;
    _dataRateBytesPerSec = 0.0;
    _dataRatePrevBytes = 0;
    emit bytesReceivedChanged();
    emit messagesReceivedChanged();
    emit dataRateChanged();

    _runningConfig = config;
    _runningUdpForward = _udpForwardEnabled;
    _runningUdpAddr = _udpForwardEnabled ? _udpTargetAddress.toString() : QString();
    _runningUdpPort = _udpForwardEnabled ? _udpTargetPort : 0;

    _transport = new NTRIPHttpTransport(config, this);

    connect(_transport, &NTRIPHttpTransport::error,
            this, &NTRIPManager::_tcpError);

    connect(_transport, &NTRIPHttpTransport::connected, this, [this]() {
        _reconnectAttempts = 0;
        _casterStatus = CasterStatus::CasterConnected;
        emit casterStatusChanged(_casterStatus);

        _setStatus(ConnectionStatus::Connected, tr("Connected"));

        if (_ggaTimer && !_ggaTimer->isActive()) {
            _ggaTimer->setInterval(1000);
            _sendGGA();
            _ggaTimer->start();
        }
        _dataRateTimer->start();
    });

    connect(_transport, &NTRIPHttpTransport::RTCMDataUpdate, this, &NTRIPManager::_rtcmDataReceived);

    _transport->start();
    qCDebug(NTRIPManagerLog) << "NTRIP started";

    _startStopBusy = false;
}

void NTRIPManager::stopNTRIP()
{
    if (_startStopBusy) {
        qCWarning(NTRIPManagerLog) << "stopNTRIP called while start/stop in progress";
        return;
    }
    _startStopBusy = true;

    if (_reconnectTimer) {
        _reconnectTimer->stop();
    }

    if (_transport) {
        disconnect(_transport, &NTRIPHttpTransport::error, this, &NTRIPManager::_tcpError);
        _transport->stop();
        _transport->deleteLater();
        _transport = nullptr;

        _runningConfig = {};
        _runningUdpForward = false;
        _runningUdpAddr.clear();
        _runningUdpPort = 0;

        _setStatus(ConnectionStatus::Disconnected, tr("Disconnected"));
        qCDebug(NTRIPManagerLog) << "NTRIP stopped";
    }

    if (_ggaTimer && _ggaTimer->isActive()) {
        _ggaTimer->stop();
    }
    _dataRateTimer->stop();
    if (_dataRateBytesPerSec != 0.0) {
        _dataRateBytesPerSec = 0.0;
        _dataRatePrevBytes = 0;
        emit dataRateChanged();
    }

    if (_udpSocket) {
        _udpSocket->close();
        delete _udpSocket;
        _udpSocket = nullptr;
    }
    _udpForwardEnabled = false;

    _startStopBusy = false;
}

void NTRIPManager::_tcpError(const QString& errorMsg)
{
    qCWarning(NTRIPManagerLog) << "NTRIP error:" << errorMsg;
    _setStatus(ConnectionStatus::Error, errorMsg);

    if (errorMsg.contains("NO_LOCATION_PROVIDED", Qt::CaseInsensitive)) {
        _casterStatus = CasterStatus::CasterNoLocation;
    } else {
        _casterStatus = CasterStatus::CasterError;
    }
    emit casterStatusChanged(_casterStatus);

    _scheduleReconnect();
}

void NTRIPManager::_scheduleReconnect()
{
    NTRIPSettings* settings = SettingsManager::instance()->ntripSettings();
    bool shouldRun = settings && settings->ntripServerConnectEnabled() &&
                     settings->ntripServerConnectEnabled()->rawValue().toBool();
    if (!shouldRun) {
        return;
    }

    stopNTRIP();

    int backoffMs = qMin(kMinReconnectMs * (1 << qMin(_reconnectAttempts, 4)), kMaxReconnectMs);
    _reconnectAttempts = qMin(_reconnectAttempts + 1, kMaxReconnectAttempts);

    qCDebug(NTRIPManagerLog) << "NTRIP reconnecting in" << backoffMs << "ms (attempt" << _reconnectAttempts << ")";

    _setStatus(ConnectionStatus::Reconnecting, tr("Reconnecting in %1s...").arg(backoffMs / 1000));

    if (!_reconnectTimer) {
        _reconnectTimer = new QTimer(this);
        _reconnectTimer->setSingleShot(true);
        connect(_reconnectTimer, &QTimer::timeout, this, [this]() {
            if (!_transport) {
                startNTRIP();
            }
        });
    }

    _reconnectTimer->start(backoffMs);
}

void NTRIPManager::_rtcmDataReceived(const QByteArray& data)
{
    _bytesReceived += static_cast<quint64>(data.size());
    _messagesReceived++;

    qCDebug(NTRIPManagerLog) << "NTRIP Forwarding RTCM to vehicle:" << data.size() << "bytes";

    if (!_rtcmMavlink && qgcApp()) {
        _rtcmMavlink = qgcApp()->findChild<RTCMMavlink*>();
    }

    if (_rtcmMavlink) {
        _rtcmMavlink->RTCMDataUpdate(data);

        if (_connectionStatus != ConnectionStatus::Connected) {
            _setStatus(ConnectionStatus::Connected, tr("Connected"));
        }
    } else {
        qCWarning(NTRIPManagerLog) << "RTCMMavlink not ready; dropping" << data.size() << "bytes";
    }

    if (_udpForwardEnabled && _udpSocket && _udpTargetPort > 0) {
        const qint64 sent = _udpSocket->writeDatagram(data, _udpTargetAddress, _udpTargetPort);
        if (sent < 0) {
            qCWarning(NTRIPManagerLog) << "NTRIP UDP forward failed:" << _udpSocket->errorString();
        }
    }
}

QByteArray NTRIPManager::makeGGA(const QGeoCoordinate& coord, double altitude_msl)
{
    const QTime utc = QDateTime::currentDateTimeUtc().time();
    const QString hhmmss = QString("%1%2%3")
        .arg(utc.hour(),   2, 10, QChar('0'))
        .arg(utc.minute(), 2, 10, QChar('0'))
        .arg(utc.second(), 2, 10, QChar('0'));

    auto dmm = [](double deg, bool lat) -> QString {
        double a = qFabs(deg);
        int d = int(a);
        double m = (a - d) * 60.0;

        int m10000 = int(m * 10000.0 + 0.5);
        double m_rounded = m10000 / 10000.0;
        if (m_rounded >= 60.0) {
            m_rounded -= 60.0;
            d += 1;
        }

        QString mm = QString::number(m_rounded, 'f', 4);
        if (m_rounded < 10.0) {
            mm.prepend("0");
        }

        if (lat) {
            return QString("%1%2").arg(d, 2, 10, QChar('0')).arg(mm);
        } else {
            return QString("%1%2").arg(d, 3, 10, QChar('0')).arg(mm);
        }
    };

    const bool latNorth = coord.latitude() >= 0.0;
    const bool lonEast  = coord.longitude() >= 0.0;

    const QString latField = dmm(coord.latitude(), true);
    const QString lonField = dmm(coord.longitude(), false);

    const QString core = QString("GPGGA,%1,%2,%3,%4,%5,1,12,1.0,%6,M,0.0,M,,")
        .arg(hhmmss)
        .arg(latField)
        .arg(latNorth ? "N" : "S")
        .arg(lonField)
        .arg(lonEast  ? "E" : "W")
        .arg(QString::number(altitude_msl, 'f', 1));

    quint8 cksum = 0;
    const QByteArray coreBytes = core.toUtf8();
    for (char ch : coreBytes) {
        cksum ^= static_cast<quint8>(ch);
    }

    const QString cks = QString("%1").arg(cksum, 2, 16, QChar('0')).toUpper();
    const QByteArray sentence = QByteArray("$") + coreBytes + QByteArray("*") + cks.toUtf8();
    return sentence;
}

QPair<QGeoCoordinate, QString> NTRIPManager::_getBestPosition() const
{
    MultiVehicleManager* mvm = MultiVehicleManager::instance();
    if (mvm) {
        if (Vehicle* veh = mvm->activeVehicle()) {
            FactGroup* gps = veh->gpsFactGroup();
            if (gps) {
                Fact* latF = gps->getFact(QStringLiteral("lat"));
                Fact* lonF = gps->getFact(QStringLiteral("lon"));

                if (latF && lonF) {
                    const double lat = latF->rawValue().toDouble();
                    const double lon = lonF->rawValue().toDouble();

                    if (qIsFinite(lat) && qIsFinite(lon) &&
                        !(lat == 0.0 && lon == 0.0) &&
                        qAbs(lat) <= 90.0 && qAbs(lon) <= 180.0) {

                        return {QGeoCoordinate(lat, lon, veh->coordinate().altitude()),
                                QStringLiteral("GPS Raw")};
                    }
                }
            }

            QGeoCoordinate coord = veh->coordinate();
            if (coord.isValid() && !(coord.latitude() == 0.0 && coord.longitude() == 0.0)) {
                return {coord, QStringLiteral("Vehicle EKF")};
            }
        }
    }

    QGCPositionManager* posMgr = QGCPositionManager::instance();
    if (posMgr) {
        QGeoCoordinate coord = posMgr->gcsPosition();
        if (coord.isValid() && !(coord.latitude() == 0.0 && coord.longitude() == 0.0)) {
            return {coord, QStringLiteral("GCS Position")};
        }
    }

    return {QGeoCoordinate(), QString()};
}

void NTRIPManager::_sendGGA()
{
    if (!_transport) {
        return;
    }

    const auto [coord, srcUsed] = _getBestPosition();

    if (!coord.isValid()) {
        qCDebug(NTRIPManagerLog) << "NTRIP: No valid position, skipping GGA";
        return;
    }

    if (_ggaTimer && _ggaTimer->interval() != 5000) {
        _ggaTimer->setInterval(5000);
        qCDebug(NTRIPManagerLog) << "NTRIP: Position acquired, reducing GGA to 5s";
    }

    double alt_msl = coord.altitude();
    if (!qIsFinite(alt_msl)) {
        alt_msl = 0.0;
    }

    const QByteArray gga = makeGGA(coord, alt_msl);
    _transport->sendNMEA(gga);

    if (!srcUsed.isEmpty() && srcUsed != _ggaSource) {
        _ggaSource = srcUsed;
        emit ggaSourceChanged();
    }
}

void NTRIPManager::_onSettingChanged()
{
    if (_startStopBusy) {
        return;
    }

    NTRIPSettings* settings = SettingsManager::instance()->ntripSettings();
    bool shouldRun = settings && settings->ntripServerConnectEnabled() &&
                     settings->ntripServerConnectEnabled()->rawValue().toBool();
    bool isRunning = (_transport != nullptr);

    if (shouldRun && isRunning && settings) {
        NTRIPTransportConfig config = _configFromSettings();
        const bool udpFwd = settings->ntripUdpForwardEnabled()->rawValue().toBool();
        const QString udpAddr = settings->ntripUdpTargetAddress()->rawValue().toString();
        const quint16 udpPort = static_cast<quint16>(settings->ntripUdpTargetPort()->rawValue().toUInt());

        if (config != _runningConfig ||
            udpFwd != _runningUdpForward || udpAddr != _runningUdpAddr || udpPort != _runningUdpPort) {
            qCDebug(NTRIPManagerLog) << "NTRIP settings changed while running, restarting";
            stopNTRIP();
            startNTRIP();
            return;
        }
    }

    const bool reconnectPending = _reconnectTimer && _reconnectTimer->isActive();

    if (shouldRun && !isRunning && !reconnectPending) {
        startNTRIP();
    } else if (!shouldRun) {
        if (_reconnectTimer) {
            _reconnectTimer->stop();
        }
        if (isRunning) {
            stopNTRIP();
        } else if (reconnectPending) {
            _reconnectAttempts = 0;
            _setStatus(ConnectionStatus::Disconnected, tr("Disconnected"));
        }
    }
}

QmlObjectListModel* NTRIPManager::mountpointModel() const
{
    if (_sourceTableModel) {
        return _sourceTableModel->mountpoints();
    }
    return nullptr;
}

void NTRIPManager::fetchMountpoints()
{
    if (_mountpointFetchStatus == MountpointFetchStatus::FetchInProgress) {
        return;
    }

    if (_sourceTableModel && _sourceTableModel->count() > 0 && _sourceTableFetchedAtMs > 0) {
        const qint64 age = QDateTime::currentMSecsSinceEpoch() - _sourceTableFetchedAtMs;
        if (age < kSourceTableCacheTtlMs) {
            qCDebug(NTRIPManagerLog) << "Source table cache hit, age:" << age << "ms";
            _mountpointFetchStatus = MountpointFetchStatus::FetchSuccess;
            emit mountpointFetchStatusChanged();
            return;
        }
    }

    NTRIPSettings* settings = SettingsManager::instance()->ntripSettings();
    if (!settings) {
        return;
    }

    const QString host = settings->ntripServerHostAddress()->rawValue().toString();
    const int port = settings->ntripServerPort()->rawValue().toInt();
    const QString user = settings->ntripUsername()->rawValue().toString();
    const QString pass = settings->ntripPassword()->rawValue().toString();
    const bool useTls = settings->ntripUseTls() ? settings->ntripUseTls()->rawValue().toBool() : false;

    if (host.isEmpty()) {
        _mountpointFetchError = tr("Host address is empty");
        _mountpointFetchStatus = MountpointFetchStatus::FetchError;
        emit mountpointFetchErrorChanged();
        emit mountpointFetchStatusChanged();
        return;
    }

    if (!_sourceTableModel) {
        _sourceTableModel = new NTRIPSourceTableModel(this);
        emit mountpointModelChanged();
    }

    if (_sourceTableFetcher) {
        _sourceTableFetcher->deleteLater();
        _sourceTableFetcher = nullptr;
    }

    _mountpointFetchStatus = MountpointFetchStatus::FetchInProgress;
    _mountpointFetchError.clear();
    emit mountpointFetchStatusChanged();
    emit mountpointFetchErrorChanged();

    _sourceTableFetcher = new NTRIPSourceTableFetcher(host, port, user, pass, useTls, this);

    connect(_sourceTableFetcher, &NTRIPSourceTableFetcher::sourceTableReceived, this, [this](const QString& table) {
        _sourceTableModel->parseSourceTable(table);
        _sourceTableFetchedAtMs = QDateTime::currentMSecsSinceEpoch();

        MultiVehicleManager* mvm = MultiVehicleManager::instance();
        if (mvm) {
            if (Vehicle* veh = mvm->activeVehicle()) {
                QGeoCoordinate coord = veh->coordinate();
                if (coord.isValid()) {
                    _sourceTableModel->updateDistances(coord);
                }
            }
        }

        _mountpointFetchStatus = MountpointFetchStatus::FetchSuccess;
        emit mountpointFetchStatusChanged();
        qCDebug(NTRIPManagerLog) << "NTRIP source table fetched:" << _sourceTableModel->count() << "mountpoints";
    });

    connect(_sourceTableFetcher, &NTRIPSourceTableFetcher::error, this, [this](const QString& errorMsg) {
        _mountpointFetchError = errorMsg;
        _mountpointFetchStatus = MountpointFetchStatus::FetchError;
        emit mountpointFetchErrorChanged();
        emit mountpointFetchStatusChanged();
        qCWarning(NTRIPManagerLog) << "NTRIP source table fetch error:" << errorMsg;
    });

    connect(_sourceTableFetcher, &NTRIPSourceTableFetcher::finished, this, [this]() {
        _sourceTableFetcher->deleteLater();
        _sourceTableFetcher = nullptr;
    });

    _sourceTableFetcher->fetch();
}

void NTRIPManager::selectMountpoint(const QString& mountpoint)
{
    NTRIPSettings* settings = SettingsManager::instance()->ntripSettings();
    if (settings && settings->ntripMountpoint()) {
        settings->ntripMountpoint()->setRawValue(mountpoint);
        qCDebug(NTRIPManagerLog) << "NTRIP mountpoint selected:" << mountpoint;
    }
}
