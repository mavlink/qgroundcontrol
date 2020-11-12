/**
 * \file PolarStereographic.hpp
 * \brief Header for GeographicLib::PolarStereographic class
 *
 * Copyright (c) Charles Karney (2008-2019) <charles@karney.com> and licensed
 * under the MIT/X11 License.  For more information, see
 * https://geographiclib.sourceforge.io/
 **********************************************************************/

#if !defined(GEOGRAPHICLIB_POLARSTEREOGRAPHIC_HPP)
#define GEOGRAPHICLIB_POLARSTEREOGRAPHIC_HPP 1

#include "Constants.hpp"

namespace GeographicLib {

  /**
   * \brief Polar stereographic projection
   *
   * Implementation taken from the report,
   * - J. P. Snyder,
   *   <a href="http://pubs.er.usgs.gov/usgspubs/pp/pp1395"> Map Projections: A
   *   Working Manual</a>, USGS Professional Paper 1395 (1987),
   *   pp. 160--163.
   *
   * This is a straightforward implementation of the equations in Snyder except
   * that Newton's method is used to invert the projection.
   *
   * This class also returns the meridian convergence \e gamma and scale \e k.
   * The meridian convergence is the bearing of grid north (the \e y axis)
   * measured clockwise from true north.
   *
   * Example of use:
   * \include example-PolarStereographic.cpp
   **********************************************************************/
  class GEOGRAPHICLIB_EXPORT PolarStereographic {
  private:
    typedef Math::real real;
    real _a, _f, _e2, _es, _e2m, _c;
    real _k0;
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
    PolarStereographic(real a, real f, real k0);

    /**
     * Set the scale for the projection.
     *
     * @param[in] lat (degrees) assuming \e northp = true.
     * @param[in] k scale at latitude \e lat (default 1).
     * @exception GeographicErr \e k is not positive.
     * @exception GeographicErr if \e lat is not in (&minus;90&deg;,
     *   90&deg;].
     **********************************************************************/
    void SetScale(real lat, real k = real(1));

    /**
     * Forward projection, from geographic to polar stereographic.
     *
     * @param[in] northp the pole which is the center of projection (true means
     *   north, false means south).
     * @param[in] lat latitude of point (degrees).
     * @param[in] lon longitude of point (degrees).
     * @param[out] x easting of point (meters).
     * @param[out] y northing of point (meters).
     * @param[out] gamma meridian convergence at point (degrees).
     * @param[out] k scale of projection at point.
     *
     * No false easting or northing is added.  \e lat should be in the range
     * (&minus;90&deg;, 90&deg;] for \e northp = true and in the range
     * [&minus;90&deg;, 90&deg;) for \e northp = false.
     **********************************************************************/
    void Forward(bool northp, real lat, real lon,
                 real& x, real& y, real& gamma, real& k) const;

    /**
     * Reverse projection, from polar stereographic to geographic.
     *
     * @param[in] northp the pole which is the center of projection (true means
     *   north, false means south).
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
    void Reverse(bool northp, real x, real y,
                 real& lat, real& lon, real& gamma, real& k) const;

    /**
     * PolarStereographic::Forward without returning the convergence and scale.
     **********************************************************************/
    void Forward(bool northp, real lat, real lon,
                 real& x, real& y) const {
      real gamma, k;
      Forward(northp, lat, lon, x, y, gamma, k);
    }

    /**
     * PolarStereographic::Reverse without returning the convergence and scale.
     **********************************************************************/
    void Reverse(bool northp, real x, real y,
                 real& lat, real& lon) const {
      real gamma, k;
      Reverse(northp, x, y, lat, lon, gamma, k);
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
     * The central scale for the projection.  This is the value of \e k0 used
     * in the constructor and is the scale at the pole unless overridden by
     * PolarStereographic::SetScale.
     **********************************************************************/
    Math::real CentralScale() const { return _k0; }

    /**
      * \deprecated An old name for EquatorialRadius().
      **********************************************************************/
    // GEOGRAPHICLIB_DEPRECATED("Use EquatorialRadius()")
    Math::real MajorRadius() const { return EquatorialRadius(); }
    ///@}

    /**
     * A global instantiation of PolarStereographic with the WGS84 ellipsoid
     * and the UPS scale factor.  However, unlike UPS, no false easting or
     * northing is added.
     **********************************************************************/
    static const PolarStereographic& UPS();
  };

} // namespace GeographicLib

#endif  // GEOGRAPHICLIB_POLARSTEREOGRAPHIC_HPP
