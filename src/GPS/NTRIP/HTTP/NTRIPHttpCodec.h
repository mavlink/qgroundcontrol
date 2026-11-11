#pragma once

#include <optional>

#include <QtCore/QByteArray>
#include <QtCore/QByteArrayView>
#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QString>
#include <QtNetwork/QHttpHeaders>

#include "NTRIPError.h"

struct NTRIPConnectionConfig;

enum class NTRIPHttpPurpose
{
    Corrections,
    SourceTable,
};

/// The NTRIP version whose form a request takes. The decoder reads the answers of both versions to either form.
enum class NTRIPVersion
{
    /// An HTTP/1.1 request with the Ntrip-Version header.
    Version2,
    /// The HTTP/1.0 request, for casters that close a version 2 request without answering it.
    Version1,
};

struct NTRIPHttpRequest
{
    QByteArray bytes;
    QString error;

    [[nodiscard]] static NTRIPHttpRequest build(const NTRIPConnectionConfig& config,
                                                NTRIPHttpPurpose purpose = NTRIPHttpPurpose::Corrections,
                                                NTRIPVersion version = NTRIPVersion::Version2);
};

/// Private transport parser; buffers framing, never correction payloads.
class NTRIPHttpDecoder
{
    // The messages are translated in NTRIPHttpTransport's context, where the translation files hold them.
    Q_DECLARE_TR_FUNCTIONS(NTRIPHttpTransport)

public:
    explicit NTRIPHttpDecoder(NTRIPHttpPurpose purpose = NTRIPHttpPurpose::Corrections)
        : _purpose(purpose)
    {}

    static constexpr qsizetype MAX_HEADER_BYTES = 32768;
    static constexpr qsizetype MAX_LINE_BYTES = 8192;
    static constexpr qsizetype MAX_HEADER_COUNT = 128;
    static constexpr int MAX_INFORMATIONAL_RESPONSES = 4;
    static constexpr quint64 MAX_CHUNK_BYTES = 16 * 1024 * 1024;
    static constexpr qsizetype MAX_ERROR_BODY_BYTES = 500;
    static constexpr qsizetype MAX_ERROR_PREVIEW_CHARS = 200;

    struct [[nodiscard]] Status
    {
        int code = 0;
        QString reason;
        bool valid = false;
    };

    struct [[nodiscard]] Result
    {
        QByteArray body;  ///< Retains valid prefixes when subsequent framing fails.
        bool connected = false;
        bool complete = false;
        std::optional<NTRIPFailure> failure;
    };

    void reset();
    Result feed(QByteArrayView bytes, const QDateTime& utcNow);
    Result finish();

    bool awaitingErrorBody() const { return _pendingFailure.has_value(); }

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
        Failed,
    };

    /// Takes the body bytes at the front of @a bytes, up to the end of a delimited body or chunk; returns their count.
    qsizetype _bodyReceived(QByteArrayView bytes, Result& result);
    /// A byte of the CRLF that ends a chunk.
    void _chunkEndReceived(char ch, Result& result);
    /// A byte of a line, which ends with CRLF.
    void _lineByteReceived(char ch, Result& result, const QDateTime& utcNow);
    /// A complete line: the status line, a header or trailer field, or a chunk size, as the state has it.
    void _lineReceived(Result& result, const QDateTime& utcNow);
    void _statusLineReceived(Result& result);
    void _chunkSizeReceived(Result& result);
    /// A header or trailer field, or the start of a legacy stream where its optional headers could be.
    void _fieldReceived(Result& result);
    void _trailersEnded(Result& result);
    void _headersEnded(Result& result, const QDateTime& utcNow);
    /// What the error status of the response means, with the caster's Retry-After hint.
    NTRIPFailure _statusFailure(const QDateTime& utcNow) const;
    /// Reads how the body is delimited; false once conflicting or unsupported framing has failed the response.
    bool _readFraming(Result& result);
    /// False once the headers of a successful response have failed it: a source table sent in place of corrections,
    /// or a body where the status allows none.
    bool _acceptContent(Result& result);
    void _beginBody(Result& result);
    void _beginIcyBody(Result& result);
    void _finishError(Result& result);
    /// What a 3xx answer means for the user, naming the Location target when the caster gave one.
    static QString _redirectDetail(int status, const QHttpHeaders& headers);
    void _fail(Result& result, const QString& detail, NTRIPError code = NTRIPError::InvalidHttpResponse);

    QHttpHeaders _headers;
    NTRIPHttpPurpose _purpose;
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
    std::optional<NTRIPFailure> _pendingFailure;
    QByteArray _errorBody;
};
