/**
 * \file MGRS.cpp
 * \brief Implementation for GeographicLib::MGRS class
 *
 * Copyright (c) Charles Karney (2008-2017) <charles@karney.com> and licensed
 * under the MIT/X11 License.  For more information, see
 * https://geographiclib.sourceforge.io/
 **********************************************************************/

#include "MGRS.hpp"
#include "Utility.hpp"

namespace GeographicLib {

  using namespace std;

  const char* const MGRS::hemispheres_ = "SN";
  const char* const MGRS::utmcols_[] = { "ABCDEFGH", "JKLMNPQR", "STUVWXYZ" };
  const char* const MGRS::utmrow_ = "ABCDEFGHJKLMNPQRSTUV";
  const char* const MGRS::upscols_[] =
    { "JKLPQRSTUXYZ", "ABCFGHJKLPQR", "RSTUXYZ", "ABCFGHJ" };
  const char* const MGRS::upsrows_[] =
    { "ABCDEFGHJKLMNPQRSTUVWXYZ", "ABCDEFGHJKLMNP" };
  const char* const MGRS::latband_ = "CDEFGHJKLMNPQRSTUVWX";
  const char* const MGRS::upsband_ = "ABYZ";
  const char* const MGRS::digits_ = "0123456789";

  const int MGRS::mineasting_[] =
    { minupsSind_, minupsNind_, minutmcol_, minutmcol_ };
  const int MGRS::maxeasting_[] =
    { maxupsSind_, maxupsNind_, maxutmcol_, maxutmcol_ };
  const int MGRS::minnorthing_[] =
    { minupsSind_, minupsNind_,
      minutmSrow_, minutmSrow_ - (maxutmSrow_ - minutmNrow_) };
  const int MGRS::maxnorthing_[] =
    { maxupsSind_, maxupsNind_,
      maxutmNrow_ + (maxutmSrow_ - minutmNrow_), maxutmNrow_ };

  void MGRS::Forward(int zone, bool northp, real x, real y, real lat,
                     int prec, std::string& mgrs) {
    // The smallest angle s.t., 90 - angeps() < 90 (approx 50e-12 arcsec)
    // 7 = ceil(log_2(90))
    static const real angeps = ldexp(real(1), -(Math::digits() - 7));
    if (zone == UTMUPS::INVALID ||
        Math::isnan(x) || Math::isnan(y) || Math::isnan(lat)) {
      mgrs = "INVALID";
      return;
    }
    bool utmp = zone != 0;
    CheckCoords(utmp, northp, x, y);
    if (!(zone >= UTMUPS::MINZONE && zone <= UTMUPS::MAXZONE))
      throw GeographicErr("Zone " + Utility::str(zone) + " not in [0,60]");
    if (!(prec >= -1 && prec <= maxprec_))
      throw GeographicErr("MGRS precision " + Utility::str(prec)
                          + " not in [-1, "
                          + Utility::str(int(maxprec_)) + "]");
    // Fixed char array for accumulating string.  Allow space for zone, 3 block
    // letters, easting + northing.  Don't need to allow for terminating null.
    char mgrs1[2 + 3 + 2 * maxprec_];
    int
      zone1 = zone - 1,
      z = utmp ? 2 : 0,
      mlen = z + 3 + 2 * prec;
    if (utmp) {
      mgrs1[0] = digits_[ zone / base_ ];
      mgrs1[1] = digits_[ zone % base_ ];
      // This isn't necessary...!  Keep y non-neg
      // if (!northp) y -= maxutmSrow_ * tile_;
    }
    // The C++ standard mandates 64 bits for long long.  But
    // check, to make sure.
    GEOGRAPHICLIB_STATIC_ASSERT(numeric_limits<long long>::digits >= 44,
                                "long long not wide enough to store 10e12");
    long long
      ix = (long long)(floor(x * mult_)),
      iy = (long long)(floor(y * mult_)),
      m = (long long)(mult_) * (long long)(tile_);
    int xh = int(ix / m), yh = int(iy / m);
    if (utmp) {
      int
        // Correct fuzziness in latitude near equator
        iband = abs(lat) > angeps ? LatitudeBand(lat) : (northp ? 0 : -1),
        icol = xh - minutmcol_,
        irow = UTMRow(iband, icol, yh % utmrowperiod_);
      if (irow != yh - (northp ? minutmNrow_ : maxutmSrow_))
        throw GeographicErr("Latitude " + Utility::str(lat)
                            + " is inconsistent with UTM coordinates");
      mgrs1[z++] = latband_[10 + iband];
      mgrs1[z++] = utmcols_[zone1 % 3][icol];
      mgrs1[z++] = utmrow_[(yh + (zone1 & 1 ? utmevenrowshift_ : 0))
                         % utmrowperiod_];
    } else {
      bool eastp = xh >= upseasting_;
      int iband = (northp ? 2 : 0) + (eastp ? 1 : 0);
      mgrs1[z++] = upsband_[iband];
      mgrs1[z++] = upscols_[iband][xh - (eastp ? upseasting_ :
                                         (northp ? minupsNind_ :
                                          minupsSind_))];
      mgrs1[z++] = upsrows_[northp][yh - (northp ? minupsNind_ : minupsSind_)];
    }
    if (prec > 0) {
      ix -= m * xh; iy -= m * yh;
      long long d = (long long)(pow(real(base_), maxprec_ - prec));
      ix /= d; iy /= d;
      for (int c = prec; c--;) {
        mgrs1[z + c       ] = digits_[ix % base_]; ix /= base_;
        mgrs1[z + c + prec] = digits_[iy % base_]; iy /= base_;
      }
    }
    mgrs.resize(mlen);
    copy(mgrs1, mgrs1 + mlen, mgrs.begin());
  }

  void MGRS::Forward(int zone, bool northp, real x, real y,
                     int prec, std::string& mgrs) {
    real lat, lon;
    if (zone > 0) {
      // Does a rough estimate for latitude determine the latitude band?
      real ys = northp ? y : y - utmNshift_;
      // A cheap calculation of the latitude which results in an "allowed"
      // latitude band would be
      //   lat = ApproxLatitudeBand(ys) * 8 + 4;
      //
      // Here we do a more careful job using the band letter corresponding to
      // the actual latitude.
      ys /= tile_;
      if (abs(ys) < 1)
        lat = real(0.9) * ys;         // accurate enough estimate near equator
      else {
        real
          // The poleward bound is a fit from above of lat(x,y)
          // for x = 500km and y = [0km, 950km]
          latp = real(0.901) * ys + (ys > 0 ? 1 : -1) * real(0.135),
          // The equatorward bound is a fit from below of lat(x,y)
          // for x = 900km and y = [0km, 950km]
          late = real(0.902) * ys * (1 - real(1.85e-6) * ys * ys);
        if (LatitudeBand(latp) == LatitudeBand(late))
          lat = latp;
        else
          // bounds straddle a band boundary so need to compute lat accurately
          UTMUPS::Reverse(zone, northp, x, y, lat, lon);
      }
    } else
      // Latitude isn't needed for UPS specs or for INVALID
      lat = 0;
    Forward(zone, northp, x, y, lat, prec, mgrs);
  }

  void MGRS::Reverse(const std::string& mgrs,
                     int& zone, bool& northp, real& x, real& y,
                     int& prec, bool centerp) {
    int
      p = 0,
      len = int(mgrs.length());
    if (len >= 3 &&
        toupper(mgrs[0]) == 'I' &&
        toupper(mgrs[1]) == 'N' &&
        toupper(mgrs[2]) == 'V') {
      zone = UTMUPS::INVALID;
      northp = false;
      x = y = Math::NaN();
      prec = -2;
      return;
    }
    int zone1 = 0;
    while (p < len) {
      int i = Utility::lookup(digits_, mgrs[p]);
      if (i < 0)
        break;
      zone1 = 10 * zone1 + i;
      ++p;
    }
    if (p > 0 && !(zone1 >= UTMUPS::MINUTMZONE && zone1 <= UTMUPS::MAXUTMZONE))
      throw GeographicErr("Zone " + Utility::str(zone1) + " not in [1,60]");
    if (p > 2)
      throw GeographicErr("More than 2 digits at start of MGRS "
                          + mgrs.substr(0, p));
    if (len - p < 1)
      throw GeographicErr("MGRS string too short " + mgrs);
    bool utmp = zone1 != UTMUPS::UPS;
    int zonem1 = zone1 - 1;
    const char* band = utmp ? latband_ : upsband_;
    int iband = Utility::lookup(band, mgrs[p++]);
    if (iband < 0)
      throw GeographicErr("Band letter " + Utility::str(mgrs[p-1]) + " not in "
                          + (utmp ? "UTM" : "UPS") + " set " + band);
    bool northp1 = iband >= (utmp ? 10 : 2);
    if (p == len) {             // Grid zone only (ignore centerp)
      // Approx length of a degree of meridian arc in units of tile.
      real deg = real(utmNshift_) / (90 * tile_);
      zone = zone1;
      northp = northp1;
      if (utmp) {
        // Pick central meridian except for 31V
        x = ((zone == 31 && iband == 17) ? 4 : 5) * tile_;
        // Pick center of 8deg latitude bands
        y = floor(8 * (iband - real(9.5)) * deg + real(0.5)) * tile_
          + (northp ? 0 : utmNshift_);
      } else {
        // Pick point at lat 86N or 86S
        x = ((iband & 1 ? 1 : -1) * floor(4 * deg + real(0.5))
             + upseasting_) * tile_;
        // Pick point at lon 90E or 90W.
        y = upseasting_ * tile_;
      }
      prec = -1;
      return;
    } else if (len - p < 2)
      throw GeographicErr("Missing row letter in " + mgrs);
    const char* col = utmp ? utmcols_[zonem1 % 3] : upscols_[iband];
    const char* row = utmp ? utmrow_ : upsrows_[northp1];
    int icol = Utility::lookup(col, mgrs[p++]);
    if (icol < 0)
      throw GeographicErr("Column letter " + Utility::str(mgrs[p-1])
                          + " not in "
                          + (utmp ? "zone " + mgrs.substr(0, p-2) :
                             "UPS band " + Utility::str(mgrs[p-2]))
                          + " set " + col );
    int irow = Utility::lookup(row, mgrs[p++]);
    if (irow < 0)
      throw GeographicErr("Row letter " + Utility::str(mgrs[p-1]) + " not in "
                          + (utmp ? "UTM" :
                             "UPS " + Utility::str(hemispheres_[northp1]))
                          + " set " + row);
    if (utmp) {
      if (zonem1 & 1)
        irow = (irow + utmrowperiod_ - utmevenrowshift_) % utmrowperiod_;
      iband -= 10;
      irow = UTMRow(iband, icol, irow);
      if (irow == maxutmSrow_)
        throw GeographicErr("Block " + mgrs.substr(p-2, 2)
                            + " not in zone/band " + mgrs.substr(0, p-2));

      irow = northp1 ? irow : irow + 100;
      icol = icol + minutmcol_;
    } else {
      bool eastp = iband & 1;
      icol += eastp ? upseasting_ : (northp1 ? minupsNind_ : minupsSind_);
      irow += northp1 ? minupsNind_ : minupsSind_;
    }
    int prec1 = (len - p)/2;
    real
      unit = 1,
      x1 = icol,
      y1 = irow;
    for (int i = 0; i < prec1; ++i) {
      unit *= base_;
      int
        ix = Utility::lookup(digits_, mgrs[p + i]),
        iy = Utility::lookup(digits_, mgrs[p + i + prec1]);
      if (ix < 0 || iy < 0)
        throw GeographicErr("Encountered a non-digit in " + mgrs.substr(p));
      x1 = base_ * x1 + ix;
      y1 = base_ * y1 + iy;
    }
    if ((len - p) % 2) {
      if (Utility::lookup(digits_, mgrs[len - 1]) < 0)
        throw GeographicErr("Encountered a non-digit in " + mgrs.substr(p));
      else
        throw GeographicErr("Not an even number of digits in "
                            + mgrs.substr(p));
    }
    if (prec1 > maxprec_)
      throw GeographicErr("More than " + Utility::str(2*maxprec_)
                          + " digits in " + mgrs.substr(p));
    if (centerp) {
      unit *= 2; x1 = 2 * x1 + 1; y1 = 2 * y1 + 1;
    }
    zone = zone1;
    northp = northp1;
    x = (tile_ * x1) / unit;
    y = (tile_ * y1) / unit;
    prec = prec1;
  }

  void MGRS::CheckCoords(bool utmp, bool& northp, real& x, real& y) {
    // Limits are all multiples of 100km and are all closed on the lower end
    // and open on the upper end -- and this is reflected in the error
    // messages.  However if a coordinate lies on the excluded upper end (e.g.,
    // after rounding), it is shifted down by eps.  This also folds UTM
    // northings to the correct N/S hemisphere.

    // The smallest length s.t., 1.0e7 - eps() < 1.0e7 (approx 1.9 nm)
    // 25 = ceil(log_2(2e7)) -- use half circumference here because
    // northing 195e5 is a legal in the "southern" hemisphere.
    static const real eps = ldexp(real(1), -(Math::digits() - 25));
    int
      ix = int(floor(x / tile_)),
      iy = int(floor(y / tile_)),
      ind = (utmp ? 2 : 0) + (northp ? 1 : 0);
    if (! (ix >= mineasting_[ind] && ix < maxeasting_[ind]) ) {
      if (ix == maxeasting_[ind] && x == maxeasting_[ind] * tile_)
        x -= eps;
      else
        throw GeographicErr("Easting " + Utility::str(int(floor(x/1000)))
                            + "km not in MGRS/"
                            + (utmp ? "UTM" : "UPS") + " range for "
                            + (northp ? "N" : "S" ) + " hemisphere ["
                            + Utility::str(mineasting_[ind]*tile_/1000)
                            + "km, "
                            + Utility::str(maxeasting_[ind]*tile_/1000)
                            + "km)");
    }
    if (! (iy >= minnorthing_[ind] && iy < maxnorthing_[ind]) ) {
      if (iy == maxnorthing_[ind] && y == maxnorthing_[ind] * tile_)
        y -= eps;
      else
        throw GeographicErr("Northing " + Utility::str(int(floor(y/1000)))
                            + "km not in MGRS/"
                            + (utmp ? "UTM" : "UPS") + " range for "
                            + (northp ? "N" : "S" ) + " hemisphere ["
                            + Utility::str(minnorthing_[ind]*tile_/1000)
                            + "km, "
                            + Utility::str(maxnorthing_[ind]*tile_/1000)
                            + "km)");
    }

    // Correct the UTM northing and hemisphere if necessary
    if (utmp) {
      if (northp && iy < minutmNrow_) {
        northp = false;
        y += utmNshift_;
      } else if (!northp && iy >= maxutmSrow_) {
        if (y == maxutmSrow_ * tile_)
          // If on equator retain S hemisphere
          y -= eps;
        else {
          northp = true;
          y -= utmNshift_;
        }
      }
    }
  }

  int MGRS::UTMRow(int iband, int icol, int irow) {
    // Input is iband = band index in [-10, 10) (as returned by LatitudeBand),
    // icol = column index in [0,8) with origin of easting = 100km, and irow =
    // periodic row index in [0,20) with origin = equator.  Output is true row
    // index in [-90, 95).  Returns maxutmSrow_ = 100, if irow and iband are
    // incompatible.

    // Estimate center row number for latitude band
    // 90 deg = 100 tiles; 1 band = 8 deg = 100*8/90 tiles
    real c = 100 * (8 * iband + 4)/real(90);
    bool northp = iband >= 0;
    // These are safe bounds on the rows
    //  iband minrow maxrow
    //   -10    -90    -81
    //    -9    -80    -72
    //    -8    -71    -63
    //    -7    -63    -54
    //    -6    -54    -45
    //    -5    -45    -36
    //    -4    -36    -27
    //    -3    -27    -18
    //    -2    -18     -9
    //    -1     -9     -1
    //     0      0      8
    //     1      8     17
    //     2     17     26
    //     3     26     35
    //     4     35     44
    //     5     44     53
    //     6     53     62
    //     7     62     70
    //     8     71     79
    //     9     80     94
    int
      minrow = iband > -10 ?
      int(floor(c - real(4.3) - real(0.1) * northp)) : -90,
      maxrow = iband <   9 ?
      int(floor(c + real(4.4) - real(0.1) * northp)) :  94,
      baserow = (minrow + maxrow) / 2 - utmrowperiod_ / 2;
    // Offset irow by the multiple of utmrowperiod_ which brings it as close as
    // possible to the center of the latitude band, (minrow + maxrow) / 2.
    // (Add maxutmSrow_ = 5 * utmrowperiod_ to ensure operand is positive.)
    irow = (irow - baserow + maxutmSrow_) % utmrowperiod_ + baserow;
    if (!( irow >= minrow && irow <= maxrow )) {
      // Outside the safe bounds, so need to check...
      // Northing = 71e5 and 80e5 intersect band boundaries
      //   y = 71e5 in scol = 2 (x = [3e5,4e5] and x = [6e5,7e5])
      //   y = 80e5 in scol = 1 (x = [2e5,3e5] and x = [7e5,8e5])
      // This holds for all the ellipsoids given in NGA.SIG.0012_2.0.0_UTMUPS.
      // The following deals with these special cases.
      int
        // Fold [-10,-1] -> [9,0]
        sband = iband >= 0 ? iband : -iband - 1,
        // Fold [-90,-1] -> [89,0]
        srow = irow >= 0 ? irow : -irow - 1,
        // Fold [4,7] -> [3,0]
        scol = icol < 4 ? icol : -icol + 7;
      // For example, the safe rows for band 8 are 71 - 79.  However row 70 is
      // allowed if scol = [2,3] and row 80 is allowed if scol = [0,1].
      if ( ! ( (srow == 70 && sband == 8 && scol >= 2) ||
               (srow == 71 && sband == 7 && scol <= 2) ||
               (srow == 79 && sband == 9 && scol >= 1) ||
               (srow == 80 && sband == 8 && scol <= 1) ) )
        irow = maxutmSrow_;
    }
    return irow;
  }

  void MGRS::Check() {
    real lat, lon, x, y, t = tile_; int zone; bool northp;
    UTMUPS::Reverse(31, true , 1*t,  0*t, lat, lon);
    if (!( lon <   0 ))
      throw GeographicErr("MGRS::Check: equator coverage failure");
    UTMUPS::Reverse(31, true , 1*t, 95*t, lat, lon);
    if (!( lat >  84 ))
      throw GeographicErr("MGRS::Check: UTM doesn't reach latitude = 84");
    UTMUPS::Reverse(31, false, 1*t, 10*t, lat, lon);
    if (!( lat < -80 ))
      throw GeographicErr("MGRS::Check: UTM doesn't reach latitude = -80");
    UTMUPS::Forward(56,  3, zone, northp, x, y, 32);
    if (!( x > 1*t ))
      throw GeographicErr("MGRS::Check: Norway exception creates a gap");
    UTMUPS::Forward(72, 21, zone, northp, x, y, 35);
    if (!( x > 1*t ))
      throw GeographicErr("MGRS::Check: Svalbard exception creates a gap");
    UTMUPS::Reverse(0, true , 20*t, 13*t, lat, lon);
    if (!( lat <  84 ))
      throw
        GeographicErr("MGRS::Check: North UPS doesn't reach latitude = 84");
    UTMUPS::Reverse(0, false, 20*t,  8*t, lat, lon);
    if (!( lat > -80 ))
      throw
        GeographicErr("MGRS::Check: South UPS doesn't reach latitude = -80");
    // Entries are [band, x, y] either side of the band boundaries.  Units for
    // x, y are t = 100km.
    const short tab[] = {
      0, 5,  0,   0, 9,  0,     // south edge of band 0
      0, 5,  8,   0, 9,  8,     // north edge of band 0
      1, 5,  9,   1, 9,  9,     // south edge of band 1
      1, 5, 17,   1, 9, 17,     // north edge of band 1
      2, 5, 18,   2, 9, 18,     // etc.
      2, 5, 26,   2, 9, 26,
      3, 5, 27,   3, 9, 27,
      3, 5, 35,   3, 9, 35,
      4, 5, 36,   4, 9, 36,
      4, 5, 44,   4, 9, 44,
      5, 5, 45,   5, 9, 45,
      5, 5, 53,   5, 9, 53,
      6, 5, 54,   6, 9, 54,
      6, 5, 62,   6, 9, 62,
      7, 5, 63,   7, 9, 63,
      7, 5, 70,   7, 7, 70,   7, 7, 71,   7, 9, 71, // y = 71t crosses boundary
      8, 5, 71,   8, 6, 71,   8, 6, 72,   8, 9, 72, // between bands 7 and 8.
      8, 5, 79,   8, 8, 79,   8, 8, 80,   8, 9, 80, // y = 80t crosses boundary
      9, 5, 80,   9, 7, 80,   9, 7, 81,   9, 9, 81, // between bands 8 and 9.
      9, 5, 95,   9, 9, 95,     // north edge of band 9
    };
    const int bandchecks = sizeof(tab) / (3 * sizeof(short));
    for (int i = 0; i < bandchecks; ++i) {
      UTMUPS::Reverse(38, true, tab[3*i+1]*t, tab[3*i+2]*t, lat, lon);
      if (!( LatitudeBand(lat) == tab[3*i+0] ))
        throw GeographicErr("MGRS::Check: Band error, b = " +
                            Utility::str(tab[3*i+0]) + ", x = " +
                            Utility::str(tab[3*i+1]) + "00km, y = " +
                            Utility::str(tab[3*i+2]) + "00km");
    }
  }

} // namespace GeographicLib
