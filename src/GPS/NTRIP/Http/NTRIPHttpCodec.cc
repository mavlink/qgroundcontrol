#include "NTRIPHttpCodec.h"

#include <algorithm>
#include <utility>

#include <QtCore/QCoreApplication>
#include <QtCore/QRegularExpression>

#include "NTRIPConfiguration.h"
#include "QGCNetworkClient.h"

namespace {
bool isToken(char ch)
{
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') ||
           QByteArrayView("!#$%&'*+-.^_`|~").contains(ch);
}

bool isFieldValue(char ch)
{
    const auto byte = static_cast<unsigned char>(ch);
    return ch == '\t' || (byte >= 32 && byte != 127);
}

/// Unsigned integer in the given base, without the sign, whitespace or "0x" prefix toULongLong() would accept.
std::optional<quint64> unsignedNumber(QByteArrayView text, int base = 10)
{
    const auto digit = [base](char ch) {
        return (ch >= '0' && ch <= '9') || (base == 16 && ((ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F')));
    };
    bool ok = false;
    const quint64 value = text.toULongLong(&ok, base);
    return ok && std::all_of(text.begin(), text.end(), digit) ? std::optional(value) : std::nullopt;
}

std::chrono::milliseconds retryAfter(const QHttpHeaders& headers, const QDateTime& utcNow)
{
    const QByteArrayView value = headers.value(QHttpHeaders::WellKnownHeader::RetryAfter);
    if (const auto seconds = unsignedNumber(value)) {
        return std::chrono::seconds{std::min<quint64>(*seconds, 300)};
    }
    auto deadline = headers.dateTimeValue(QHttpHeaders::WellKnownHeader::RetryAfter);
    if (!deadline || !utcNow.isValid()) {
        return {};
    }
    // Qt reads the two-digit RFC 850 year as 19yy; RFC 9110 wants the latest century not over 50 years ahead.
    if (value.indexOf(',') > 3) {
        *deadline = deadline->addYears(utcNow.date().year() / 100 * 100 - 1900);
        if (*deadline > utcNow.addYears(50)) {
            *deadline = deadline->addYears(-100);
        }
    }
    return std::chrono::milliseconds{std::clamp(utcNow.msecsTo(*deadline), qint64(0), qint64(300000))};
}

QString tr(const char* text)
{
    return QCoreApplication::translate("NTRIPHttpTransport", text);
}
}  // namespace

NTRIPHttpRequest NTRIPHttpRequest::build(const NTRIPConnectionConfig& config, Purpose purpose)
{
    NTRIPHttpRequest result;
    result.error = purpose == Purpose::Corrections ? config.streamValidationError() : config.validationError();
    if (!result.error.isEmpty()) {
        return result;
    }

    result.url.setScheme(config.useTls ? QStringLiteral("https") : QStringLiteral("http"));
    result.url.setHost(config.host);
    result.url.setPort(config.port);
    result.url.setPath(purpose == Purpose::SourceTable ? QStringLiteral("/") : QLatin1Char('/') + config.mountpoint);
    if (!result.url.isValid() || result.url.host().isEmpty()) {
        result.error = QCoreApplication::translate("NTRIPHttpTransport", "Invalid NTRIP endpoint");
        return result;
    }

    // Some legacy casters match these header spellings case-sensitively. The fully encoded URL parts and the
    // Base64 credentials are always valid field values.
    result.bytes = "GET " + result.url.path(QUrl::FullyEncoded).toLatin1() + " HTTP/1.1\r\n" +
                   "Host: " + result.url.authority(QUrl::FullyEncoded).toLatin1() + "\r\n" +
                   "Ntrip-Version: Ntrip/2.0\r\nUser-Agent: NTRIP QGroundControl/1.0\r\n";
    const bool hasCredentials = !config.username.isEmpty() || !config.password.isEmpty();
    if (hasCredentials) {
        result.bytes += "Authorization: Basic " +
                        QGCNetworkHelper::createBasicAuthCredentials(config.username, config.password).toLatin1() +
                        "\r\n";
    }
    result.bytes += "\r\n";
    result.credentialsInClear = hasCredentials && !config.useTls;
    return result;
}

void NTRIPHttpDecoder::reset()
{
    *this = NTRIPHttpDecoder{_purpose};
}

NTRIPHttpDecoder::Status NTRIPHttpDecoder::parseStatusLine(QByteArrayView line)
{
    static const QRegularExpression pattern(
        QStringLiteral("^(HTTP/1\\.[01]|(?i:ICY|SOURCETABLE)) ([1-5][0-9]{2})(?: ([\\t\\x20-\\x7e\\x80-\\xff]*))?\\z"));
    const auto match = pattern.match(QString::fromLatin1(line));
    return match.hasMatch() ? Status{match.captured(2).toInt(), match.captured(3), true} : Status{};
}

void NTRIPHttpDecoder::_fail(Result& result, const QString& detail, NTRIPError code)
{
    if (_pendingFailure) {
        _finishError(result);
        return;
    }
    _state = State::Failed;
    _line.clear();
    result.complete = false;
    result.failure = NTRIPFailure{code, detail};
}

void NTRIPHttpDecoder::_finishError(Result& result)
{
    if (!_pendingFailure) {
        _fail(result, tr("Invalid HTTP error state"));
        return;
    }
    static const QRegularExpression htmlTags(QStringLiteral("<[^>]*(?:>|$)"));
    static const QRegularExpression controls(QStringLiteral("[\\x00-\\x08\\x0e-\\x1f\\x7f]"));
    QString preview = QString::fromUtf8(_errorBody);
    preview.remove(htmlTags);
    preview.remove(controls);
    preview = preview.simplified().left(MAX_ERROR_PREVIEW_CHARS);
    result.failure = std::exchange(_pendingFailure, std::nullopt);
    if (!preview.isEmpty()) {
        result.failure->detail += QStringLiteral(" \u2014 ") + preview;
    }
    _state = State::Failed;
    _line.clear();
    _errorBody.clear();
    result.body.clear();
    result.connected = false;
    result.complete = false;
    result.awaitingErrorBody = false;
}

void NTRIPHttpDecoder::_beginIcyBody(Result& result)
{
    _beginBody(result);
    result.body += std::exchange(_line, {});
}

NTRIPHttpDecoder::Result NTRIPHttpDecoder::feed(QByteArrayView bytes, const QDateTime& utcNow)
{
    Result result;
    while (!bytes.isEmpty() && _state != State::Failed) {
        if (_state == State::Complete) {
            _fail(result, tr("Unexpected data after HTTP response"));
            break;
        }
        if (_state == State::IcyHeaders) {
            const auto first = static_cast<unsigned char>(bytes.front());
            const bool binary = first >= 127 || (first < 32 && first != '\t' && first != '\r' && first != '\n');
            // A legacy stream may start mid-frame, not at the RTCM preamble.
            // Probe optional ASCII headers only until binary or non-header input arrives.
            if (_headers.isEmpty() &&
                (binary || (first == '\n' && !_line.endsWith('\r')) || (_line.endsWith('\r') && first != '\n'))) {
                _beginIcyBody(result);
            } else if (!_headers.isEmpty() && _line.isEmpty() && binary) {
                // ICY casters may omit the blank separator even after optional headers.
                _lineReceived(result, utcNow);
                continue;
            }
        }
        if (_state == State::Identity || _state == State::ChunkData) {
            const bool bounded = _state == State::ChunkData || _contentLength.has_value();
            const auto count =
                bounded ? static_cast<qsizetype>(std::min<quint64>(bytes.size(), _remaining)) : bytes.size();
            if (_pendingFailure) {
                _errorBody.append(bytes.data(), std::min(count, MAX_ERROR_BODY_BYTES - _errorBody.size()));
            } else {
                result.body.append(bytes.data(), count);
            }
            bytes = bytes.sliced(count);
            if (bounded && (_remaining -= count) == 0) {
                _state = _state == State::ChunkData ? State::ChunkEnd : State::Complete;
                result.complete = _state == State::Complete;
            }
            if (_pendingFailure && (_state == State::Complete || _errorBody.size() == MAX_ERROR_BODY_BYTES)) {
                _finishError(result);
            }
            continue;
        }
        if (_state == State::ChunkEnd) {
            if (bytes.front() != (_chunkEndBytes == 0 ? '\r' : '\n')) {
                _fail(result, tr("Invalid HTTP chunk terminator"));
                break;
            }
            bytes = bytes.sliced(1);
            if (++_chunkEndBytes == 2) {
                _chunkEndBytes = 0;
                _state = State::ChunkSize;
            }
            continue;
        }
        const char ch = bytes.front();
        bytes = bytes.sliced(1);
        if ((ch == '\n' && !_line.endsWith('\r')) || (_line.endsWith('\r') && ch != '\n')) {
            _fail(result, tr("Invalid HTTP line ending"));
            break;
        }
        _line.append(ch);
        if ((_state != State::ChunkSize && ++_headerBytes > MAX_HEADER_BYTES) || _line.size() > MAX_LINE_BYTES) {
            _fail(result, tr("HTTP response header too large"), NTRIPError::HeaderTooLarge);
            break;
        }
        if (ch == '\n') {
            _line.chop(2);
            _lineReceived(result, utcNow);
            _line.clear();
        }
    }
    result.awaitingErrorBody = awaitingErrorBody();
    return result;
}

void NTRIPHttpDecoder::_lineReceived(Result& result, const QDateTime& utcNow)
{
    if (_state == State::Status) {
        _status = parseStatusLine(_line);
        if (!_status.valid) {
            _fail(result, tr("Invalid HTTP status line"));
        } else if (_line.left(11).compare("SOURCETABLE", Qt::CaseInsensitive) == 0) {
            if (_purpose == Purpose::SourceTable && _status.code == 200) {
                _state = State::IcyHeaders;
                result.connected = true;
            } else {
                _fail(result, tr("Caster returned a source table; select a valid mountpoint"),
                      NTRIPError::InvalidMountpoint);
            }
        } else if (_line.first(3).compare("ICY", Qt::CaseInsensitive) == 0 && _status.code == 200) {
            _state = State::IcyHeaders;
            result.connected = true;
        } else {
            _http10 = _line.startsWith("HTTP/1.0");
            _state = State::Headers;
        }
        return;
    }
    if (_state == State::ChunkSize) {
        // NTRIP uses no chunk extensions, so everything after ';' (and the whitespace before it) is ignored.
        QByteArrayView size(_line);
        if (const qsizetype separator = size.indexOf(';'); separator >= 0) {
            size = size.first(separator);
            while (size.endsWith(' ') || size.endsWith('\t')) {
                size.chop(1);
            }
        }
        const auto length = unsignedNumber(size, 16);
        if (!length || *length > MAX_CHUNK_BYTES) {
            _fail(result, tr("Invalid or oversized HTTP chunk"));
        } else {
            _remaining = *length;
            _state = _remaining == 0 ? State::Trailers : State::ChunkData;
        }
        return;
    }
    if (_line.isEmpty()) {
        if (_state == State::Trailers) {
            if (_pendingFailure) {
                _finishError(result);
            } else {
                _state = State::Complete;
                result.complete = true;
            }
            return;
        }
        if (_status.code >= 300) {
            const auto code = _status.code == 401 ? NTRIPError::AuthFailed : NTRIPError::HttpError;
            const QString detail = _status.code == 401 ? tr("Authentication failed (401): check username and password")
                                                       : tr("HTTP %1: %2").arg(_status.code).arg(_status.reason);
            _pendingFailure = NTRIPFailure{code, detail};
            if (_headers.values(QHttpHeaders::WellKnownHeader::RetryAfter).size() == 1) {
                _pendingFailure->retryAfter = retryAfter(_headers, utcNow);
            }
            if (code == NTRIPError::AuthFailed) {
                _finishError(result);
                return;
            }
        }
        const auto lengths = _headers.values(QHttpHeaders::WellKnownHeader::ContentLength);
        for (const auto& field : lengths) {
            for (const auto& value : field.split(',')) {
                const auto length = unsignedNumber(QByteArrayView(value).trimmed());
                if (!length || (_contentLength && *_contentLength != *length)) {
                    _fail(result, tr("Invalid or conflicting HTTP content length"));
                    return;
                }
                _contentLength = *length;
            }
        }
        const auto transfers = _headers.values(QHttpHeaders::WellKnownHeader::TransferEncoding);
        _chunked = !transfers.isEmpty();
        if (_chunked && (_http10 || transfers.size() != 1 ||
                         transfers.first().compare("chunked", Qt::CaseInsensitive) != 0 || _contentLength)) {
            _fail(result, tr("Unsupported or conflicting HTTP transfer encoding"));
            return;
        }
        if (_status.code >= 100 && _status.code < 200) {
            if (_status.code == 101 || _contentLength || _chunked || ++_informationalResponses > 4) {
                _fail(result, tr("Unsupported HTTP informational response"));
            } else {
                _headers.clear();
                _state = State::Status;
            }
            return;
        }
        const auto encodings = _headers.values(QHttpHeaders::WellKnownHeader::ContentEncoding);
        if (!encodings.isEmpty() &&
            (encodings.size() != 1 || encodings.first().compare("identity", Qt::CaseInsensitive) != 0)) {
            _fail(result, tr("Unsupported HTTP content encoding"));
            return;
        }
        if (_pendingFailure) {
            _beginBody(result);
            return;
        }
        for (const auto& value : _headers.values(QHttpHeaders::WellKnownHeader::ContentType)) {
            if (_purpose == Purpose::Corrections &&
                value.split(';').first().trimmed().compare("gnss/sourcetable", Qt::CaseInsensitive) == 0) {
                _fail(result, tr("Caster returned a source table; select a valid mountpoint"),
                      NTRIPError::InvalidMountpoint);
                return;
            }
        }
        if (_status.code == 204 || _status.code == 205) {
            if (_chunked || (_contentLength && *_contentLength != 0)) {
                _fail(result, tr("Unexpected framing for an empty HTTP response"));
                return;
            }
            _contentLength = 0;
        }
        result.connected = result.connected || _state != State::IcyHeaders;
        _beginBody(result);
        return;
    }

    const qsizetype colon = _line.indexOf(':');
    const QByteArrayView line(_line);
    if (colon <= 0 || !std::all_of(line.begin(), line.begin() + colon, isToken) ||
        !std::all_of(line.begin() + colon + 1, line.end(), isFieldValue)) {
        if (_state == State::IcyHeaders && _headers.isEmpty()) {
            _line += "\r\n";
            _beginIcyBody(result);
            return;
        }
        _fail(result, tr("Invalid HTTP response header"));
        return;
    }
    if (_headers.size() >= 128) {
        _fail(result, tr("Too many HTTP response headers"), NTRIPError::HeaderTooLarge);
        return;
    }
    const auto name = line.first(colon);
    const auto value = line.sliced(colon + 1);
    if (_state == State::Trailers &&
        (name.compare("content-length", Qt::CaseInsensitive) == 0 ||
         name.compare("transfer-encoding", Qt::CaseInsensitive) == 0 ||
         name.compare("content-encoding", Qt::CaseInsensitive) == 0 ||
         name.compare("content-type", Qt::CaseInsensitive) == 0 ||
         name.compare("retry-after", Qt::CaseInsensitive) == 0 || name.compare("host", Qt::CaseInsensitive) == 0)) {
        _fail(result, tr("Forbidden HTTP trailer"));
        return;
    }
    if (!_headers.append(QLatin1StringView(name.data(), name.size()), QLatin1StringView(value.data(), value.size()))) {
        _fail(result, tr("Invalid HTTP response header"));
    }
}

void NTRIPHttpDecoder::_beginBody(Result& result)
{
    _remaining = _contentLength.value_or(0);
    _state = _chunked ? State::ChunkSize : State::Identity;
    if (!_chunked && _contentLength && _remaining == 0) {
        if (_pendingFailure) {
            _finishError(result);
        } else {
            _state = State::Complete;
            result.complete = true;
        }
    }
}

NTRIPHttpDecoder::Result NTRIPHttpDecoder::finish()
{
    Result result;
    if (_pendingFailure) {
        _finishError(result);
    } else if ((_state == State::Identity && !_contentLength) ||
               (_state == State::IcyHeaders && _line.isEmpty() && _headers.isEmpty()) || _state == State::Complete) {
        _state = State::Complete;
        result.complete = true;
    } else if (_state == State::Identity || _state == State::ChunkSize || _state == State::ChunkData ||
               _state == State::ChunkEnd || _state == State::Trailers) {
        _fail(result, tr("Caster closed the connection mid-transfer"));
    } else if (_state != State::Failed) {
        _fail(result, tr("Caster disconnected before completing the HTTP response"));
    }
    return result;
}
