/****************************************************************************
 *
 * MIST MAVIROV — ROV Companion for QGroundControl (implementation)
 *
 ****************************************************************************/

#include "ROVCompanionController.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QStandardPaths>
#include <QtCore/QTime>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QNetworkInterface>
#include <QtNetwork/QTcpSocket>

namespace {
constexpr int kProbeAttemptTimeoutMs = 1800;
constexpr int kChannelSettleMs       = 2500;
constexpr int kMaxLogLines           = 250;

QString jStr(const QJsonObject &o, const char *key, const QString &def)
{
    const QJsonValue v = o.value(QLatin1String(key));
    return v.isString() ? v.toString() : def;
}

int jInt(const QJsonObject &o, const char *key, int def)
{
    const QJsonValue v = o.value(QLatin1String(key));
    return v.isDouble() ? static_cast<int>(v.toDouble()) : def;
}

double jDouble(const QJsonObject &o, const char *key, double def)
{
    const QJsonValue v = o.value(QLatin1String(key));
    return v.isDouble() ? v.toDouble() : def;
}
} // namespace

ROVCompanionController::ROVCompanionController(QObject *parent)
    : QObject(parent)
{
    _loadConfig();

    _clawPulse = _clawMinPulse;     // claw starts fully closed
    _rollPulse = _rollCenterPulse;  // arm roll starts centered (0 deg)

    _probeAttemptTimer.setSingleShot(true);
    _probeAttemptTimer.setInterval(kProbeAttemptTimeoutMs);
    connect(&_probeAttemptTimer, &QTimer::timeout, this, &ROVCompanionController::_onProbeAttemptTimeout);

    _log(QStringLiteral("MAVIROV ROV Companion ready — config: %1").arg(_configPath));
    _log(QStringLiteral("Pi %1 | Teensy %2:%3 | %4 camera(s) from rtsp port %5")
             .arg(_piAddress, _teensyAddress)
             .arg(_teensyPort)
             .arg(_cameraCount)
             .arg(_rtspBasePort),
         QStringLiteral("dim"));
}

ROVCompanionController::~ROVCompanionController()
{
    // Quiet teardown — no logging/signals while the app is going away.
    _probeAttemptTimer.stop();
    if (_probeSocket) {
        _probeSocket->abort();
        delete _probeSocket;
        _probeSocket = nullptr;
    }
    for (QProcess **p : { &_mavlinkProc, &_cameraProc }) {
        if (*p) {
            disconnect(*p, nullptr, this, nullptr);
            (*p)->kill();
            (*p)->waitForFinished(1000);
            delete *p;
            *p = nullptr;
        }
    }
}

/*===========================================================================
 * Configuration
 *===========================================================================*/

QString ROVCompanionController::_locateConfigFile() const
{
    const QString fileName = QStringLiteral("ROVCompanion.json");

    const QString besideApp = QCoreApplication::applicationDirPath() + QLatin1Char('/') + fileName;
    if (QFile::exists(besideApp)) {
        return besideApp;
    }

    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (configDir.isEmpty()) {
        configDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    }
    return configDir + QLatin1Char('/') + fileName;
}

void ROVCompanionController::_writeDefaultConfig(const QString &path) const
{
    QJsonObject o;
    o.insert(QStringLiteral("pi_address"),           _piAddress);
    o.insert(QStringLiteral("pc_address"),           _pcAddressConfig);
    o.insert(QStringLiteral("qgc_mavlink_port"),     _qgcMavlinkPort);
    o.insert(QStringLiteral("extra_mavlink_port"),   _extraMavlinkPort);
    o.insert(QStringLiteral("mavlink_ssh_user"),     _mavSshUser);
    o.insert(QStringLiteral("mavlink_ssh_password"), _mavSshPassword);
    o.insert(QStringLiteral("camera_ssh_user"),      _camSshUser);
    o.insert(QStringLiteral("camera_ssh_password"),  _camSshPassword);
    o.insert(QStringLiteral("ssh_program"),          _sshProgramConfig);
    o.insert(QStringLiteral("pi_boot_timeout_s"),    _piBootTimeoutS);
    o.insert(QStringLiteral("pi_probe_interval_s"),  _piProbeIntervalS);
    o.insert(QStringLiteral("mavproxy_command"),     _mavproxyCommand);
    o.insert(QStringLiteral("camera_command"),       _cameraCommand);
    o.insert(QStringLiteral("rtsp_base_port"),       _rtspBasePort);
    o.insert(QStringLiteral("camera_count"),         _cameraCount);
    o.insert(QStringLiteral("teensy_address"),       _teensyAddress);
    o.insert(QStringLiteral("teensy_port"),          _teensyPort);
    o.insert(QStringLiteral("claw_min_pulse"),       _clawMinPulse);
    o.insert(QStringLiteral("claw_max_pulse"),       _clawMaxPulse);
    o.insert(QStringLiteral("roll_min_pulse"),       _rollMinPulse);
    o.insert(QStringLiteral("roll_max_pulse"),       _rollMaxPulse);
    o.insert(QStringLiteral("roll_center_pulse"),    _rollCenterPulse);
    o.insert(QStringLiteral("claw_max_cm"),          _clawMaxCm);
    o.insert(QStringLiteral("servo_big_step"),       _servoBigStep);
    o.insert(QStringLiteral("servo_small_step"),     _servoSmallStep);

    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile f(path);
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        f.write(QJsonDocument(o).toJson(QJsonDocument::Indented));
    }
}

void ROVCompanionController::_loadConfig()
{
    _configPath = _locateConfigFile();

    if (!QFile::exists(_configPath)) {
        _writeDefaultConfig(_configPath);
    }

    QFile f(_configPath);
    if (!f.open(QIODevice::ReadOnly)) {
        return; // keep built-in defaults
    }

    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject()) {
        return;
    }
    const QJsonObject o = doc.object();

    _piAddress         = jStr(o, "pi_address",           _piAddress);
    _pcAddressConfig   = jStr(o, "pc_address",           _pcAddressConfig);
    _qgcMavlinkPort    = jInt(o, "qgc_mavlink_port",     _qgcMavlinkPort);
    _extraMavlinkPort  = jInt(o, "extra_mavlink_port",   _extraMavlinkPort);
    _mavSshUser        = jStr(o, "mavlink_ssh_user",     _mavSshUser);
    _mavSshPassword    = jStr(o, "mavlink_ssh_password", _mavSshPassword);
    _camSshUser        = jStr(o, "camera_ssh_user",      _camSshUser);
    _camSshPassword    = jStr(o, "camera_ssh_password",  _camSshPassword);
    _sshProgramConfig  = jStr(o, "ssh_program",          _sshProgramConfig);
    _piBootTimeoutS    = jInt(o, "pi_boot_timeout_s",    _piBootTimeoutS);
    _piProbeIntervalS  = jDouble(o, "pi_probe_interval_s", _piProbeIntervalS);
    _mavproxyCommand   = jStr(o, "mavproxy_command",     _mavproxyCommand);
    _cameraCommand     = jStr(o, "camera_command",       _cameraCommand);
    _rtspBasePort      = jInt(o, "rtsp_base_port",       _rtspBasePort);
    _cameraCount       = qMax(1, jInt(o, "camera_count", _cameraCount));
    _teensyAddress     = jStr(o, "teensy_address",       _teensyAddress);
    _teensyPort        = jInt(o, "teensy_port",          _teensyPort);
    _clawMinPulse      = jInt(o, "claw_min_pulse",       _clawMinPulse);
    _clawMaxPulse      = jInt(o, "claw_max_pulse",       _clawMaxPulse);
    _rollMinPulse      = jInt(o, "roll_min_pulse",       _rollMinPulse);
    _rollMaxPulse      = jInt(o, "roll_max_pulse",       _rollMaxPulse);
    _rollCenterPulse   = jInt(o, "roll_center_pulse",    _rollCenterPulse);
    _clawMaxCm         = jDouble(o, "claw_max_cm",       _clawMaxCm);
    _servoBigStep      = jInt(o, "servo_big_step",       _servoBigStep);
    _servoSmallStep    = jInt(o, "servo_small_step",     _servoSmallStep);

    if (_extraMavlinkPort == _qgcMavlinkPort) {
        _extraMavlinkPort = (_qgcMavlinkPort != 14551) ? 14551 : 14552;
    }
}

void ROVCompanionController::_resolvePcAddress()
{
    const QString cfg = _pcAddressConfig.trimmed();
    if (!cfg.isEmpty() && (cfg.compare(QLatin1String("auto"), Qt::CaseInsensitive) != 0)) {
        _pcAddress = cfg;
        return;
    }

    const QHostAddress piHost(_piAddress);
    const quint32 piV4 = piHost.toIPv4Address();
    QString fallback;

    const QList<QHostAddress> all = QNetworkInterface::allAddresses();
    for (const QHostAddress &addr : all) {
        if ((addr.protocol() != QAbstractSocket::IPv4Protocol) || addr.isLoopback()) {
            continue;
        }
        if (fallback.isEmpty()) {
            fallback = addr.toString();
        }
        if (piV4 && ((addr.toIPv4Address() & 0xFFFFFF00u) == (piV4 & 0xFFFFFF00u))) {
            _pcAddress = addr.toString();
            _log(QStringLiteral("PC address auto-detected on Pi subnet: %1").arg(_pcAddress), QStringLiteral("ok"));
            return;
        }
    }

    _pcAddress = fallback.isEmpty() ? QStringLiteral("192.168.2.3") : fallback;
    _log(QStringLiteral("No interface on the Pi subnet — using PC address %1 "
                        "(set \"pc_address\" in ROVCompanion.json if MAVLink does not arrive)")
             .arg(_pcAddress),
         QStringLiteral("warn"));
}

/*===========================================================================
 * State / logging
 *===========================================================================*/

QString ROVCompanionController::stateText() const
{
    switch (_state) {
    case Disconnected:    return QStringLiteral("DISCONNECTED");
    case WaitingForPi:    return QStringLiteral("WAITING FOR PI…");
    case StartingMavlink: return QStringLiteral("STARTING MAVPROXY…");
    case StartingCameras: return QStringLiteral("STARTING CAMERAS…");
    case Connected:       return QStringLiteral("CONNECTED");
    case Failed:          return QStringLiteral("FAILED — TAP TO RETRY");
    }
    return QString();
}

void ROVCompanionController::_setState(LinkState s)
{
    if (_state != s) {
        _state = s;
        emit linkStateChanged();
    }
}

void ROVCompanionController::_log(const QString &msg, const QString &level)
{
    QString glyph = QStringLiteral("•");
    if (level == QLatin1String("ok")) {
        glyph = QStringLiteral("✓");
    } else if (level == QLatin1String("err")) {
        glyph = QStringLiteral("✗");
    } else if (level == QLatin1String("warn")) {
        glyph = QStringLiteral("!");
    } else if (level == QLatin1String("dim")) {
        glyph = QStringLiteral("·");
    }

    _logLines.append(QStringLiteral("%1 %2 %3")
                         .arg(QTime::currentTime().toString(QStringLiteral("hh:mm:ss")), glyph, msg));
    while (_logLines.count() > kMaxLogLines) {
        _logLines.removeFirst();
    }
    emit logChanged();
}

/*===========================================================================
 * Connect-all state machine
 *===========================================================================*/

void ROVCompanionController::toggleConnect()
{
    if (busy() || connected()) {
        disconnectAll();
    } else {
        connectAll();
    }
}

void ROVCompanionController::connectAll()
{
    if (busy()) {
        return;
    }
    if (connected()) {
        _log(QStringLiteral("Already connected"), QStringLiteral("warn"));
        return;
    }

    _cancelRequested = false;
    _stopProcess(_mavlinkProc, QStringLiteral("MAVProxy"));
    _stopProcess(_cameraProc, QStringLiteral("Cameras"));

    _resolvePcAddress();

    _setState(WaitingForPi);
    _bootClock.start();
    _lastWaitLogMs = 0;
    _log(QStringLiteral("Waiting for Pi at %1:22 (up to %2 s)…").arg(_piAddress).arg(_piBootTimeoutS));
    _probeOnce();
}

void ROVCompanionController::disconnectAll()
{
    _cancelRequested = true;
    _abortProbe();
    _stopProcess(_mavlinkProc, QStringLiteral("MAVProxy"));
    _stopProcess(_cameraProc, QStringLiteral("Cameras"));
    _setState(Disconnected);
    _log(QStringLiteral("Disconnected — all SSH channels closed"), QStringLiteral("warn"));
}

void ROVCompanionController::_abortProbe()
{
    _probeAttemptTimer.stop();
    if (_probeSocket) {
        _probeHandled = true;
        _probeSocket->abort();
        _probeSocket->deleteLater();
        _probeSocket = nullptr;
    }
}

void ROVCompanionController::_probeOnce()
{
    if (_cancelRequested) {
        return;
    }

    if (_probeSocket) {
        _probeSocket->abort();
        _probeSocket->deleteLater();
    }
    _probeHandled = false;
    _probeSocket = new QTcpSocket(this);
    connect(_probeSocket, &QTcpSocket::connected, this, &ROVCompanionController::_onProbeConnected);
    connect(_probeSocket, &QTcpSocket::errorOccurred, this, &ROVCompanionController::_onProbeFailed);
    _probeAttemptTimer.start();
    _probeSocket->connectToHost(_piAddress, 22);
}

void ROVCompanionController::_onProbeConnected()
{
    if (_probeHandled) {
        return;
    }
    _probeHandled = true;
    _probeAttemptTimer.stop();

    if (_probeSocket) {
        _probeSocket->abort();
        _probeSocket->deleteLater();
        _probeSocket = nullptr;
    }

    if (_cancelRequested) {
        return;
    }

    _log(QStringLiteral("Pi reachable after %1 s").arg(_bootClock.elapsed() / 1000), QStringLiteral("ok"));
    _startMavlinkChannel();
}

void ROVCompanionController::_onProbeFailed()
{
    if (_probeHandled) {
        return;
    }
    _probeHandled = true;
    _probeAttemptTimer.stop();

    if (_probeSocket) {
        _probeSocket->abort();
        _probeSocket->deleteLater();
        _probeSocket = nullptr;
    }

    _probeRetryOrFail();
}

void ROVCompanionController::_onProbeAttemptTimeout()
{
    if (_probeHandled) {
        return;
    }
    _probeHandled = true;

    if (_probeSocket) {
        _probeSocket->abort();      // abort() does not emit errorOccurred
        _probeSocket->deleteLater();
        _probeSocket = nullptr;
    }

    _probeRetryOrFail();
}

void ROVCompanionController::_probeRetryOrFail()
{
    if (_cancelRequested) {
        return;
    }

    const qint64 elapsedMs = _bootClock.elapsed();
    if (elapsedMs > static_cast<qint64>(_piBootTimeoutS) * 1000) {
        _failConnect(QStringLiteral("Pi did not respond on port 22 within %1 s — check Ethernet/power, then tap retry")
                         .arg(_piBootTimeoutS));
        return;
    }

    if ((elapsedMs - _lastWaitLogMs) > 6000) {
        _lastWaitLogMs = elapsedMs;
        _log(QStringLiteral("…still waiting for Pi (%1 s elapsed)").arg(elapsedMs / 1000), QStringLiteral("dim"));
    }

    QTimer::singleShot(static_cast<int>(_piProbeIntervalS * 1000.0), this, &ROVCompanionController::_probeOnce);
}

void ROVCompanionController::_startMavlinkChannel()
{
    _setState(StartingMavlink);

    const QString remoteCmd = _expandRemoteCommand(_mavproxyCommand);
    _log(QStringLiteral("MAVProxy outputs → QGC udp:%1:%2 + extra udp:%1:%3")
             .arg(_pcAddress)
             .arg(_qgcMavlinkPort)
             .arg(_extraMavlinkPort));

    _mavlinkProc = _spawnSsh(_mavSshUser, _mavSshPassword, remoteCmd, QStringLiteral("MAVProxy"));
    if (!_mavlinkProc) {
        _failConnect(QStringLiteral("Could not start the MAVProxy SSH channel"));
        return;
    }

    QTimer::singleShot(kChannelSettleMs, this, &ROVCompanionController::_checkMavlinkChannelUp);
}

void ROVCompanionController::_checkMavlinkChannelUp()
{
    if (_cancelRequested) {
        return;
    }
    if (!_mavlinkProc || (_mavlinkProc->state() == QProcess::NotRunning)) {
        _failConnect(QStringLiteral("MAVProxy SSH channel exited early — check the SSH password/client and Pi login"));
        return;
    }

    _log(QStringLiteral("MAVLink should now stream to udp:%1 — QGC auto-connects the vehicle from here")
             .arg(_qgcMavlinkPort),
         QStringLiteral("ok"));
    _startCameraChannel();
}

void ROVCompanionController::_startCameraChannel()
{
    _setState(StartingCameras);

    const QString remoteCmd = _expandRemoteCommand(_cameraCommand);
    _cameraProc = _spawnSsh(_camSshUser, _camSshPassword, remoteCmd, QStringLiteral("Cameras"));
    if (!_cameraProc) {
        _log(QStringLiteral("Camera SSH channel failed — MAVLink will still work"), QStringLiteral("warn"));
        _finishConnected();
        return;
    }

    QTimer::singleShot(kChannelSettleMs, this, &ROVCompanionController::_finishConnected);
}

void ROVCompanionController::_finishConnected()
{
    if (_cancelRequested) {
        return;
    }

    if (_cameraProc && (_cameraProc->state() == QProcess::NotRunning)) {
        _log(QStringLiteral("Camera channel exited early — video may be unavailable"), QStringLiteral("warn"));
    }

    _setState(Connected);
    _log(QStringLiteral("All systems connected — Pixhawk via QGC link, cameras via RTSP, arm via UDP"),
         QStringLiteral("ok"));

    emit applyVideoSourceRequested();
}

void ROVCompanionController::_failConnect(const QString &reason)
{
    _abortProbe();
    _stopProcess(_mavlinkProc, QStringLiteral("MAVProxy"));
    _stopProcess(_cameraProc, QStringLiteral("Cameras"));
    _setState(Failed);
    _log(reason, QStringLiteral("err"));
}

/*===========================================================================
 * SSH channels
 *===========================================================================*/

QString ROVCompanionController::_expandRemoteCommand(QString cmd) const
{
    cmd.replace(QLatin1String("{PC_IP}"), _pcAddress);
    cmd.replace(QLatin1String("{PC}"), _pcAddress);
    cmd.replace(QLatin1String("{PI_IP}"), _piAddress);
    cmd.replace(QLatin1String("{PI}"), _piAddress);
    cmd.replace(QLatin1String("{QGC_MAVLINK_PORT}"), QString::number(_qgcMavlinkPort));
    cmd.replace(QLatin1String("{QGC_PORT}"), QString::number(_qgcMavlinkPort));
    cmd.replace(QLatin1String("{MAVLINK_PORT}"), QString::number(_extraMavlinkPort));
    cmd.replace(QLatin1String("{EXTRA_PORT}"), QString::number(_extraMavlinkPort));
    return cmd;
}

QProcess *ROVCompanionController::_spawnSsh(const QString &user, const QString &password,
                                            const QString &remoteCommand, const QString &label)
{
    QString program;
    bool plinkMode = false;
    bool sshpassMode = false;

    const QString cfg = _sshProgramConfig.trimmed();
    if (!cfg.isEmpty() && (cfg.compare(QLatin1String("auto"), Qt::CaseInsensitive) != 0)) {
        program = cfg;
        plinkMode = program.contains(QLatin1String("plink"), Qt::CaseInsensitive);
    } else {
#ifdef Q_OS_WIN
        QString plink = QStandardPaths::findExecutable(QStringLiteral("plink"));
        if (plink.isEmpty()) {
            const QString beside = QCoreApplication::applicationDirPath() + QStringLiteral("/plink.exe");
            if (QFile::exists(beside)) {
                plink = beside;
            }
        }
        if (!plink.isEmpty()) {
            program = plink;
            plinkMode = true;
        } else {
            const QString ssh = QStandardPaths::findExecutable(QStringLiteral("ssh"));
            program = ssh.isEmpty() ? QStringLiteral("ssh") : ssh;
        }
#else
        const QString sshpass = QStandardPaths::findExecutable(QStringLiteral("sshpass"));
        if (!password.isEmpty() && !sshpass.isEmpty()) {
            program = sshpass;
            sshpassMode = true;
        } else {
            const QString ssh = QStandardPaths::findExecutable(QStringLiteral("ssh"));
            program = ssh.isEmpty() ? QStringLiteral("ssh") : ssh;
        }
#endif
    }

    const QString target = user + QLatin1Char('@') + _piAddress;
    QStringList args;

    if (plinkMode) {
        // PuTTY plink: -t allocates a pty so the remote command dies when we
        // close the channel (same behaviour as the Python paramiko GCS).
        args << QStringLiteral("-ssh") << QStringLiteral("-t");
        if (!password.isEmpty()) {
            args << QStringLiteral("-pw") << password;
        }
        args << target << remoteCommand;
        _sshModeHint = QStringLiteral("plink (PuTTY), password login");
    } else if (sshpassMode) {
        args << QStringLiteral("-p") << password
             << QStringLiteral("ssh") << QStringLiteral("-tt")
             << QStringLiteral("-o") << QStringLiteral("StrictHostKeyChecking=no")
             << QStringLiteral("-o") << QStringLiteral("ConnectTimeout=10")
             << QStringLiteral("-o") << QStringLiteral("ServerAliveInterval=15")
             << target << remoteCommand;
        _sshModeHint = QStringLiteral("sshpass + OpenSSH, password login");
    } else {
        args << QStringLiteral("-tt")
             << QStringLiteral("-o") << QStringLiteral("StrictHostKeyChecking=no")
             << QStringLiteral("-o") << QStringLiteral("ConnectTimeout=10")
             << QStringLiteral("-o") << QStringLiteral("ServerAliveInterval=15")
             << target << remoteCommand;
        _sshModeHint = QStringLiteral("OpenSSH (key login)");
        if (!password.isEmpty()) {
            _log(QStringLiteral("%1: password login needs plink (Windows) or sshpass (Linux/macOS) — "
                                "trying key login instead. Put plink.exe next to QGC or install PuTTY.")
                     .arg(label),
                 QStringLiteral("warn"));
        }
    }
    emit linkStateChanged(); // refresh sshModeHint in the UI

    _log(QStringLiteral("SSH → %1  (%2)").arg(target, label));

    QProcess *proc = new QProcess(this);
    proc->setProcessChannelMode(QProcess::MergedChannels);
    _wireProcessLogging(proc, label);
    proc->start(program, args);
    if (!proc->waitForStarted(4000)) {
        _log(QStringLiteral("%1: could not start \"%2\" — is an SSH client installed?").arg(label, program),
             QStringLiteral("err"));
        proc->deleteLater();
        return nullptr;
    }

    _log(QStringLiteral("%1: SSH channel running").arg(label), QStringLiteral("ok"));
    return proc;
}

void ROVCompanionController::_wireProcessLogging(QProcess *proc, const QString &label)
{
    connect(proc, &QProcess::readyReadStandardOutput, this, [this, proc, label]() {
        QString text = QString::fromUtf8(proc->readAllStandardOutput());

        // First-connection host-key prompt (plink): auto-answer "y", same as
        // paramiko.AutoAddPolicy() in the Python GCS.
        if (text.contains(QLatin1String("(y/n"), Qt::CaseInsensitive)
            || text.contains(QLatin1String("Store key in cache"), Qt::CaseInsensitive)) {
            proc->write("y\n");
            _log(QStringLiteral("%1: host key auto-accepted").arg(label), QStringLiteral("warn"));
        }

        text.replace(QLatin1Char('\r'), QLatin1Char('\n'));
        const QStringList lines = text.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
        for (const QString &lineRaw : lines) {
            const QString line = lineRaw.trimmed();
            if (!line.isEmpty()) {
                _log(QStringLiteral("[%1] %2").arg(label, line.left(160)), QStringLiteral("dim"));
            }
        }
    });

    connect(proc, &QProcess::errorOccurred, this, [this, label](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            _log(QStringLiteral("%1: SSH client failed to start (program not found?)").arg(label),
                 QStringLiteral("err"));
        }
    });

    connect(proc, &QProcess::finished, this, [this, label](int exitCode, QProcess::ExitStatus) {
        _log(QStringLiteral("%1: channel closed (exit=%2)").arg(label).arg(exitCode), QStringLiteral("warn"));
    });
}

void ROVCompanionController::_stopProcess(QProcess *&proc, const QString &label)
{
    if (!proc) {
        return;
    }
    disconnect(proc, nullptr, this, nullptr);
    proc->kill(); // pty (-t/-tt) makes the remote command exit with the channel
    proc->waitForFinished(1500);
    proc->deleteLater();
    proc = nullptr;
    _log(QStringLiteral("%1: SSH channel stopped").arg(label), QStringLiteral("dim"));
}

/*===========================================================================
 * Robotic arm servos (Teensy over UDP)
 *===========================================================================*/

void ROVCompanionController::_sendPulse(int channel, int pulseUs)
{
    const QByteArray datagram = QStringLiteral("PULSE%1:%2").arg(channel).arg(pulseUs).toUtf8();
    _servoSocket.writeDatagram(datagram, QHostAddress(_teensyAddress), static_cast<quint16>(_teensyPort));
}

double ROVCompanionController::clawCm() const
{
    const int span = _clawMaxPulse - _clawMinPulse;
    if (span == 0) {
        return 0.0;
    }
    const double t = qBound(0.0, double(_clawPulse - _clawMinPulse) / double(span), 1.0);
    return _clawMaxCm * t;
}

double ROVCompanionController::clawRatio() const
{
    const int span = _clawMaxPulse - _clawMinPulse;
    if (span == 0) {
        return 0.0;
    }
    return qBound(0.0, double(_clawPulse - _clawMinPulse) / double(span), 1.0);
}

double ROVCompanionController::rollDegrees() const
{
    const int pulse = _clampInt(_rollPulse, _rollMinPulse, _rollMaxPulse);
    if (pulse >= _rollCenterPulse) {
        const int span = qMax(1, _rollMaxPulse - _rollCenterPulse);
        return qBound(0.0, double(pulse - _rollCenterPulse) / double(span), 1.0) * 90.0;
    }
    const int span = qMax(1, _rollCenterPulse - _rollMinPulse);
    return -qBound(0.0, double(_rollCenterPulse - pulse) / double(span), 1.0) * 90.0;
}

void ROVCompanionController::setClawPulse(int pulseUs)
{
    const int clamped = _clampInt(pulseUs, _clawMinPulse, _clawMaxPulse);
    _sendPulse(1, clamped); // Servo 1 / PULSE1 = claw
    if (clamped != _clawPulse) {
        _clawPulse = clamped;
        emit servoChanged();
    }
}

void ROVCompanionController::setRollPulse(int pulseUs)
{
    const int clamped = _clampInt(pulseUs, _rollMinPulse, _rollMaxPulse);
    _sendPulse(2, clamped); // Servo 2 / PULSE2 = arm roll
    if (clamped != _rollPulse) {
        _rollPulse = clamped;
        emit servoChanged();
    }
}

void ROVCompanionController::clawStep(int deltaUs)   { setClawPulse(_clawPulse + deltaUs); }
void ROVCompanionController::rollStep(int deltaUs)   { setRollPulse(_rollPulse + deltaUs); }
void ROVCompanionController::clawOpenFull()          { setClawPulse(_clawMaxPulse); }
void ROVCompanionController::clawCloseFull()         { setClawPulse(_clawMinPulse); }
void ROVCompanionController::rollCenter()            { setRollPulse(_rollCenterPulse); }

void ROVCompanionController::resendServoPulses()
{
    _sendPulse(1, _clawPulse);
    _sendPulse(2, _rollPulse);
    _log(QStringLiteral("Servo pulses re-sent (claw %1 µs, roll %2 µs)").arg(_clawPulse).arg(_rollPulse),
         QStringLiteral("dim"));
}

/*===========================================================================
 * Cameras
 *===========================================================================*/

void ROVCompanionController::setActiveCamera(int index)
{
    const int clamped = _clampInt(index, 0, _cameraCount - 1);
    if (clamped != _activeCamera) {
        _activeCamera = clamped;
        _log(QStringLiteral("CAM %1 selected — %2").arg(_activeCamera + 1).arg(currentRtspUrl()));
        emit activeCameraChanged();
    }
}

QString ROVCompanionController::rtspUrlForCamera(int index) const
{
    const int idx = _clampInt(index, 0, _cameraCount - 1);
    return QStringLiteral("rtsp://%1:%2/cam%3").arg(_piAddress).arg(_rtspBasePort + idx).arg(idx);
}
