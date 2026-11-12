#include "QGCFormat.h"

#include <cmath>

#include <QtCore/QLocale>
#include <QtCore/QString>

namespace QGC {

namespace {
constexpr quint64 kKB = 1024ULL;
constexpr quint64 kMB = kKB * 1024ULL;
constexpr quint64 kGB = kMB * 1024ULL;
constexpr quint64 kTB = kGB * 1024ULL;

/// Whether @a value, shown with one decimal, stays below 1,024.0 and so keeps its unit
bool showsBelowNextUnit(double value)
{
    return std::round(value * 10.0) < 10240.0;
}
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
    // The unit follows the rounded number: 1,048,575 bytes are 1.0MB, not 1,024.0KB
    if (showsBelowNextUnit(static_cast<double>(size) / kKB)) {
        return locale.toString(static_cast<double>(size) / kKB, 'f', 1) + QStringLiteral("KB");
    }
    if (showsBelowNextUnit(static_cast<double>(size) / kMB)) {
        return locale.toString(static_cast<double>(size) / kMB, 'f', 1) + QStringLiteral("MB");
    }
    if (showsBelowNextUnit(static_cast<double>(size) / kGB)) {
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
    if (showsBelowNextUnit(static_cast<double>(sizeMB) / kKB)) {
        return locale.toString(static_cast<double>(sizeMB) / kKB, 'f', 1) + QStringLiteral(" GB");
    }
    return locale.toString(static_cast<double>(sizeMB) / kMB, 'f', 2) + QStringLiteral(" TB");
}

}  // namespace QGC
