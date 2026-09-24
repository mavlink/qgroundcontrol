#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QTime>

namespace NMEA {
struct GGA;
}

namespace NMEAUtils {
/// Validate an NMEA sentence's checksum. Accepts sentences of the form
/// `$BODY*XX` (with no terminator, LF, or CRLF). Returns false if the sentence
/// is malformed or the checksum does not match.
bool verifyChecksum(const QByteArray& sentence);

/// Rebuild a valid frame with its checksum and CRLF; short or malformed bodies are only terminated.
QByteArray repairChecksum(const QByteArray& sentence);

/// Build GPGGA from explicit fix fields and UTC, truncated to whole seconds.
/// NaN altitude/geoid separation/HDOP and absent satellite count produce empty fields, not defaults.
/// Returns empty for invalid UTC, out-of-range/nonfinite coordinates, infinite measurements,
/// negative HDOP, or quality above the protocol maximum. Altitude is MSL; geoid separation is signed.
QByteArray makeGGA(const NMEA::GGA& fix, const QTime& utc);

}  // namespace NMEAUtils
