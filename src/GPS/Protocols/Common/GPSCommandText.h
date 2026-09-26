#pragma once

#include <QtCore/QByteArray>

/// Formats @a value as printf's "%.<decimals>f" does in the C locale: exact binary ties round to even and a negative
/// zero keeps its sign. QByteArray::number() does neither, and would change the command bytes for such values.
QByteArray gpsFixedDecimal(double value, int decimals);
