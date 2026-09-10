#pragma once

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtNetwork/QTcpSocket>

#include "GPSScheduledTask.h"
#include "NTRIPHttpDecoder.h"
#include "NTRIPTransportConfig.h"

/// One bounded HTTP/NTRIP transaction shared by correction streams and source-table discovery.
class NTRIPHttpResponse : public QObject
{
    Q_OBJECT
    friend class NTRIPHttpTransportTest;

public:
    using Mode = NTRIPHttpDecoder::Mode;
    static constexpr std::chrono::milliseconds kConnectTimeout{10000};
    static constexpr qint64 MAX_SOURCE_TABLE_BYTES = 8 * 1024 * 1024;

    NTRIPHttpResponse(const NTRIPTransportConfig& config, Mode mode, QObject* parent = nullptr,
                      GPSRuntimeScheduler* scheduler = nullptr);
    ~NTRIPHttpResponse() override;
    void start();
    void stop();

    bool active() const { return _socket && !_stopped; }

    bool write(const QByteArray& bytes, bool requireHandshake = true);

signals:
    void connected();
    void bodyReceived(const QByteArray& bytes, qint64 receivedAtMs);
    void completed();
    void failed(const NTRIPFailure& failure);
    void plaintextCredentialsWarning();

private:
    void _connect();
    void _fail(NTRIPError code, const QString& msg);
    void _fail(const NTRIPFailure& failure);
    void _sendHttpRequest();
    void _readBytes();
    void _armDeadline();
    void _scheduleRead();
    void _consumeResult(const NTRIPHttpDecoder::Result& result);

    NTRIPTransportConfig _config;
    Mode _mode;
    QPointer<GPSRuntimeScheduler> _scheduler;
    GPSScheduledTask _deadline;
    QTcpSocket* _socket = nullptr;
    NTRIPHttpDecoder _httpDecoder;
    quint64 _generation = 0;
    qint64 _receivedAtMs = 0;
    qint64 _bodyBytes = 0;
    bool _httpHandshakeDone = false;
    bool _stopped = false;
    bool _readScheduled = false;
    bool _eof = false;
    static constexpr qint64 MAX_READ_PER_TURN = 16384;
    static constexpr qint64 MAX_SOCKET_BUFFER = 65536;
};
