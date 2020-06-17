/**
 * \file UTMUPS.hpp
 * \brief Header for GeographicLib::UTMUPS class
 *
 * Copyright (c) Charles Karney (2008-2019) <charles@karney.com> and licensed
 * under the MIT/X11 License.  For more information, see
 * https://geographiclib.sourceforge.io/
 **********************************************************************/

#if !defined(GEOGRAPHICLIB_UTMUPS_HPP)
#define GEOGRAPHICLIB_UTMUPS_HPP 1

#include "Constants.hpp"

namespace GeographicLib {

  /**
   * \brief Convert between geographic coordinates and UTM/UPS
   *
   * UTM and UPS are defined
   * - J. W. Hager, J. F. Behensky, and B. W. Drew,
   *   <a href="http://earth-info.nga.mil/GandG/publications/tm8358.2/TM8358_2.pdf">
   *   The Universal Grids: Universal Transverse Mercator (UTM) and Universal
   *   Polar Stereographic (UPS)</a>, Defense Mapping Agency, Technical Manual
   *   TM8358.2 (1989).
   * .
   * Section 2-3 defines UTM and section 3-2.4 defines UPS.  This document also
   * includes approximate algorithms for the computation of the underlying
   * transverse Mercator and polar stereographic projections.  Here we
   * substitute much more accurate algorithms given by
   * GeographicLib:TransverseMercator and GeographicLib:PolarStereographic.
   * These are the algorithms recommended by the NGA document
   * - <a href="http://earth-info.nga.mil/GandG/publications/NGA_SIG_0012_2_0_0_UTMUPS/NGA.SIG.0012_2.0.0_UTMUPS.pdf">
   *   The Universal Grids and the Transverse Mercator and Polar Stereographic
   *   Map Projections</a>, NGA.SIG.0012_2.0.0_UTMUPS (2014).
   *
   * In this implementation, the conversions are closed, i.e., output from
   * Forward is legal input for Reverse and vice versa.  The error is about 5nm
   * in each direction.  However, the conversion from legal UTM/UPS coordinates
   * to geographic coordinates and back might throw an error if the initial
   * point is within 5nm of the edge of the allowed range for the UTM/UPS
   * coordinates.
   *
   * The simplest way to guarantee the closed property is to define allowed
   * ranges for the eastings and northings for UTM and UPS coordinates.  The
   * UTM boundaries are the same for all zones.  (The only place the
   * exceptional nature of the zone boundaries is evident is when converting to
   * UTM/UPS coordinates requesting the standard zone.)  The MGRS lettering
   * scheme imposes natural limits on UTM/UPS coordinates which may be
   * converted into MGRS coordinates.  For the conversion to/from geographic
   * coordinates these ranges have been extended by 100km in order to provide a
   * generous overlap between UTM and UPS and between UTM zones.
   *
   * The <a href="http://www.nga.mil">NGA</a> software package
   * <a href="http://earth-info.nga.mil/GandG/geotrans/index.html">geotrans</a>
   * also provides conversions to and from UTM and UPS.  Version 2.4.2 (and
   * earlier) suffers from some drawbacks:
   * - Inconsistent rules are used to determine the whether a particular UTM or
   *   UPS coordinate is legal.  A more systematic approach is taken here.
   * - The underlying projections are not very accurately implemented.
   *
   * The GeographicLib::UTMUPS::EncodeZone encodes the UTM zone and hemisphere
   * to allow UTM/UPS coordinated to be displayed as, for example, "38N 444500
   * 3688500".  According to NGA.SIG.0012_2.0.0_UTMUPS the use of "N" to denote
   * "north" in the context is not allowed (since a upper case letter in this
   * context denotes the MGRS latitude band).  Consequently, as of version
   * 1.36, EncodeZone uses the lower case letters "n" and "s" to denote the
   * hemisphere.  In addition EncodeZone accepts an optional final argument \e
   * abbrev, which, if false, results in the hemisphere being spelled out as in
   * "38north".
   *
   * Example of use:
   * \include example-UTMUPS.cpp
   **********************************************************************/
  class GEOGRAPHICLIB_EXPORT UTMUPS {
  private:
    typedef Math::real real;
    static const int falseeasting_[4];
    static const int falsenorthing_[4];
    static const int mineasting_[4];
    static const int maxeasting_[4];
    static const int minnorthing_[4];
    static const int maxnorthing_[4];
    static const int epsg01N = 32601; // EPSG code for UTM 01N
    static const int epsg60N = 32660; // EPSG code for UTM 60N
    static const int epsgN   = 32661; // EPSG code for UPS   N
    static const int epsg01S = 32701; // EPSG code for UTM 01S
    static const int epsg60S = 32760; // EPSG code for UTM 60S
    static const int epsgS   = 32761; // EPSG code for UPS   S
    static real CentralMeridian(int zone)
    { return real(6 * zone - 183); }
    // Throw an error if easting or northing are outside standard ranges.  If
    // throwp = false, return bool instead.
    static bool CheckCoords(bool utmp, bool northp, real x, real y,
                            bool msgrlimits = false, bool throwp = true);
    UTMUPS();                   // Disable constructor

  public:

    /**
     * In this class we bring together the UTM and UPS coordinates systems.
     * The UTM divides the earth between latitudes &minus;80&deg; and 84&deg;
     * into 60 zones numbered 1 thru 60.  Zone assign zone number 0 to the UPS
     * regions, covering the two poles.  Within UTMUPS, non-negative zone
     * numbers refer to one of the "physical" zones, 0 for UPS and [1, 60] for
     * UTM.  Negative "pseudo-zone" numbers are used to select one of the
     * physical zones.
     **********************************************************************/
    enum zonespec {
      /**
       * The smallest pseudo-zone number.
       **********************************************************************/
      MINPSEUDOZONE = -4,
      /**
       * A marker for an undefined or invalid zone.  Equivalent to NaN.
       **********************************************************************/
      INVALID = -4,
      /**
       * If a coordinate already include zone information (e.g., it is an MGRS
       * coordinate), use that, otherwise apply the UTMUPS::STANDARD rules.
       **********************************************************************/
      MATCH = -3,
      /**
       * Apply the standard rules for UTM zone assigment extending the UTM zone
       * to each pole to give a zone number in [1, 60].  For example, use UTM
       * zone 38 for longitude in [42&deg;, 48&deg;).  The rules include the
       * Norway and Svalbard exceptions.
       **********************************************************************/
      UTM = -2,
      /**
       * Apply the standard rules for zone assignment to give a zone number in
       * [0, 60].  If the latitude is not in [&minus;80&deg;, 84&deg;), then
       * use UTMUPS::UPS = 0, otherwise apply the rules for UTMUPS::UTM.  The
       * tests on latitudes and longitudes are all closed on the lower end open
       * on the upper.  Thus for UTM zone 38, latitude is in [&minus;80&deg;,
       * 84&deg;) and longitude is in [42&deg;, 48&deg;).
       **********************************************************************/
      STANDARD = -1,
      /**
       * The largest pseudo-zone number.
       **********************************************************************/
      MAXPSEUDOZONE = -1,
      /**
       * The smallest physical zone number.
       **********************************************************************/
      MINZONE = 0,
      /**
       * The zone number used for UPS
       **********************************************************************/
      UPS = 0,
      /**
       * The smallest UTM zone number.
       **********************************************************************/
      MINUTMZONE = 1,
      /**
       * The largest UTM zone number.
       **********************************************************************/
      MAXUTMZONE = 60,
      /**
       * The largest physical zone number.
       **********************************************************************/
      MAXZONE = 60,
    };

    /**
     * The standard zone.
     *
     * @param[in] lat latitude (degrees).
     * @param[in] lon longitude (degrees).
     * @param[in] setzone zone override (optional).  If omitted, use the
     *   standard rules for picking the zone.  If \e setzone is given then use
     *   that zone if it is non-negative, otherwise apply the rules given in
     *   UTMUPS::zonespec.
     * @exception GeographicErr if \e setzone is outside the range
     *   [UTMUPS::MINPSEUDOZONE, UTMUPS::MAXZONE] = [&minus;4, 60].
     *
     * This is exact.
     **********************************************************************/
    static int StandardZone(real lat, real lon, int setzone = STANDARD);

    /**
     * Forward projection, from geographic to UTM/UPS.
     *
     * @param[in] lat latitude of point (degrees).
     * @param[in] lon longitude of point (degrees).
     * @param[out] zone the UTM zone (zero means UPS).
     * @param[out] northp hemisphere (true means north, false means south).
     * @param[out] x easting of point (meters).
     * @param[out] y northing of point (meters).
     * @param[out] gamma meridian convergence at point (degrees).
     * @param[out] k scale of projection at point.
     * @param[in] setzone zone override (optional).
     * @param[in] mgrslimits if true enforce the stricter MGRS limits on the
     *   coordinates (default = false).
     * @exception GeographicErr if \e lat is not in [&minus;90&deg;,
     *   90&deg;].
     * @exception GeographicErr if the resulting \e x or \e y is out of allowed
     *   range (see Reverse); in this case, these arguments are unchanged.
     *
     * If \e setzone is omitted, use the standard rules for picking the zone.
     * If \e setzone is given then use that zone if it is non-negative,
     * otherwise apply the rules given in UTMUPS::zonespec.  The accuracy of
     * the conversion is about 5nm.
     *
     * The northing \e y jumps by UTMUPS::UTMShift() when crossing the equator
     * in the southerly direction.  Sometimes it is useful to remove this
     * discontinuity in \e y by extending the "northern" hemisphere using
     * UTMUPS::Transfer:
     * \code
     double lat = -1, lon = 123;
     int zone;
     bool northp;
     double x, y, gamma, k;
     GeographicLib::UTMUPS::Forward(lat, lon, zone, northp, x, y, gamma, k);
     GeographicLib::UTMUPS::Transfer(zone, northp, x, y,
                                     zone, true,   x, y, zone);
     northp = true;
     \endcode
     **********************************************************************/
    static void Forward(real lat, real lon,
                        int& zone, bool& northp, real& x, real& y,
                        real& gamma, real& k,
                        int setzone = STANDARD, bool mgrslimits = false);

    /**
     * Reverse projection, from  UTM/UPS to geographic.
     *
     * @param[in] zone the UTM zone (zero means UPS).
     * @param[in] northp hemisphere (true means north, false means south).
     * @param[in] x easting of point (meters).
     * @param[in] y northing of point (meters).
     * @param[out] lat latitude of point (degrees).
     * @param[out] lon longitude of point (degrees).
     * @param[out] gamma meridian convergence at point (degrees).
     * @param[out] k scale of projection at point.
     * @param[in] mgrslimits if true enforce the stricter MGRS limits on the
     *   coordinates (default = false).
     * @exception GeographicErr if \e zone, \e x, or \e y is out of allowed
     *   range; this this case the arguments are unchanged.
     *
     * The accuracy of the conversion is about 5nm.
     *
     * UTM eastings are allowed to be in the range [0km, 1000km], northings are
     * allowed to be in in [0km, 9600km] for the northern hemisphere and in
     * [900km, 10000km] for the southern hemisphere.  However UTM northings
     * can be continued across the equator.  So the actual limits on the
     * northings are [-9100km, 9600km] for the "northern" hemisphere and
     * [900km, 19600km] for the "southern" hemisphere.
     *
     * UPS eastings and northings are allowed to be in the range [1200km,
     * 2800km] in the northern hemisphere and in [700km, 3300km] in the
     * southern hemisphere.
     *
     * These ranges are 100km larger than allowed for the conversions to MGRS.
     * (100km is the maximum extra padding consistent with eastings remaining
     * non-negative.)  This allows generous overlaps between zones and UTM and
     * UPS.  If \e mgrslimits = true, then all the ranges are shrunk by 100km
     * so that they agree with the stricter MGRS ranges.  No checks are
     * performed besides these (e.g., to limit the distance outside the
     * standard zone boundaries).
     **********************************************************************/
    static void Reverse(int zone, bool northp, real x, real y,
                        real& lat, real& lon, real& gamma, real& k,
                        bool mgrslimits = false);

    /**
     * UTMUPS::Forward without returning convergence and scale.
     **********************************************************************/
    static void Forward(real lat, real lon,
                        int& zone, bool& northp, real& x, real& y,
                        int setzone = STANDARD, bool mgrslimits = false) {
      real gamma, k;
      Forward(lat, lon, zone, northp, x, y, gamma, k, setzone, mgrslimits);
    }

    /**
     * UTMUPS::Reverse without returning convergence and scale.
     **********************************************************************/
    static void Reverse(int zone, bool northp, real x, real y,
                        real& lat, real& lon, bool mgrslimits = false) {
      real gamma, k;
      Reverse(zone, northp, x, y, lat, lon, gamma, k, mgrslimits);
    }

    /**
     * Transfer UTM/UPS coordinated from one zone to another.
     *
     * @param[in] zonein the UTM zone for \e xin and \e yin (or zero for UPS).
     * @param[in] northpin hemisphere for \e xin and \e yin (true means north,
     *   false means south).
     * @param[in] xin easting of point (meters) in \e zonein.
     * @param[in] yin northing of point (meters) in \e zonein.
     * @param[in] zoneout the requested UTM zone for \e xout and \e yout (or
     *   zero for UPS).
     * @param[in] northpout hemisphere for \e xout output and \e yout.
     * @param[out] xout easting of point (meters) in \e zoneout.
     * @param[out] yout northing of point (meters) in \e zoneout.
     * @param[out] zone the actual UTM zone for \e xout and \e yout (or zero
     *   for UPS); this equals \e zoneout if \e zoneout &ge; 0.
     * @exception GeographicErr if \e zonein is out of range (see below).
     * @exception GeographicErr if \e zoneout is out of range (see below).
     * @exception GeographicErr if \e xin or \e yin fall outside their allowed
     *   ranges (see UTMUPS::Reverse).
     * @exception GeographicErr if \e xout or \e yout fall outside their
     *   allowed ranges (see UTMUPS::Reverse).
     *
     * \e zonein must be in the range [UTMUPS::MINZONE, UTMUPS::MAXZONE] = [0,
     * 60] with \e zonein = UTMUPS::UPS, 0, indicating UPS.  \e zonein may
     * also be UTMUPS::INVALID.
     *
     * \e zoneout must be in the range [UTMUPS::MINPSEUDOZONE, UTMUPS::MAXZONE]
     * = [-4, 60].  If \e zoneout &lt; UTMUPS::MINZONE then the rules give in
     * the documentation of UTMUPS::zonespec are applied, and \e zone is set to
     * the actual zone used for output.
     *
     * (\e xout, \e yout) can overlap with (\e xin, \e yin).
     **********************************************************************/
    static void Transfer(int zonein, bool northpin, real xin, real yin,
                         int zoneout, bool northpout, real& xout, real& yout,
                         int& zone);

    /**
     * Decode a UTM/UPS zone string.
     *
     * @param[in] zonestr string representation of zone and hemisphere.
     * @param[out] zone the UTM zone (zero means UPS).
     * @param[out] northp hemisphere (true means north, false means south).
     * @exception GeographicErr if \e zonestr is malformed.
     *
     * For UTM, \e zonestr has the form of a zone number in the range
     * [UTMUPS::MINUTMZONE, UTMUPS::MAXUTMZONE] = [1, 60] followed by a
     * hemisphere letter, n or s (or "north" or "south" spelled out).  For UPS,
     * it consists just of the hemisphere letter (or the spelled out
     * hemisphere).  The returned value of \e zone is UTMUPS::UPS = 0 for UPS.
     * Note well that "38s" indicates the southern hemisphere of zone 38 and
     * not latitude band S, 32&deg; &le; \e lat &lt; 40&deg;.  n, 01s, 2n, 38s,
     * south, 3north are legal.  0n, 001s, +3n, 61n, 38P are illegal.  INV is a
     * special value for which the returned value of \e is UTMUPS::INVALID.
     **********************************************************************/
    static void DecodeZone(const std::string& zonestr,
                           int& zone, bool& northp);

    /**
     * Encode a UTM/UPS zone string.
     *
     * @param[in] zone the UTM zone (zero means UPS).
     * @param[in] northp hemisphere (true means north, false means south).
     * @param[in] abbrev if true (the default) use abbreviated (n/s) notation
     *   for hemisphere; otherwise spell out the hemisphere (north/south)
     * @exception GeographicErr if \e zone is out of range (see below).
     * @exception std::bad_alloc if memoy for the string can't be allocated.
     * @return string representation of zone and hemisphere.
     *
     * \e zone must be in the range [UTMUPS::MINZONE, UTMUPS::MAXZONE] = [0,
     * 60] with \e zone = UTMUPS::UPS, 0, indicating UPS (but the resulting
     * string does not contain "0").  \e zone may also be UTMUPS::INVALID, in
     * which case the returned string is "inv".  This reverses
     * UTMUPS::DecodeZone.
     **********************************************************************/
    static std::string EncodeZone(int zone, bool northp, bool abbrev = true);

    /**
     * Decode EPSG.
     *
     * @param[in] epsg the EPSG code.
     * @param[out] zone the UTM zone (zero means UPS).
     * @param[out] northp hemisphere (true means north, false means south).
     *
     * EPSG (European Petroleum Survery Group) codes are a way to refer to many
     * different projections.  DecodeEPSG decodes those referring to UTM or UPS
     * projections for the WGS84 ellipsoid.  If the code does not refer to one
     * of these projections, \e zone is set to UTMUPS::INVALID.  See
     * https://www.spatialreference.org/ref/epsg/
     **********************************************************************/
    static void DecodeEPSG(int epsg, int& zone, bool& northp);

    /**
     * Encode zone as EPSG.
     *
     * @param[in] zone the UTM zone (zero means UPS).
     * @param[in] northp hemisphere (true means north, false means south).
     * @return EPSG code (or -1 if \e zone is not in the range
     *   [UTMUPS::MINZONE, UTMUPS::MAXZONE] = [0, 60])
     *
     * Convert \e zone and \e northp to the corresponding EPSG (European
     * Petroleum Survery Group) codes
     **********************************************************************/
    static int EncodeEPSG(int zone, bool northp);

    /**
     * @return shift (meters) necessary to align north and south halves of a
     * UTM zone (10<sup>7</sup>).
     **********************************************************************/
    static Math::real UTMShift();

    /** \name Inspector functions
     **********************************************************************/
    ///@{
    /**
     * @return \e a the equatorial radius of the WGS84 ellipsoid (meters).
     *
     * (The WGS84 value is returned because the UTM and UPS projections are
     * based on this ellipsoid.)
     **********************************************************************/
    static Math::real EquatorialRadius()
    { return Constants::WGS84_a(); }

    /**
     * @return \e f the flattening of the WGS84 ellipsoid.
     *
     * (The WGS84 value is returned because the UTM and UPS projections are
     * based on this ellipsoid.)
     **********************************************************************/
    static Math::real Flattening()
    { return Constants::WGS84_f(); }

    /**
      * \deprecated An old name for EquatorialRadius().
      **********************************************************************/
    // GEOGRAPHICLIB_DEPRECATED("Use EquatorialRadius()")
    static Math::real MajorRadius() { return EquatorialRadius(); }
    ///@}

  };

} // namespace GeographicLib

#endif  // GEOGRAPHICLIB_UTMUPS_HPP
