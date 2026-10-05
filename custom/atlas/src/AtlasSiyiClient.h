#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QElapsedTimer>
#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QUdpSocket>

class AtlasSiyiClient : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(QString host READ host NOTIFY endpointChanged)
    Q_PROPERTY(int port READ port NOTIFY endpointChanged)
    Q_PROPERTY(bool recording READ recording NOTIFY statusChanged)
    Q_PROPERTY(double zoomLevel READ zoomLevel NOTIFY statusChanged)
    Q_PROPERTY(double gimbalYawDeg READ gimbalYawDeg NOTIFY statusChanged)
    Q_PROPERTY(double gimbalPitchDeg READ gimbalPitchDeg NOTIFY statusChanged)
    Q_PROPERTY(double gimbalRollDeg READ gimbalRollDeg NOTIFY statusChanged)

    // Camera codec telemetry/configuration exposed by SIYI External SDK 0x20/0x21.
    Q_PROPERTY(bool mainStreamInfoValid READ mainStreamInfoValid NOTIFY statusChanged)
    Q_PROPERTY(QString mainStreamCodec READ mainStreamCodec NOTIFY statusChanged)
    Q_PROPERTY(int mainStreamWidth READ mainStreamWidth NOTIFY statusChanged)
    Q_PROPERTY(int mainStreamHeight READ mainStreamHeight NOTIFY statusChanged)
    Q_PROPERTY(int mainStreamBitrateKbps READ mainStreamBitrateKbps NOTIFY statusChanged)
    Q_PROPERTY(int mainStreamFps READ mainStreamFps NOTIFY statusChanged)
    Q_PROPERTY(bool recordStreamInfoValid READ recordStreamInfoValid NOTIFY statusChanged)
    Q_PROPERTY(QString recordStreamCodec READ recordStreamCodec NOTIFY statusChanged)
    Q_PROPERTY(int recordStreamWidth READ recordStreamWidth NOTIFY statusChanged)
    Q_PROPERTY(int recordStreamHeight READ recordStreamHeight NOTIFY statusChanged)
    Q_PROPERTY(int recordStreamBitrateKbps READ recordStreamBitrateKbps NOTIFY statusChanged)
    Q_PROPERTY(int recordStreamFps READ recordStreamFps NOTIFY statusChanged)

public:
    explicit AtlasSiyiClient(QObject *parent = nullptr);
    ~AtlasSiyiClient() override;

    bool connected() const { return _connected; }
    QString host() const { return _host; }
    int port() const { return static_cast<int>(_port); }
    bool recording() const { return _recording; }
    double zoomLevel() const { return _zoomLevel; }
    double gimbalYawDeg() const { return _gimbalYawDeg; }
    double gimbalPitchDeg() const { return _gimbalPitchDeg; }
    double gimbalRollDeg() const { return _gimbalRollDeg; }

    bool mainStreamInfoValid() const { return _mainStreamInfoValid; }
    QString mainStreamCodec() const { return _mainStreamCodec; }
    int mainStreamWidth() const { return _mainStreamWidth; }
    int mainStreamHeight() const { return _mainStreamHeight; }
    int mainStreamBitrateKbps() const { return _mainStreamBitrateKbps; }
    int mainStreamFps() const { return _mainStreamFps; }
    bool recordStreamInfoValid() const { return _recordStreamInfoValid; }
    QString recordStreamCodec() const { return _recordStreamCodec; }
    int recordStreamWidth() const { return _recordStreamWidth; }
    int recordStreamHeight() const { return _recordStreamHeight; }
    int recordStreamBitrateKbps() const { return _recordStreamBitrateKbps; }
    int recordStreamFps() const { return _recordStreamFps; }

    Q_INVOKABLE void connectToHost(const QString &host, int port);
    Q_INVOKABLE void disconnectFromHost();

    Q_INVOKABLE void gimbalRate(double yaw, double pitch);
    Q_INVOKABLE void gimbalStop();
    Q_INVOKABLE void gimbalCenter();
    Q_INVOKABLE void gimbalPreset(const QString &preset);
    Q_INVOKABLE void zoom(int value);
    Q_INVOKABLE void takePhoto();
    Q_INVOKABLE void setRecording(bool enabled);

    Q_INVOKABLE void requestCodecSpecs();
    Q_INVOKABLE void configureMainStream(const QString &codec, int width, int height, int bitrateKbps);
    Q_INVOKABLE void configureRecordStream(int width, int height, int bitrateKbps);
    Q_INVOKABLE void setGimbalWorkingMode(const QString &mode);
    Q_INVOKABLE void setVideoOutput(const QString &output);

signals:
    void connectedChanged();
    void statusChanged();
    void endpointChanged();
    void eventReceived(const QString &level, const QString &message, qint64 timestampMs);

private slots:
    void _onReadyRead();
    void _onPoll();

private:
    static constexpr int kPollIntervalMs = 350;
    static constexpr qint64 kWatchdogTimeoutMs = 2200;

    void _setConnected(bool connected);
    void _sendCommand(quint8 commandId, const QByteArray &payload = QByteArray());
    void _handleFrame(quint8 commandId, const QByteArray &payload);
    void _checkWatchdog();

    static quint16 _crc16Xmodem(const QByteArray &data);
    static void _appendInt16LE(QByteArray &data, qint16 value);
    static void _appendUInt16LE(QByteArray &data, quint16 value);
    static qint16 _readInt16LE(const QByteArray &data, int offset);
    static quint16 _readUInt16LE(const QByteArray &data, int offset);

    QUdpSocket _socket;
    QTimer _pollTimer;
    QElapsedTimer _lastRxTimer;

    QString _host = QStringLiteral("192.168.144.25");
    quint16 _port = 37260;
    QHostAddress _address;
    bool _wantConnection = false;
    bool _connected = false;
    bool _recording = false;
    double _zoomLevel = 1.0;
    double _gimbalYawDeg = 0.0;
    double _gimbalPitchDeg = 0.0;
    double _gimbalRollDeg = 0.0;

    bool _mainStreamInfoValid = false;
    QString _mainStreamCodec = QStringLiteral("-");
    int _mainStreamWidth = 0;
    int _mainStreamHeight = 0;
    int _mainStreamBitrateKbps = 0;
    int _mainStreamFps = 0;

    bool _recordStreamInfoValid = false;
    QString _recordStreamCodec = QStringLiteral("-");
    int _recordStreamWidth = 0;
    int _recordStreamHeight = 0;
    int _recordStreamBitrateKbps = 0;
    int _recordStreamFps = 0;
    quint16 _sequence = 0;
    int _pollCounter = 0;
};
