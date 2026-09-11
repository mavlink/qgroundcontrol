#pragma once

#include <QtCore/QByteArray>
#include <QtPositioning/QGeoCoordinate>

#include "GPSObservation.h"

namespace NMEAUtils {

/// Explicit talker/system context takes precedence. Legacy GN IDs with overlapping
/// receiver numbering schemes remain Unknown; no missing ID implies a zero count.
GPSSatellite::Constellation satelliteConstellation(const QByteArray& talker, std::optional<int> systemId = std::nullopt,
                                                   std::optional<int> satelliteId = std::nullopt);

/// Compute XOR checksum over NMEA body (between '$' and '*').
quint8 computeChecksum(const QByteArray& body);

/// Validate an NMEA sentence's checksum. Accepts sentences of the form
/// `$BODY*XX` (with or without CRLF suffix). Returns false if the sentence
/// is malformed or the checksum does not match.
bool verifyChecksum(const QByteArray& sentence);

/// Repair or append a valid NMEA checksum and ensure CRLF termination.
QByteArray repairChecksum(const QByteArray& sentence);

/// Build a GGA sentence from a coordinate and altitude.
QByteArray makeGGA(const QGeoCoordinate& coord, double altitudeMsl, int fixQuality = 1, int numSatellites = -1);

/// Encode available observation metadata; unknown quality, satellite, DOP and altitude fields stay empty.
QByteArray makeGGA(const GPSObservation& observation);

}  // namespace NMEAUtils
