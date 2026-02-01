#include "LogCompression.h"
#include "QGCCompression.h"
#include <QtCore/QLoggingCategory>

Q_STATIC_LOGGING_CATEGORY(LogCompressionLog, "Utilities.Logging.LogCompression", QtWarningMsg)

namespace LogCompression {

static thread_local int s_lastRatio = 100;

QByteArray compress(const QByteArray& data, Level level, int minSize)
{
    // No compression requested or data too small
    if (level == Level::None || data.size() < minSize) {
        s_lastRatio = 100;
        QByteArray result;
        result.reserve(1 + data.size());
        result.append(static_cast<char>(HeaderUncompressed));
        result.append(data);
        return result;
    }

    const QByteArray compressed = QGCCompression::compressData(data, static_cast<int>(level));

    // Only use if actually smaller
    if (!compressed.isEmpty() && compressed.size() < data.size()) {
        s_lastRatio = (compressed.size() * 100) / data.size();
        qCDebug(LogCompressionLog) << "Compressed" << data.size() << "->"
                                   << compressed.size() << "bytes (" << s_lastRatio << "%)";
        QByteArray result;
        result.reserve(1 + compressed.size());
        result.append(static_cast<char>(HeaderCompressed));
        result.append(compressed);
        return result;
    }

    // Compression didn't help or failed
    s_lastRatio = 100;
    QByteArray result;
    result.reserve(1 + data.size());
    result.append(static_cast<char>(HeaderUncompressed));
    result.append(data);
    return result;
}

QByteArray decompress(const QByteArray& data)
{
    if (data.isEmpty()) {
        return {};
    }

    const quint8 header = static_cast<quint8>(data[0]);
    const QByteArray payload = data.mid(1);

    if (header == HeaderUncompressed) {
        return payload;
    }

    if (header == HeaderCompressed) {
        const QByteArray result = QGCCompression::decompressZlib(payload);
        if (result.isEmpty() && !payload.isEmpty()) {
            qCWarning(LogCompressionLog) << "Decompression failed";
        }
        return result;
    }

    qCWarning(LogCompressionLog) << "Unknown header byte:" << header;
    return {};
}

bool isCompressed(const QByteArray& data)
{
    return !data.isEmpty() && static_cast<quint8>(data[0]) == HeaderCompressed;
}

int lastCompressionRatio()
{
    return s_lastRatio;
}

} // namespace LogCompression
