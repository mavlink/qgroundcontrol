/****************************************************************************
 *
 * MIST MAVIROV — ROV Companion for QGroundControl
 *
 * One-click bring-up of the whole ROV stack from inside QGC:
 *   - Waits for the Raspberry Pi to finish booting (SSH port 22 probe)
 *   - SSH channel 1: starts MAVProxy on the Pi, which streams MAVLink to
 *     this PC on UDP 14550 — QGC's normal UDP autoconnect then picks the
 *     Pixhawk up automatically (so QGC behaves exactly as stock QGC).
 *   - SSH channel 2: starts the multi-camera RTSP server on the Pi; the
 *     panel then points QGC's built-in video widget at the selected stream.
 *   - Drives the Teensy robotic-arm servos (claw + roll) over UDP using
 *     the same "PULSE1:<us>" / "PULSE2:<us>" protocol as the Python GCS.
 *
 * Configuration is read from ROVCompanion.json (see _writeDefaultConfig).
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QElapsedTimer>
#include <QtCore/QObject>
#include <QtCore/QProcess>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QTimer>
#include <QtNetwork/QUdpSocket>
#include <QtQmlIntegration/QtQmlIntegration>

class QTcpSocket;

class ROVCompanionController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(int          linkState       READ linkState      NOTIFY linkStateChanged)
    Q_PROPERTY(QString      stateText       READ stateText      NOTIFY linkStateChanged)
    Q_PROPERTY(bool         connected       READ connected      NOTIFY linkStateChanged)
    Q_PROPERTY(bool         busy            READ busy           NOTIFY linkStateChanged)
    Q_PROPERTY(QStringList  logLines        READ logLines       NOTIFY logChanged)

    Q_PROPERTY(int          clawPulse       READ clawPulse      NOTIFY servoChanged)
    Q_PROPERTY(int          rollPulse       READ rollPulse      NOTIFY servoChanged)
    Q_PROPERTY(double       clawCm          READ clawCm         NOTIFY servoChanged)
    Q_PROPERTY(double       clawRatio       READ clawRatio      NOTIFY servoChanged)
    Q_PROPERTY(double       rollDegrees     READ rollDegrees    NOTIFY servoChanged)
    Q_PROPERTY(int          bigStep         READ bigStep        CONSTANT)
    Q_PROPERTY(int          smallStep       READ smallStep      CONSTANT)

    Q_PROPERTY(int          activeCamera    READ activeCamera   WRITE setActiveCamera NOTIFY activeCameraChanged)
    Q_PROPERTY(int          cameraCount     READ cameraCount    CONSTANT)
    Q_PROPERTY(QString      currentRtspUrl  READ currentRtspUrl NOTIFY activeCameraChanged)

    Q_PROPERTY(QString      piAddress       READ piAddress      CONSTANT)
    Q_PROPERTY(QString      pcAddress       READ pcAddress      NOTIFY linkStateChanged)
    Q_PROPERTY(QString      teensyAddress   READ teensyAddress  CONSTANT)
    Q_PROPERTY(QString      sshModeHint     READ sshModeHint    NOTIFY linkStateChanged)
    Q_PROPERTY(QString      configPath      READ configPath     CONSTANT)

public:
    enum LinkState {
        Disconnected = 0,
        WaitingForPi,
        StartingMavlink,
        StartingCameras,
        Connected,
        Failed
    };
    Q_ENUM(LinkState)

    explicit ROVCompanionController(QObject *parent = nullptr);
    ~ROVCompanionController() override;

    int         linkState()      const { return static_cast<int>(_state); }
    QString     stateText()      const;
    bool        connected()      const { return _state == Connected; }
    bool        busy()           const { return (_state == WaitingForPi) || (_state == StartingMavlink) || (_state == StartingCameras); }
    QStringList logLines()       const { return _logLines; }

    int         clawPulse()      const { return _clawPulse; }
    int         rollPulse()      const { return _rollPulse; }
    double      clawCm()         const;
    double      clawRatio()      const;
    double      rollDegrees()    const;
    int         bigStep()        const { return _servoBigStep; }
    int         smallStep()      const { return _servoSmallStep; }

    int         activeCamera()   const { return _activeCamera; }
    void        setActiveCamera(int index);
    int         cameraCount()    const { return _cameraCount; }
    QString     currentRtspUrl() const { return rtspUrlForCamera(_activeCamera); }

    QString     piAddress()      const { return _piAddress; }
    QString     pcAddress()      const { return _pcAddress; }
    QString     teensyAddress()  const { return _teensyAddress; }
    QString     sshModeHint()    const { return _sshModeHint; }
    QString     configPath()     const { return _configPath; }

    /// One click: wait for Pi boot -> SSH MAVProxy -> SSH cameras -> done.
    Q_INVOKABLE void connectAll();
    /// Tears down both SSH channels and stops any pending connect attempt.
    Q_INVOKABLE void disconnectAll();
    /// Convenience for a single toolbar/panel button.
    Q_INVOKABLE void toggleConnect();

    Q_INVOKABLE void clawStep(int deltaUs);
    Q_INVOKABLE void rollStep(int deltaUs);
    Q_INVOKABLE void setClawPulse(int pulseUs);
    Q_INVOKABLE void setRollPulse(int pulseUs);
    Q_INVOKABLE void clawOpenFull();
    Q_INVOKABLE void clawCloseFull();
    Q_INVOKABLE void rollCenter();
    /// Re-transmits the current claw + roll pulses (e.g. after a Teensy reboot).
    Q_INVOKABLE void resendServoPulses();

    Q_INVOKABLE QString rtspUrlForCamera(int index) const;

signals:
    void linkStateChanged();
    void logChanged();
    void servoChanged();
    void activeCameraChanged();
    /// Emitted when QGC's video settings should be switched to currentRtspUrl
    /// (handled in QML so this class stays free of QGC internals).
    void applyVideoSourceRequested();

private slots:
    void _probeOnce();
    void _onProbeConnected();
    void _onProbeFailed();
    void _onProbeAttemptTimeout();

private:
    void     _loadConfig();
    QString  _locateConfigFile() const;
    void     _writeDefaultConfig(const QString &path) const;
    void     _resolvePcAddress();
    void     _setState(LinkState s);
    void     _log(const QString &msg, const QString &level = QStringLiteral("info"));
    void     _startMavlinkChannel();
    void     _checkMavlinkChannelUp();
    void     _startCameraChannel();
    void     _finishConnected();
    void     _failConnect(const QString &reason);
    void     _probeRetryOrFail();
    QString  _expandRemoteCommand(QString cmd) const;
    QProcess *_spawnSsh(const QString &user, const QString &password, const QString &remoteCommand, const QString &label);
    void     _wireProcessLogging(QProcess *proc, const QString &label);
    void     _stopProcess(QProcess *&proc, const QString &label);
    void     _abortProbe();
    void     _sendPulse(int channel, int pulseUs);

    static int _clampInt(int v, int lo, int hi) { return (v < lo) ? lo : ((v > hi) ? hi : v); }

    // ---- connection state ----
    LinkState     _state = Disconnected;
    bool          _cancelRequested = false;
    QElapsedTimer _bootClock;
    qint64        _lastWaitLogMs = 0;
    QTcpSocket   *_probeSocket = nullptr;
    QTimer        _probeAttemptTimer;
    bool          _probeHandled = false;
    QProcess     *_mavlinkProc = nullptr;
    QProcess     *_cameraProc = nullptr;
    QStringList   _logLines;

    // ---- servo state ----
    QUdpSocket    _servoSocket;
    int           _clawPulse = 500;
    int           _rollPulse = 1525;

    // ---- camera state ----
    int           _activeCamera = 0;

    // ---- configuration (defaults mirror the MAVIROV Python GCS) ----
    QString  _configPath;
    QString  _piAddress           = QStringLiteral("192.168.2.2");
    QString  _pcAddressConfig     = QStringLiteral("auto");
    QString  _pcAddress           = QStringLiteral("192.168.2.3");
    int      _qgcMavlinkPort      = 14550;
    int      _extraMavlinkPort    = 14551;
    QString  _mavSshUser          = QStringLiteral("pi");
    QString  _mavSshPassword      = QStringLiteral("pi");
    QString  _camSshUser          = QStringLiteral("farhanpi");
    QString  _camSshPassword      = QStringLiteral("pi");
    QString  _sshProgramConfig    = QStringLiteral("auto");
    QString  _sshModeHint;
    int      _piBootTimeoutS      = 90;
    double   _piProbeIntervalS    = 2.0;
    QString  _mavproxyCommand     = QStringLiteral(
        "source ~/mavenv/bin/activate && "
        "mavproxy.py --master=/dev/ttyACM0 --baudrate 115200 "
        "--out=udp:{PC_IP}:{QGC_MAVLINK_PORT} --out=udp:{PC_IP}:{MAVLINK_PORT}");
    QString  _cameraCommand       = QStringLiteral("python3 ~/multi_camera_rtsp.py");
    int      _rtspBasePort        = 8554;
    int      _cameraCount         = 3;
    QString  _teensyAddress       = QStringLiteral("192.168.2.1");
    int      _teensyPort          = 8888;
    int      _clawMinPulse        = 500;   // fully closed
    int      _clawMaxPulse        = 1860;  // fully open
    int      _rollMinPulse        = 500;
    int      _rollMaxPulse        = 2500;
    int      _rollCenterPulse     = 1525;  // 0 degrees
    double   _clawMaxCm           = 12.0;
    int      _servoBigStep        = 15;
    int      _servoSmallStep      = 3;
};
