#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QByteArrayView>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QtGlobal>

namespace QGCJpegStreamGuard {

inline constexpr qsizetype kMaximumEncodedBytes = 16 * 1024 * 1024;
inline constexpr quint32 kMaximumDimension = 8192;
inline constexpr quint64 kMaximumDecodedPixels = 7680ULL * 4320ULL;
inline constexpr qsizetype kMaximumMultipartHeaderBytes = 64 * 1024;
inline constexpr qsizetype kMaximumMultipartPartBytes = kMaximumEncodedBytes + kMaximumMultipartHeaderBytes;

/// Validate a complete JPEG and reject dimensions that could cause excessive decoder allocation.
[[nodiscard]] bool validateJpeg(QByteArrayView jpeg, QString* error = nullptr);

/// Bounds bytes accepted by multipartdemux between exact MIME boundary markers.
class MultipartGuard
{
public:
    explicit MultipartGuard(qsizetype maximumPartBytes = kMaximumMultipartPartBytes,
                            qsizetype maximumInitialBoundaryBytes = 4096);

    [[nodiscard]] bool setBoundary(QByteArrayView boundary, QString* error = nullptr);
    [[nodiscard]] bool consume(QByteArrayView data, QString* error = nullptr);

private:
    [[nodiscard]] bool _consumeKnownBoundary(QByteArrayView data, QString* error);
    [[nodiscard]] bool _discoverBoundary(QByteArrayView data, QString* error);
    [[nodiscard]] QList<qsizetype> _findBoundaryEnds(QByteArrayView data, bool allowInitialBoundary) const;
    [[nodiscard]] bool _reject(const QString& reason, QString* error);
    void _updateTail(QByteArrayView data);

    const qsizetype _maximumPartBytes;
    const qsizetype _maximumInitialBoundaryBytes;
    QByteArray _boundaryMarker;
    QByteArray _framedBoundaryMarker;
    QByteArray _tail;
    QByteArray _initialBoundaryLine;
    quint64 _streamBytes = 0;
    quint64 _lastBoundaryEnd = 0;
    quint64 _bytesSinceBoundary = 0;
    bool _seenBoundary = false;
    bool _rejected = false;
};

}  // namespace QGCJpegStreamGuard
