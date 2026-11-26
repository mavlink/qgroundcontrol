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
    /// The number shown is below 1,024: a size that rounds up to it is shown as 1.0 of the next unit.
    QString bigSizeToString(quint64 size);

    /// MB-scaled size, output in MB/GB/TB. Input is in MB.
    /// The GB shown are below 1,024.0: a size that rounds up to it is shown as 1.00 TB.
    QString bigSizeMBToString(quint64 sizeMB);
}
