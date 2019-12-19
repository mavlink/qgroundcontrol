/**
 * \file TransverseMercator.hpp
 * \brief Header for GeographicLib::TransverseMercator class
 *
 * Copyright (c) Charles Karney (2008-2019) <charles@karney.com> and licensed
 * under the MIT/X11 License.  For more information, see
 * https://geographiclib.sourceforge.io/
 **********************************************************************/

#if !defined(GEOGRAPHICLIB_TRANSVERSEMERCATOR_HPP)
#define GEOGRAPHICLIB_TRANSVERSEMERCATOR_HPP 1

#include "Constants.hpp"

#if !defined(GEOGRAPHICLIB_TRANSVERSEMERCATOR_ORDER)
/**
 * The order of the series approximation used in TransverseMercator.
 * GEOGRAPHICLIB_TRANSVERSEMERCATOR_ORDER can be set to any integer in [4, 8].
 **********************************************************************/
#  define GEOGRAPHICLIB_TRANSVERSEMERCATOR_ORDER \
  (GEOGRAPHICLIB_PRECISION == 2 ? 6 : \
   (GEOGRAPHICLIB_PRECISION == 1 ? 4 : 8))
#endif

namespace GeographicLib {

  /**
   * \brief Transverse Mercator projection
   *
   * This uses Kr&uuml;ger's method which evaluates the projection and its
   * inverse in terms of a series.  See
   *  - L. Kr&uuml;ger,
   *    <a href="https://doi.org/10.2312/GFZ.b103-krueger28"> Konforme
   *    Abbildung des Erdellipsoids in der Ebene</a> (Conformal mapping of the
   *    ellipsoidal earth to the plane), Royal Prussian Geodetic Institute, New
   *    Series 52, 172 pp. (1912).
   *  - C. F. F. Karney,
   *    <a href="https://doi.org/10.1007/s00190-011-0445-3">
   *    Transverse Mercator with an accuracy of a few nanometers,</a>
   *    J. Geodesy 85(8), 475--485 (Aug. 2011);
   *    preprint
   *    <a href="https://arxiv.org/abs/1002.1417">arXiv:1002.1417</a>.
   *
   * Kr&uuml;ger's method has been extended from 4th to 6th order.  The maximum
   * error is 5 nm (5 nanometers), ground distance, for all positions within 35
   * degrees of the central meridian.  The error in the convergence is 2
   * &times; 10<sup>&minus;15</sup>&quot; and the relative error in the scale
   * is 6 &times; 10<sup>&minus;12</sup>%%.  See Sec. 4 of
   * <a href="https://arxiv.org/abs/1002.1417">arXiv:1002.1417</a> for details.
   * The speed penalty in going to 6th order is only about 1%.
   *
   * There's a singularity in the projection at &phi; = 0&deg;, &lambda;
   * &minus; &lambda;<sub>0</sub> = &plusmn;(1 &minus; \e e)90&deg; (&asymp;
   * &plusmn;82.6&deg; for the WGS84 ellipsoid), where \e e is the
   * eccentricity.  Beyond this point, the series ceases to converge and the
   * results from this method will be garbage.  To be on the safe side, don't
   * use this method if the angular distance from the central meridian exceeds
   * (1 &minus; 2e)90&deg; (&asymp; 75&deg; for the WGS84 ellipsoid)
   *
   * TransverseMercatorExact is an alternative implementation of the projection
   * using exact formulas which yield accurate (to 8 nm) results over the
   * entire ellipsoid.
   *
   * The ellipsoid parameters and the central scale are set in the constructor.
   * The central meridian (which is a trivial shift of the longitude) is
   * specified as the \e lon0 argument of the TransverseMercator::Forward and
   * TransverseMercator::Reverse functions.  The latitude of origin is taken to
   * be the equator.  There is no provision in this class for specifying a
   * false easting or false northing or a different latitude of origin.
   * However these are can be simply included by the calling function.  For
   * example, the UTMUPS class applies the false easting and false northing for
   * the UTM projections.  A more complicated example is the British National
   * Grid (<a href="https://www.spatialreference.org/ref/epsg/7405/">
   * EPSG:7405</a>) which requires the use of a latitude of origin.  This is
   * implemented by the GeographicLib::OSGB class.
   *
   * This class also returns the meridian convergence \e gamma and scale \e k.
   * The meridian convergence is the bearing of grid north (the \e y axis)
   * measured clockwise from true north.
   *
   * See TransverseMercator.cpp for more information on the implementation.
   *
   * See \ref transversemercator for a discussion of this projection.
   *
   * Example of use:
   * \include example-TransverseMercator.cpp
   *
   * <a href="TransverseMercatorProj.1.html">TransverseMercatorProj</a> is a
   * command-line utility providing access to the functionality of
   * TransverseMercator and TransverseMercatorExact.
   **********************************************************************/

  class GEOGRAPHICLIB_EXPORT TransverseMercator {
  private:
    typedef Math::real real;
    static const int maxpow_ = GEOGRAPHICLIB_TRANSVERSEMERCATOR_ORDER;
    static const int numit_ = 5;
    real _a, _f, _k0, _e2, _es, _e2m,  _c, _n;
    // _alp[0] and _bet[0] unused
    real _a1, _b1, _alp[maxpow_ + 1], _bet[maxpow_ + 1];
    friend class Ellipsoid;           // For access to taupf, tauf.
  public:

    /**
     * Constructor for a ellipsoid with
     *
     * @param[in] a equatorial radius (meters).
     * @param[in] f flattening of ellipsoid.  Setting \e f = 0 gives a sphere.
     *   Negative \e f gives a prolate ellipsoid.
     * @param[in] k0 central scale factor.
     * @exception GeographicErr if \e a, (1 &minus; \e f) \e a, or \e k0 is
     *   not positive.
     **********************************************************************/
    TransverseMercator(real a, real f, real k0);

    /**
     * Forward projection, from geographic to transverse Mercator.
     *
     * @param[in] lon0 central meridian of the projection (degrees).
     * @param[in] lat latitude of point (degrees).
     * @param[in] lon longitude of point (degrees).
     * @param[out] x easting of point (meters).
     * @param[out] y northing of point (meters).
     * @param[out] gamma meridian convergence at point (degrees).
     * @param[out] k scale of projection at point.
     *
     * No false easting or northing is added. \e lat should be in the range
     * [&minus;90&deg;, 90&deg;].
     **********************************************************************/
    void Forward(real lon0, real lat, real lon,
                 real& x, real& y, real& gamma, real& k) const;

    /**
     * Reverse projection, from transverse Mercator to geographic.
     *
     * @param[in] lon0 central meridian of the projection (degrees).
     * @param[in] x easting of point (meters).
     * @param[in] y northing of point (meters).
     * @param[out] lat latitude of point (degrees).
     * @param[out] lon longitude of point (degrees).
     * @param[out] gamma meridian convergence at point (degrees).
     * @param[out] k scale of projection at point.
     *
     * No false easting or northing is added.  The value of \e lon returned is
     * in the range [&minus;180&deg;, 180&deg;].
     **********************************************************************/
    void Reverse(real lon0, real x, real y,
                 real& lat, real& lon, real& gamma, real& k) const;

    /**
     * TransverseMercator::Forward without returning the convergence and scale.
     **********************************************************************/
    void Forward(real lon0, real lat, real lon,
                 real& x, real& y) const {
      real gamma, k;
      Forward(lon0, lat, lon, x, y, gamma, k);
    }

    /**
     * TransverseMercator::Reverse without returning the convergence and scale.
     **********************************************************************/
    void Reverse(real lon0, real x, real y,
                 real& lat, real& lon) const {
      real gamma, k;
      Reverse(lon0, x, y, lat, lon, gamma, k);
    }

    /** \name Inspector functions
     **********************************************************************/
    ///@{
    /**
     * @return \e a the equatorial radius of the ellipsoid (meters).  This is
     *   the value used in the constructor.
     **********************************************************************/
    Math::real EquatorialRadius() const { return _a; }

    /**
     * @return \e f the flattening of the ellipsoid.  This is the value used in
     *   the constructor.
     **********************************************************************/
    Math::real Flattening() const { return _f; }

    /**
     * @return \e k0 central scale for the projection.  This is the value of \e
     *   k0 used in the constructor and is the scale on the central meridian.
     **********************************************************************/
    Math::real CentralScale() const { return _k0; }

    /**
      * \deprecated An old name for EquatorialRadius().
      **********************************************************************/
    // GEOGRAPHICLIB_DEPRECATED("Use EquatorialRadius()")
    Math::real MajorRadius() const { return EquatorialRadius(); }
    ///@}

    /**
     * A global instantiation of TransverseMercator with the WGS84 ellipsoid
     * and the UTM scale factor.  However, unlike UTM, no false easting or
     * northing is added.
     **********************************************************************/
    static const TransverseMercator& UTM();
  };

} // namespace GeographicLib

#endif  // GEOGRAPHICLIB_TRANSVERSEMERCATOR_HPP
