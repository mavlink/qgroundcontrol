#ifndef QGCGEO_H
#define QGCGEO_H

/**
 * Converting from latitude / longitude to tangent on earth surface
 * @link http://psas.pdx.edu/CoordinateSystem/Latitude_to_LocalTangent.pdf
 * @link http://dspace.dsto.defence.gov.au/dspace/bitstream/1947/3538/1/DSTO-TN-0432.pdf
 */
void LatLonToENU(double lat, double lon, double alt, double originLat, double originLon, double originAlt, double* x, double* y, double* z)
{

}

#endif // QGCGEO_H
