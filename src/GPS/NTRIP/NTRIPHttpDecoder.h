#pragma once

#include <optional>

#include <QtCore/QByteArray>
#include <QtCore/QByteArrayView>
#include <QtCore/QDateTime>
#include <QtNetwork/QHttpHeaders>

#include "NTRIPError.h"

/// Private transport parser; buffers framing, never correction payloads.
class NTRIPHttpDecoder
{
public:
    static constexpr qsizetype MAX_HEADER_BYTES = 32768;
    static constexpr qsizetype MAX_LINE_BYTES = 8192;
    static constexpr quint64 MAX_CHUNK_BYTES = 16 * 1024 * 1024;

    struct Status
    {
        int code = 0;
        QString reason;
        bool valid = false;
    };

    struct Result
    {
        QByteArray body;  ///< Retains valid prefixes when subsequent framing fails.
        bool connected = false;
        bool complete = false;
        std::optional<NTRIPFailure> failure;
    };

    void reset();
    Result feed(QByteArrayView bytes, const QDateTime& utcNow);
    Result finish();
    static Status parseStatusLine(QByteArrayView line);

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

    void _lineReceived(Result& result, const QDateTime& utcNow);
    void _beginBody(Result& result);
    void _fail(Result& result, const QString& detail, NTRIPError code = NTRIPError::InvalidHttpResponse);

    QHttpHeaders _headers;
    State _state = State::Status;
    QByteArray _line;
    Status _status;
    qsizetype _headerBytes = 0;
    quint64 _remaining = 0;
    int _chunkEndBytes = 0;
    int _informationalResponses = 0;
    bool _http10 = false;
    bool _chunked = false;
    std::optional<quint64> _contentLength;
};
