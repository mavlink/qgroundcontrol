#pragma once

#include <QtCore/QtTypes>

class QString;

// Locale-aware. QGCApplication sets QLocale::setDefault() at startup, so a
// default-constructed QLocale matches the app's active language.
namespace QGC
{
    /// Decimal integer (e.g. "1,234,567").
    QString numberToString(quint64 number);

    /// Byte size with unit: B, KB, MB, GB, TB. 1 fractional digit above 1 KB.
    QString bigSizeToString(quint64 size);

    /// MB-scaled size, output in MB/GB/TB. Input is in MB.
    QString bigSizeMBToString(quint64 sizeMB);
}
