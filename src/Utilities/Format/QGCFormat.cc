#include "QGCFormat.h"

#include <QtCore/QLocale>
#include <QtCore/QString>

namespace QGC {

namespace {
constexpr quint64 kKB = 1024ULL;
constexpr quint64 kMB = kKB * 1024ULL;
constexpr quint64 kGB = kMB * 1024ULL;
constexpr quint64 kTB = kGB * 1024ULL;
} // namespace

QString numberToString(quint64 number)
{
    return QLocale().toString(number);
}

QString bigSizeToString(quint64 size)
{
    const QLocale locale;
    if (size < kKB) {
        return locale.toString(size) + QStringLiteral("B");
    }
    if (size < kMB) {
        return locale.toString(static_cast<double>(size) / kKB, 'f', 1) + QStringLiteral("KB");
    }
    if (size < kGB) {
        return locale.toString(static_cast<double>(size) / kMB, 'f', 1) + QStringLiteral("MB");
    }
    if (size < kTB) {
        return locale.toString(static_cast<double>(size) / kGB, 'f', 1) + QStringLiteral("GB");
    }
    return locale.toString(static_cast<double>(size) / kTB, 'f', 1) + QStringLiteral("TB");
}

QString bigSizeMBToString(quint64 sizeMB)
{
    const QLocale locale;
    if (sizeMB < kKB) {
        return locale.toString(static_cast<double>(sizeMB), 'f', 0) + QStringLiteral(" MB");
    }
    if (sizeMB < kMB) {
        return locale.toString(static_cast<double>(sizeMB) / kKB, 'f', 1) + QStringLiteral(" GB");
    }
    return locale.toString(static_cast<double>(sizeMB) / kMB, 'f', 2) + QStringLiteral(" TB");
}

}
