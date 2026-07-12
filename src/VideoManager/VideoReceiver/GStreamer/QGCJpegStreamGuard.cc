#include "QGCJpegStreamGuard.h"

#include <algorithm>
#include <utility>

namespace QGCJpegStreamGuard {

namespace {

constexpr quint8 kMarkerPrefix = 0xFF;
constexpr quint8 kStartOfImage = 0xD8;
constexpr quint8 kEndOfImage = 0xD9;
constexpr quint8 kStartOfScan = 0xDA;
constexpr qsizetype kMaximumBoundaryTokenBytes = 200;

void setError(QString* error, const QString& reason)
{
    if (error) {
        *error = reason;
    }
}

quint8 byteAt(QByteArrayView data, qsizetype offset)
{
    return static_cast<quint8>(data.at(offset));
}

bool isStartOfFrame(quint8 marker)
{
    return ((marker >= 0xC0) && (marker <= 0xC3)) || ((marker >= 0xC5) && (marker <= 0xC7)) ||
           ((marker >= 0xC9) && (marker <= 0xCB)) || ((marker >= 0xCD) && (marker <= 0xCF));
}

bool isStandaloneMarker(quint8 marker)
{
    return marker == 0x01 || ((marker >= 0xD0) && (marker <= 0xD7));
}

bool isValidBoundaryToken(QByteArrayView token)
{
    if (token.isEmpty() || token.size() > kMaximumBoundaryTokenBytes) {
        return false;
    }

    return std::all_of(token.begin(), token.end(), [](char value) {
        const uchar character = static_cast<uchar>(value);
        return (character >= 0x21) && (character <= 0x7E) && character != '"';
    });
}

}  // namespace

bool validateJpeg(QByteArrayView jpeg, QString* error)
{
    if (jpeg.size() > kMaximumEncodedBytes) {
        setError(error, QStringLiteral("JPEG exceeds the 16 MiB encoded-size limit."));
        return false;
    }
    if (jpeg.size() < 4 || byteAt(jpeg, 0) != kMarkerPrefix || byteAt(jpeg, 1) != kStartOfImage ||
        byteAt(jpeg, jpeg.size() - 2) != kMarkerPrefix || byteAt(jpeg, jpeg.size() - 1) != kEndOfImage) {
        setError(error, QStringLiteral("JPEG is missing a complete SOI/EOI envelope."));
        return false;
    }

    bool foundDimensions = false;
    bool foundScan = false;
    bool inEntropyData = false;
    qsizetype offset = 2;
    while (offset < jpeg.size()) {
        if (byteAt(jpeg, offset) != kMarkerPrefix) {
            if (inEntropyData) {
                ++offset;
                continue;
            }
            setError(error, QStringLiteral("JPEG contains data outside a marker segment."));
            return false;
        }

        while (offset < jpeg.size() && byteAt(jpeg, offset) == kMarkerPrefix) {
            ++offset;
        }
        if (offset >= jpeg.size()) {
            setError(error, QStringLiteral("JPEG ends inside a marker."));
            return false;
        }

        const quint8 marker = byteAt(jpeg, offset++);
        if (marker == 0x00) {
            if (!inEntropyData) {
                setError(error, QStringLiteral("JPEG contains an unexpected stuffed byte."));
                return false;
            }
            continue;
        }
        if (marker == kEndOfImage) {
            if (offset != jpeg.size() || !foundDimensions || !foundScan) {
                setError(error, QStringLiteral("JPEG has trailing data or is missing frame/scan metadata."));
                return false;
            }
            return true;
        }
        if (marker == kStartOfImage) {
            setError(error, QStringLiteral("JPEG contains a nested SOI marker."));
            return false;
        }
        if (isStandaloneMarker(marker)) {
            if (!inEntropyData && marker != 0x01) {
                setError(error, QStringLiteral("JPEG restart marker appears outside scan data."));
                return false;
            }
            continue;
        }

        inEntropyData = false;
        if ((offset + 2) > jpeg.size()) {
            setError(error, QStringLiteral("JPEG segment length is truncated."));
            return false;
        }
        const quint16 segmentLength = static_cast<quint16>((byteAt(jpeg, offset) << 8) | byteAt(jpeg, offset + 1));
        if (segmentLength < 2 || segmentLength > static_cast<quint64>(jpeg.size() - offset)) {
            setError(error, QStringLiteral("JPEG segment length is invalid."));
            return false;
        }

        if (isStartOfFrame(marker)) {
            if (segmentLength < 8) {
                setError(error, QStringLiteral("JPEG frame header is truncated."));
                return false;
            }
            const quint32 height = static_cast<quint32>((byteAt(jpeg, offset + 3) << 8) | byteAt(jpeg, offset + 4));
            const quint32 width = static_cast<quint32>((byteAt(jpeg, offset + 5) << 8) | byteAt(jpeg, offset + 6));
            const quint64 pixels = static_cast<quint64>(width) * height;
            if (width == 0 || height == 0) {
                setError(error, QStringLiteral("JPEG frame dimensions must be non-zero."));
                return false;
            }
            if (width > kMaximumDimension || height > kMaximumDimension || pixels > kMaximumDecodedPixels) {
                setError(error, QStringLiteral("JPEG frame dimensions exceed the 8K decoder-allocation limit."));
                return false;
            }
            foundDimensions = true;
        }

        if (marker == kStartOfScan) {
            foundScan = true;
            inEntropyData = true;
        }
        offset += segmentLength;
    }

    setError(error, QStringLiteral("JPEG does not terminate with EOI."));
    return false;
}

MultipartGuard::MultipartGuard(qsizetype maximumPartBytes, qsizetype maximumInitialBoundaryBytes)
    : _maximumPartBytes(std::max<qsizetype>(1, maximumPartBytes)),
      _maximumInitialBoundaryBytes(std::max<qsizetype>(1, maximumInitialBoundaryBytes))
{}

bool MultipartGuard::setBoundary(QByteArrayView boundary, QString* error)
{
    QByteArray normalized(boundary.data(), boundary.size());
    if (normalized.size() >= 2 && normalized.front() == '"' && normalized.back() == '"') {
        normalized = normalized.mid(1, normalized.size() - 2);
    }
    if (normalized.startsWith("--")) {
        normalized.remove(0, 2);
    }
    if (!isValidBoundaryToken(normalized)) {
        return _reject(QStringLiteral("Multipart boundary token is invalid."), error);
    }

    _boundaryMarker = QByteArrayLiteral("--") + normalized;
    _framedBoundaryMarker = QByteArrayLiteral("\r\n") + _boundaryMarker;
    return true;
}

bool MultipartGuard::consume(QByteArrayView data, QString* error)
{
    if (_rejected) {
        setError(error, QStringLiteral("Multipart stream was already rejected."));
        return false;
    }
    if (data.isEmpty()) {
        return true;
    }
    if (_boundaryMarker.isEmpty()) {
        return _discoverBoundary(data, error);
    }
    return _consumeKnownBoundary(data, error);
}

bool MultipartGuard::_discoverBoundary(QByteArrayView data, QString* error)
{
    const QByteArray rawData = QByteArray::fromRawData(data.data(), data.size());
    const qsizetype newline = rawData.indexOf('\n');
    const qsizetype bytesToAppend = (newline >= 0) ? newline + 1 : data.size();
    if ((_initialBoundaryLine.size() + bytesToAppend) > _maximumInitialBoundaryBytes) {
        return _reject(QStringLiteral("Multipart stream did not provide a bounded initial boundary."), error);
    }

    _initialBoundaryLine.append(data.data(), bytesToAppend);
    if (newline < 0) {
        return true;
    }

    QByteArray line = _initialBoundaryLine;
    _initialBoundaryLine.clear();
    if (line.endsWith('\n')) {
        line.chop(1);
    }
    if (line.endsWith('\r')) {
        line.chop(1);
    }
    if (!line.startsWith("--") || !setBoundary(QByteArrayView(line.constData() + 2, line.size() - 2), error)) {
        return _reject(QStringLiteral("Multipart stream initial boundary is invalid."), error);
    }

    _seenBoundary = true;
    _lastBoundaryEnd = 0;
    const qsizetype remainingOffset = bytesToAppend;
    return _consumeKnownBoundary(QByteArrayView(data.data() + remainingOffset, data.size() - remainingOffset), error);
}

bool MultipartGuard::_consumeKnownBoundary(QByteArrayView data, QString* error)
{
    const quint64 previousStreamBytes = _streamBytes;
    const quint64 newStreamBytes = previousStreamBytes + static_cast<quint64>(data.size());
    QList<quint64> boundaryEnds;

    const QList<qsizetype> chunkBoundaryEnds = _findBoundaryEnds(data, !_seenBoundary && previousStreamBytes == 0);
    for (const qsizetype boundaryEnd : chunkBoundaryEnds) {
        boundaryEnds.append(previousStreamBytes + static_cast<quint64>(boundaryEnd));
    }

    if (!_tail.isEmpty()) {
        const qsizetype prefixSize = std::min<qsizetype>(data.size(), _framedBoundaryMarker.size() + 2);
        QByteArray overlap = _tail;
        overlap.append(data.data(), prefixSize);
        const quint64 overlapStart = previousStreamBytes - static_cast<quint64>(_tail.size());
        const QList<qsizetype> overlapBoundaryEnds = _findBoundaryEnds(overlap, !_seenBoundary && overlapStart == 0);
        for (const qsizetype boundaryEnd : overlapBoundaryEnds) {
            boundaryEnds.append(overlapStart + static_cast<quint64>(boundaryEnd));
        }
    }

    _streamBytes = newStreamBytes;
    std::sort(boundaryEnds.begin(), boundaryEnds.end());
    boundaryEnds.erase(std::unique(boundaryEnds.begin(), boundaryEnds.end()), boundaryEnds.end());
    for (const quint64 boundaryEnd : std::as_const(boundaryEnds)) {
        if (_seenBoundary && boundaryEnd <= _lastBoundaryEnd) {
            continue;
        }
        if (_seenBoundary &&
            (boundaryEnd - _lastBoundaryEnd) > static_cast<quint64>(_maximumPartBytes + _framedBoundaryMarker.size())) {
            return _reject(QStringLiteral("Multipart part exceeds the configured encoded-frame bound."), error);
        }
        _seenBoundary = true;
        _lastBoundaryEnd = boundaryEnd;
    }
    _bytesSinceBoundary = _seenBoundary ? newStreamBytes - _lastBoundaryEnd : newStreamBytes;
    _updateTail(data);

    if (!_seenBoundary && _streamBytes > static_cast<quint64>(_maximumInitialBoundaryBytes)) {
        return _reject(QStringLiteral("Multipart stream did not match its declared boundary."), error);
    }
    if (_seenBoundary && _bytesSinceBoundary > static_cast<quint64>(_maximumPartBytes)) {
        return _reject(QStringLiteral("Multipart part exceeds the configured encoded-frame bound."), error);
    }
    return true;
}

QList<qsizetype> MultipartGuard::_findBoundaryEnds(QByteArrayView data, bool allowInitialBoundary) const
{
    QList<qsizetype> boundaryEnds;
    if (_boundaryMarker.isEmpty()) {
        return boundaryEnds;
    }

    const QByteArray rawData = QByteArray::fromRawData(data.data(), data.size());
    const auto suffixIsValid = [&rawData](qsizetype markerEnd) {
        return (markerEnd + 2) <= rawData.size() &&
               ((rawData.at(markerEnd) == '\r' && rawData.at(markerEnd + 1) == '\n') ||
                (rawData.at(markerEnd) == '-' && rawData.at(markerEnd + 1) == '-'));
    };

    if (allowInitialBoundary && rawData.startsWith(_boundaryMarker) && suffixIsValid(_boundaryMarker.size())) {
        boundaryEnds.append(_boundaryMarker.size());
    }

    qsizetype offset = 0;
    while ((offset = rawData.indexOf(_framedBoundaryMarker, offset)) >= 0) {
        const qsizetype markerEnd = offset + _framedBoundaryMarker.size();
        if (suffixIsValid(markerEnd)) {
            boundaryEnds.append(markerEnd);
        }
        ++offset;
    }
    return boundaryEnds;
}

bool MultipartGuard::_reject(const QString& reason, QString* error)
{
    _rejected = true;
    setError(error, reason);
    return false;
}

void MultipartGuard::_updateTail(QByteArrayView data)
{
    const qsizetype tailBytes = _framedBoundaryMarker.size() + 2;
    if (data.size() >= tailBytes) {
        _tail = QByteArray(data.data() + data.size() - tailBytes, tailBytes);
        return;
    }

    _tail.append(data.data(), data.size());
    if (_tail.size() > tailBytes) {
        _tail.remove(0, _tail.size() - tailBytes);
    }
}

}  // namespace QGCJpegStreamGuard
