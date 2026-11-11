#pragma once

namespace QGCGeo {

/// Height of mean sea level (the EGM96 geoid) above the WGS84 ellipsoid in meters: the geoid separation of an NMEA
/// GGA sentence. A height above the ellipsoid is the height above mean sea level plus this value.
///
/// The value is interpolated from a 0.5 degree grid compiled into the program (0.5 MB), so no geoid data file is
/// needed. It is within 0.15 m RMS of the model and within 1 m on 99.8 % of the Earth. The grid flattens what is
/// narrower than its cells, by up to 4 m: in northern Colombia, on the Canary Islands and on Hawaii.
/// @return NaN when the latitude is outside +/-90 degrees or a value is not finite. Any finite longitude is accepted.
[[nodiscard]] double geoidSeparation(double latitudeDegrees, double longitudeDegrees);

}  // namespace QGCGeo
