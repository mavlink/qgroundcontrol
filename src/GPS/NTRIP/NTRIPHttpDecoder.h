#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QByteArrayView>

#include <optional>

#include "NTRIPError.h"

/// Incremental HTTP/ICY response framing. Never interprets RTCM or owns a socket.
class NTRIPHttpDecoder
{
public:
    static constexpr qsizetype MAX_HEADER_BYTES = 32768;
    static constexpr qsizetype MAX_LINE_BYTES = 8192;
    static constexpr quint64 MAX_CHUNK_BYTES = 16 * 1024 * 1024;

    struct Result
    {
        QByteArray body;
        bool connected = false;
        bool complete = false;
        std::optional<NTRIPFailure> failure;
    };

    NTRIPHttpDecoder();
    ~NTRIPHttpDecoder();
    void reset();
    Result feed(QByteArrayView bytes);
    Result finish();

    qsizetype bufferedBytes() const { return _line.size(); }

private:
    enum class State
    {
        Status,
        Headers,
        IcyHeaders,
        Identity,
        ChunkSize,
        ChunkData,
        ChunkEnd,
        Trailers,
        Complete,
        Failed
    };
    void _lineReceived(Result& result);
    void _fail(Result& result, const QString& detail, NTRIPError code = NTRIPError::InvalidHttpResponse);

    State _state = State::Status;
    QByteArray _line;
    qsizetype _headerBytes = 0;
    quint64 _remaining = 0;
    int _status = 0;
    int _chunkEndBytes = 0;
    int _informationalResponses = 0;
    bool _chunked = false;
    bool _contentLengthSet = false;
    quint64 _contentLength = 0;
    std::chrono::milliseconds _retryAfter{0};
};
