#include "NTRIPHttpDecoder.h"

#include <QtCore/QRegularExpression>

#include <algorithm>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(NTRIPHttpDecoderLog, "GPS.NTRIP.NTRIPHttpDecoder")

NTRIPHttpDecoder::NTRIPHttpDecoder()
{
    qCDebug(NTRIPHttpDecoderLog) << this;
}

NTRIPHttpDecoder::~NTRIPHttpDecoder()
{
    qCDebug(NTRIPHttpDecoderLog) << this;
}

void NTRIPHttpDecoder::reset(Mode mode)
{
    _mode = mode;
    _state = State::Status;
    _line.clear();
    _headerBytes = 0;
    _remaining = 0;
    _status = 0;
    _chunkEndBytes = 0;
    _informationalResponses = 0;
    _chunked = false;
    _contentLengthSet = false;
    _contentLength = 0;
    _retryAfter = std::chrono::milliseconds{0};
}

void NTRIPHttpDecoder::_fail(Result& result, const QString& detail, NTRIPError code)
{
    _state = State::Failed;
    _line.clear();
    result.failure = NTRIPFailure::fromError(code, detail);
}

NTRIPHttpDecoder::Result NTRIPHttpDecoder::feed(QByteArrayView bytes)
{
    Result result;
    while (!bytes.isEmpty() && _state != State::Failed && _state != State::Complete) {
        if (_state == State::IcyHeaders && _line.isEmpty()) {
            const auto first = static_cast<unsigned char>(bytes.front());
            // Bare ICY is followed immediately by binary RTCM. Some v1 casters
            // instead include ordinary ASCII headers or an empty separator.
            if (first != '\r' && !(first >= 'A' && first <= 'Z') && !(first >= 'a' && first <= 'z')) {
                _beginBody(result);
                if (result.complete) {
                    break;
                }
            }
        }
        if (_state == State::Identity || _state == State::ChunkData) {
            const bool bounded = _state == State::ChunkData || _contentLengthSet;
            const qsizetype count =
                bounded ? static_cast<qsizetype>(std::min<quint64>(bytes.size(), _remaining)) : bytes.size();
            result.body.append(bytes.data(), count);
            bytes = bytes.sliced(count);
            if (bounded) {
                _remaining -= count;
                if (_remaining == 0) {
                    _state = _state == State::ChunkData ? State::ChunkEnd : State::Complete;
                    result.complete = _state == State::Complete;
                }
            }
            continue;
        }
        if (_state == State::ChunkEnd) {
            const char expected = _chunkEndBytes == 0 ? '\r' : '\n';
            if (bytes.front() != expected) {
                _fail(result, QStringLiteral("Invalid HTTP chunk terminator"));
                break;
            }
            bytes = bytes.sliced(1);
            if (++_chunkEndBytes == 2) {
                _chunkEndBytes = 0;
                _state = State::ChunkSize;
            }
            continue;
        }
        _line.append(bytes.front());
        bytes = bytes.sliced(1);
        const bool header = _state == State::Status || _state == State::Headers || _state == State::IcyHeaders ||
                            _state == State::Trailers;
        if ((header && ++_headerBytes > MAX_HEADER_BYTES) || _line.size() > MAX_LINE_BYTES) {
            _fail(result, QStringLiteral("HTTP response header too large"), NTRIPError::HeaderTooLarge);
            break;
        }
        if (_line.endsWith("\r\n")) {
            _line.chop(2);
            _lineReceived(result);
            _line.clear();
        }
    }
    return result;
}

void NTRIPHttpDecoder::_lineReceived(Result& result)
{
    if (_state == State::Status) {
        static const QRegularExpression statusPattern(
            QStringLiteral("^(HTTP/1\\.[01]|ICY|SOURCETABLE) ([0-9]{3})(?: .*)?$"));
        const auto match = statusPattern.match(QString::fromLatin1(_line));
        if (!match.hasMatch()) {
            _fail(result, QStringLiteral("Invalid HTTP status line"));
            return;
        }
        _status = match.captured(2).toInt();
        if (match.captured(1) == QStringLiteral("SOURCETABLE") && _mode == Mode::Corrections) {
            _fail(result, QStringLiteral("Caster returned a source table; select a valid mountpoint"),
                  NTRIPError::InvalidMountpoint);
            return;
        }
        if (match.captured(1) == QStringLiteral("ICY") && _mode == Mode::SourceTable) {
            _fail(result, QStringLiteral("Caster returned a correction stream instead of a source table"));
            return;
        }
        if (match.captured(1) == QStringLiteral("ICY") && _status == 200) {
            _state = State::IcyHeaders;
            result.connected = true;
        } else {
            _state = State::Headers;
        }
        return;
    }
    if (_state == State::ChunkSize) {
        const QByteArray token = _line.split(';').first().trimmed();
        const bool hex = !token.isEmpty() && std::all_of(token.begin(), token.end(), [](char ch) {
            return (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F');
        });
        bool ok = false;
        _remaining = token.toULongLong(&ok, 16);
        if (!hex || !ok || _remaining > MAX_CHUNK_BYTES) {
            _fail(result, QStringLiteral("Invalid or oversized HTTP chunk"));
            return;
        }
        _state = _remaining == 0 ? State::Trailers : State::ChunkData;
        return;
    }
    if (_state == State::Trailers) {
        if (_line.isEmpty()) {
            _state = State::Complete;
            result.complete = true;
        } else if (!_line.contains(':')) {
            _fail(result, QStringLiteral("Invalid HTTP trailer"));
        }
        return;
    }
    if (_line.isEmpty()) {
        if (_state == State::IcyHeaders) {
            _beginBody(result);
            return;
        }
        if (_status >= 100 && _status < 200 && _status != 101 && ++_informationalResponses <= 4) {
            _state = State::Status;
            _chunked = false;
            _contentLengthSet = false;
            return;
        }
        if (_status < 200 || _status >= 300) {
            const auto code = (_status == 401 || _status == 403) ? NTRIPError::AuthFailed
                              : _status == 404                   ? NTRIPError::InvalidMountpoint
                                                                 : NTRIPError::HttpError;
            _fail(result, QStringLiteral("Caster returned HTTP %1").arg(_status), code);
            result.failure->httpStatus = _status;
            result.failure->retryable = _status == 408 || _status == 429 || _status >= 500;
            result.failure->retryAfter = _retryAfter;
            return;
        }
        result.connected = true;
        _beginBody(result);
        return;
    }
    const auto colon = _line.indexOf(':');
    if (colon <= 0) {
        _fail(result, QStringLiteral("Invalid HTTP response header"));
        return;
    }
    const auto name = _line.first(colon).trimmed().toLower();
    const auto value = _line.sliced(colon + 1).trimmed().toLower();
    if (name == "content-type" && value.split(';').first().trimmed() == "gnss/sourcetable" &&
        _mode == Mode::Corrections && _status >= 200 && _status < 300) {
        _fail(result, QStringLiteral("Caster returned a source table; select a valid mountpoint"),
              NTRIPError::InvalidMountpoint);
    } else if (name == "transfer-encoding") {
        if (value != "chunked" || _chunked) {
            _fail(result, QStringLiteral("Unsupported HTTP transfer encoding"));
            return;
        }
        _chunked = true;
    } else if (name == "content-encoding" && value != "identity") {
        _fail(result, QStringLiteral("Unsupported HTTP content encoding"));
    } else if (name == "content-length") {
        bool ok = false;
        const quint64 length = value.toULongLong(&ok);
        if (!ok || (_contentLengthSet && length != _contentLength)) {
            _fail(result, QStringLiteral("Invalid HTTP content length"));
            return;
        }
        _contentLengthSet = true;
        _contentLength = length;
    } else if (name == "retry-after") {
        bool ok = false;
        const auto seconds = value.toULongLong(&ok);
        if (ok) {
            _retryAfter = std::chrono::seconds{std::min<quint64>(seconds, 300)};
        }
    }
}

void NTRIPHttpDecoder::_beginBody(Result& result)
{
    _remaining = _contentLength;
    _state = _chunked ? State::ChunkSize : State::Identity;
    if (!_chunked && _contentLengthSet && _remaining == 0) {
        _state = State::Complete;
        result.complete = true;
    }
}

NTRIPHttpDecoder::Result NTRIPHttpDecoder::finish()
{
    Result result;
    if (_state == State::Identity && !_contentLengthSet) {
        _state = State::Complete;
        result.complete = true;
    } else if (_state != State::Complete && _state != State::Failed) {
        _fail(result, QStringLiteral("Caster disconnected before completing the HTTP response"),
              NTRIPError::InterruptedResponse);
    }
    return result;
}
