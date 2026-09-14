#pragma once

#include <QtCore/QByteArray>
#include <QtPositioning/QGeoCoordinate>

namespace NMEAUtils {
/// Validate an NMEA sentence's checksum. Accepts sentences of the form
/// `$BODY*XX` (with no terminator, LF, or CRLF). Returns false if the sentence
/// is malformed or the checksum does not match.
bool verifyChecksum(const QByteArray& sentence);

/// Rebuild a valid frame with its checksum and CRLF; short or malformed bodies are only terminated.
QByteArray repairChecksum(const QByteArray& sentence);

/// Build a GGA sentence from a coordinate and altitude.
QByteArray makeGGA(const QGeoCoordinate& coord, double altitudeMsl, int fixQuality = 1, int numSatellites = 12);

}  // namespace NMEAUtils
