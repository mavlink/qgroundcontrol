#pragma once

#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtNetwork/QSslSocket>
#include <QtNetwork/QTcpSocket>

#include "RTCMParser.h"

struct NTRIPTransportConfig {
    QString host;
    int port = 2101;
    QString username;
    QString password;
    QString mountpoint;
    QString whitelist;
    bool useTls = false;

    bool operator==(const NTRIPTransportConfig& other) const {
        return host == other.host && port == other.port &&
               username == other.username && password == other.password &&
               mountpoint == other.mountpoint && whitelist == other.whitelist &&
               useTls == other.useTls;
    }
    bool operator!=(const NTRIPTransportConfig& other) const { return !(*this == other); }
};

class NTRIPHttpTransport : public QObject
{
    Q_OBJECT
    friend class NTRIPHttpTransportTest;

public:
    static constexpr int kConnectTimeoutMs   = 10000;
    static constexpr int kDataWatchdogMs     = 30000;
    static constexpr int kMaxHttpHeaderSize  = 32768;

    explicit NTRIPHttpTransport(const NTRIPTransportConfig& config, QObject* parent = nullptr);
    ~NTRIPHttpTransport() override;

    void start();
    void stop();
    void sendNMEA(const QByteArray& nmea);

signals:
    void connected();
    void error(const QString& errorMsg);
    void RTCMDataUpdate(const QByteArray& message);
    void finished();

protected:
    struct HttpStatus {
        int code = 0;
        QString reason;
        bool valid = false;
    };

    static HttpStatus parseHttpStatusLine(const QString& line);
    static QByteArray repairNmeaChecksum(const QByteArray& sentence);
    static bool isHttpSuccess(int code) { return code >= 200 && code < 300; }

private:
    void _connect();
    void _sendHttpRequest();
    void _readBytes();
    void _parseRtcm(const QByteArray& buffer);

    QTcpSocket* _socket = nullptr;
    QTimer* _connectTimeoutTimer = nullptr;
    QTimer* _dataWatchdogTimer = nullptr;

    QString _hostAddress;
    int _port;
    QString _username;
    QString _password;
    QString _mountpoint;
    QVector<int> _whitelist;

    RTCMParser _rtcmParser;
    bool _httpHandshakeDone = false;
    bool _useTls = false;
    bool _stopped = false;

    qint64 _postOkTimestampMs = 0;

    QByteArray _httpResponseBuf;
};
