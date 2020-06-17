/**
 * \file MGRS.hpp
 * \brief Header for GeographicLib::MGRS class
 *
 * Copyright (c) Charles Karney (2008-2019) <charles@karney.com> and licensed
 * under the MIT/X11 License.  For more information, see
 * https://geographiclib.sourceforge.io/
 **********************************************************************/

#if !defined(GEOGRAPHICLIB_MGRS_HPP)
#define GEOGRAPHICLIB_MGRS_HPP 1

#include "Constants.hpp"
#include "UTMUPS.hpp"

#if defined(_MSC_VER)
// Squelch warnings about dll vs string
#  pragma warning (push)
#  pragma warning (disable: 4251)
#endif

namespace GeographicLib {

  /**
   * \brief Convert between UTM/UPS and %MGRS
   *
   * MGRS is defined in Chapter 3 of
   * - J. W. Hager, L. L. Fry, S. S. Jacks, D. R. Hill,
   *   <a href="http://earth-info.nga.mil/GandG/publications/tm8358.1/pdf/TM8358_1.pdf">
   *   Datums, Ellipsoids, Grids, and Grid Reference Systems</a>,
   *   Defense Mapping Agency, Technical Manual TM8358.1 (1990).
   * .
   * This document has been updated by the two NGA documents
   * - <a href="http://earth-info.nga.mil/GandG/publications/NGA_STND_0037_2_0_0_GRIDS/NGA.STND.0037_2.0.0_GRIDS.pdf">
   *   Universal Grids and Grid Reference Systems</a>,
   *   NGA.STND.0037_2.0.0_GRIDS (2014).
   * - <a href="http://earth-info.nga.mil/GandG/publications/NGA_SIG_0012_2_0_0_UTMUPS/NGA.SIG.0012_2.0.0_UTMUPS.pdf">
   *   The Universal Grids and the Transverse Mercator and Polar Stereographic
   *   Map Projections</a>, NGA.SIG.0012_2.0.0_UTMUPS (2014).
   *
   * This implementation has the following properties:
   * - The conversions are closed, i.e., output from Forward is legal input for
   *   Reverse and vice versa.  Conversion in both directions preserve the
   *   UTM/UPS selection and the UTM zone.
   * - Forward followed by Reverse and vice versa is approximately the
   *   identity.  (This is affected in predictable ways by errors in
   *   determining the latitude band and by loss of precision in the MGRS
   *   coordinates.)
   * - The trailing digits produced by Forward are consistent as the precision
   *   is varied.  Specifically, the digits are obtained by operating on the
   *   easting with &lfloor;10<sup>6</sup> <i>x</i>&rfloor; and extracting the
   *   required digits from the resulting number (and similarly for the
   *   northing).
   * - All MGRS coordinates truncate to legal 100 km blocks.  All MGRS
   *   coordinates with a legal 100 km block prefix are legal (even though the
   *   latitude band letter may now belong to a neighboring band).
   * - The range of UTM/UPS coordinates allowed for conversion to MGRS
   *   coordinates is the maximum consistent with staying within the letter
   *   ranges of the MGRS scheme.
   * - All the transformations are implemented as static methods in the MGRS
   *   class.
   *
   * The <a href="http://www.nga.mil">NGA</a> software package
   * <a href="http://earth-info.nga.mil/GandG/geotrans/index.html">geotrans</a>
   * also provides conversions to and from MGRS.  Version 3.0 (and earlier)
   * suffers from some drawbacks:
   * - Inconsistent rules are used to determine the whether a particular MGRS
   *   coordinate is legal.  A more systematic approach is taken here.
   * - The underlying projections are not very accurately implemented.
   *
   * Example of use:
   * \include example-MGRS.cpp
   **********************************************************************/
  class GEOGRAPHICLIB_EXPORT MGRS {
  private:
    typedef Math::real real;
    static const char* const hemispheres_;
    static const char* const utmcols_[3];
    static const char* const utmrow_;
    static const char* const upscols_[4];
    static const char* const upsrows_[2];
    static const char* const latband_;
    static const char* const upsband_;
    static const char* const digits_;

    static const int mineasting_[4];
    static const int maxeasting_[4];
    static const int minnorthing_[4];
    static const int maxnorthing_[4];
    enum {
      base_ = 10,
      // Top-level tiles are 10^5 m = 100 km on a side
      tilelevel_ = 5,
      // Period of UTM row letters
      utmrowperiod_ = 20,
      // Row letters are shifted by 5 for even zones
      utmevenrowshift_ = 5,
      // Maximum precision is um
      maxprec_ = 5 + 6,
      // For generating digits at maxprec
      mult_ = 1000000,
    };
    static void CheckCoords(bool utmp, bool& northp, real& x, real& y);
    static int UTMRow(int iband, int icol, int irow);

    friend class UTMUPS;        // UTMUPS::StandardZone calls LatitudeBand
    // Return latitude band number [-10, 10) for the given latitude (degrees).
    // The bands are reckoned in include their southern edges.
    static int LatitudeBand(real lat) {
      using std::floor;
      int ilat = int(floor(lat));
      return (std::max)(-10, (std::min)(9, (ilat + 80)/8 - 10));
    }
    // Return approximate latitude band number [-10, 10) for the given northing
    // (meters).  With this rule, each 100km tile would have a unique band
    // letter corresponding to the latitude at the center of the tile.  This
    // function isn't currently used.
    static int ApproxLatitudeBand(real y) {
      // northing at tile center in units of tile = 100km
      using std::floor; using std::abs;
      real ya = floor( (std::min)(real(88), abs(y/tile_)) ) +
        real(0.5);
      // convert to lat (mult by 90/100) and then to band (divide by 8)
      // the +1 fine tunes the boundary between bands 3 and 4
      int b = int(floor( ((ya * 9 + 1) / 10) / 8 ));
      // For the northern hemisphere we have
      // band rows  num
      // N 0   0:8    9
      // P 1   9:17   9
      // Q 2  18:26   9
      // R 3  27:34   8
      // S 4  35:43   9
      // T 5  44:52   9
      // U 6  53:61   9
      // V 7  62:70   9
      // W 8  71:79   9
      // X 9  80:94  15
      return y >= 0 ? b : -(b + 1);
    }
    // UTMUPS access these enums
    enum {
      tile_ = 100000,            // Size MGRS blocks
      minutmcol_ = 1,
      maxutmcol_ = 9,
      minutmSrow_ = 10,
      maxutmSrow_ = 100,         // Also used for UTM S false northing
      minutmNrow_ = 0,           // Also used for UTM N false northing
      maxutmNrow_ = 95,
      minupsSind_ = 8,           // These 4 ind's apply to easting and northing
      maxupsSind_ = 32,
      minupsNind_ = 13,
      maxupsNind_ = 27,
      upseasting_ = 20,          // Also used for UPS false northing
      utmeasting_ = 5,           // UTM false easting
      // Difference between S hemisphere northing and N hemisphere northing
      utmNshift_ = (maxutmSrow_ - minutmNrow_) * tile_
    };
    MGRS();                     // Disable constructor

  public:

    /**
     * Convert UTM or UPS coordinate to an MGRS coordinate.
     *
     * @param[in] zone UTM zone (zero means UPS).
     * @param[in] northp hemisphere (true means north, false means south).
     * @param[in] x easting of point (meters).
     * @param[in] y northing of point (meters).
     * @param[in] prec precision relative to 100 km.
     * @param[out] mgrs MGRS string.
     * @exception GeographicErr if \e zone, \e x, or \e y is outside its
     *   allowed range.
     * @exception GeographicErr if the memory for the MGRS string can't be
     *   allocated.
     *
     * \e prec specifies the precision of the MGRS string as follows:
     * - \e prec = &minus;1 (min), only the grid zone is returned
     * - \e prec = 0, 100 km
     * - \e prec = 1, 10 km
     * - \e prec = 2, 1 km
     * - \e prec = 3, 100 m
     * - \e prec = 4, 10 m
     * - \e prec = 5, 1 m
     * - \e prec = 6, 0.1 m
     * - &hellip;
     * - \e prec = 11 (max), 1 &mu;m
     *
     * UTM eastings are allowed to be in the range [100 km, 900 km], northings
     * are allowed to be in in [0 km, 9500 km] for the northern hemisphere and
     * in [1000 km, 10000 km] for the southern hemisphere.  (However UTM
     * northings can be continued across the equator.  So the actual limits on
     * the northings are [&minus;9000 km, 9500 km] for the "northern"
     * hemisphere and [1000 km, 19500 km] for the "southern" hemisphere.)
     *
     * UPS eastings/northings are allowed to be in the range [1300 km, 2700 km]
     * in the northern hemisphere and in [800 km, 3200 km] in the southern
     * hemisphere.
     *
     * The ranges are 100 km more restrictive than for the conversion between
     * geographic coordinates and UTM and UPS given by UTMUPS.  These
     * restrictions are dictated by the allowed letters in MGRS coordinates.
     * The choice of 9500 km for the maximum northing for northern hemisphere
     * and of 1000 km as the minimum northing for southern hemisphere provide
     * at least 0.5 degree extension into standard UPS zones.  The upper ends
     * of the ranges for the UPS coordinates is dictated by requiring symmetry
     * about the meridians 0E and 90E.
     *
     * All allowed UTM and UPS coordinates may now be converted to legal MGRS
     * coordinates with the proviso that eastings and northings on the upper
     * boundaries are silently reduced by about 4 nm (4 nanometers) to place
     * them \e within the allowed range.  (This includes reducing a southern
     * hemisphere northing of 10000 km by 4 nm so that it is placed in latitude
     * band M.)  The UTM or UPS coordinates are truncated to requested
     * precision to determine the MGRS coordinate.  Thus in UTM zone 38n, the
     * square area with easting in [444 km, 445 km) and northing in [3688 km,
     * 3689 km) maps to MGRS coordinate 38SMB4488 (at \e prec = 2, 1 km),
     * Khulani Sq., Baghdad.
     *
     * The UTM/UPS selection and the UTM zone is preserved in the conversion to
     * MGRS coordinate.  Thus for \e zone > 0, the MGRS coordinate begins with
     * the zone number followed by one of [C--M] for the southern
     * hemisphere and [N--X] for the northern hemisphere.  For \e zone =
     * 0, the MGRS coordinates begins with one of [AB] for the southern
     * hemisphere and [XY] for the northern hemisphere.
     *
     * The conversion to the MGRS is exact for prec in [0, 5] except that a
     * neighboring latitude band letter may be given if the point is within 5nm
     * of a band boundary.  For prec in [6, 11], the conversion is accurate to
     * roundoff.
     *
     * If \e prec = &minus;1, then the "grid zone designation", e.g., 18T, is
     * returned.  This consists of the UTM zone number (absent for UPS) and the
     * first letter of the MGRS string which labels the latitude band for UTM
     * and the hemisphere for UPS.
     *
     * If \e x or \e y is NaN or if \e zone is UTMUPS::INVALID, the returned
     * MGRS string is "INVALID".
     *
     * Return the result via a reference argument to avoid the overhead of
     * allocating a potentially large number of small strings.  If an error is
     * thrown, then \e mgrs is unchanged.
     **********************************************************************/
    static void Forward(int zone, bool northp, real x, real y,
                        int prec, std::string& mgrs);

    /**
     * Convert UTM or UPS coordinate to an MGRS coordinate when the latitude is
     * known.
     *
     * @param[in] zone UTM zone (zero means UPS).
     * @param[in] northp hemisphere (true means north, false means south).
     * @param[in] x easting of point (meters).
     * @param[in] y northing of point (meters).
     * @param[in] lat latitude (degrees).
     * @param[in] prec precision relative to 100 km.
     * @param[out] mgrs MGRS string.
     * @exception GeographicErr if \e zone, \e x, or \e y is outside its
     *   allowed range.
     * @exception GeographicErr if \e lat is inconsistent with the given UTM
     *   coordinates.
     * @exception std::bad_alloc if the memory for \e mgrs can't be allocated.
     *
     * The latitude is ignored for \e zone = 0 (UPS); otherwise the latitude is
     * used to determine the latitude band and this is checked for consistency
     * using the same tests as Reverse.
     **********************************************************************/
    static void Forward(int zone, bool northp, real x, real y, real lat,
                        int prec, std::string& mgrs);

    /**
     * Convert a MGRS coordinate to UTM or UPS coordinates.
     *
     * @param[in] mgrs MGRS string.
     * @param[out] zone UTM zone (zero means UPS).
     * @param[out] northp hemisphere (true means north, false means south).
     * @param[out] x easting of point (meters).
     * @param[out] y northing of point (meters).
     * @param[out] prec precision relative to 100 km.
     * @param[in] centerp if true (default), return center of the MGRS square,
     *   else return SW (lower left) corner.
     * @exception GeographicErr if \e mgrs is illegal.
     *
     * All conversions from MGRS to UTM/UPS are permitted provided the MGRS
     * coordinate is a possible result of a conversion in the other direction.
     * (The leading 0 may be dropped from an input MGRS coordinate for UTM
     * zones 1--9.)  In addition, MGRS coordinates with a neighboring
     * latitude band letter are permitted provided that some portion of the
     * 100 km block is within the given latitude band.  Thus
     * - 38VLS and 38WLS are allowed (latitude 64N intersects the square
     *   38[VW]LS); but 38VMS is not permitted (all of 38WMS is north of 64N)
     * - 38MPE and 38NPF are permitted (they straddle the equator); but 38NPE
     *   and 38MPF are not permitted (the equator does not intersect either
     *   block).
     * - Similarly ZAB and YZB are permitted (they straddle the prime
     *   meridian); but YAB and ZZB are not (the prime meridian does not
     *   intersect either block).
     *
     * The UTM/UPS selection and the UTM zone is preserved in the conversion
     * from MGRS coordinate.  The conversion is exact for prec in [0, 5].  With
     * \e centerp = true, the conversion from MGRS to geographic and back is
     * stable.  This is not assured if \e centerp = false.
     *
     * If a "grid zone designation" (for example, 18T or A) is given, then some
     * suitable (but essentially arbitrary) point within that grid zone is
     * returned.  The main utility of the conversion is to allow \e zone and \e
     * northp to be determined.  In this case, the \e centerp parameter is
     * ignored and \e prec is set to &minus;1.
     *
     * If the first 3 characters of \e mgrs are "INV", then \e x and \e y are
     * set to NaN, \e zone is set to UTMUPS::INVALID, and \e prec is set to
     * &minus;2.
     *
     * If an exception is thrown, then the arguments are unchanged.
     **********************************************************************/
    static void Reverse(const std::string& mgrs,
                        int& zone, bool& northp, real& x, real& y,
                        int& prec, bool centerp = true);

    /** \name Inspector functions
     **********************************************************************/
    ///@{
    /**
     * @return \e a the equatorial radius of the WGS84 ellipsoid (meters).
     *
     * (The WGS84 value is returned because the UTM and UPS projections are
     * based on this ellipsoid.)
     **********************************************************************/
    static Math::real EquatorialRadius() { return UTMUPS::EquatorialRadius(); }

    /**
     * @return \e f the flattening of the WGS84 ellipsoid.
     *
     * (The WGS84 value is returned because the UTM and UPS projections are
     * based on this ellipsoid.)
     **********************************************************************/
    static Math::real Flattening() { return UTMUPS::Flattening(); }

    /**
      * \deprecated An old name for EquatorialRadius().
      **********************************************************************/
    // GEOGRAPHICLIB_DEPRECATED("Use EquatorialRadius()")
    static Math::real MajorRadius() { return EquatorialRadius(); }
    ///@}

    /**
     * Perform some checks on the UTMUPS coordinates on this ellipsoid.  Throw
     * an error if any of the assumptions made in the MGRS class is not true.
     * This check needs to be carried out if the ellipsoid parameters (or the
     * UTM/UPS scales) are ever changed.
     **********************************************************************/
    static void Check();

  };

} // namespace GeographicLib

#if defined(_MSC_VER)
#  pragma warning (pop)
#endif

#endif  // GEOGRAPHICLIB_MGRS_HPP
