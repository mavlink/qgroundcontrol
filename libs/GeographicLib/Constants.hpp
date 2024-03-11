/**
 * \file Constants.hpp
 * \brief Header for GeographicLib::Constants class
 *
 * Copyright (c) Charles Karney (2008-2019) <charles@karney.com> and licensed
 * under the MIT/X11 License.  For more information, see
 * https://geographiclib.sourceforge.io/
 **********************************************************************/

#pragma once

// This will be overwritten by ./configure

#define GEOGRAPHICLIB_VERSION_STRING "1.50"
#define GEOGRAPHICLIB_VERSION_MAJOR 1
#define GEOGRAPHICLIB_VERSION_MINOR 50
#define GEOGRAPHICLIB_VERSION_PATCH 0

// Undefine HAVE_LONG_DOUBLE if this type is unknown to the compiler
#define GEOGRAPHICLIB_HAVE_LONG_DOUBLE 1

// Define WORDS_BIGENDIAN to be 1 if your machine is big endian
/* #undef WORDS_BIGENDIAN */

#include "Math.hpp"

/**
 * @relates GeographicLib::Constants
 * Pack the version components into a single integer.  Users should not rely on
 * this particular packing of the components of the version number; see the
 * documentation for GEOGRAPHICLIB_VERSION, below.
 **********************************************************************/
#define GEOGRAPHICLIB_VERSION_NUM(a,b,c) ((((a) * 10000 + (b)) * 100) + (c))

/**
 * @relates GeographicLib::Constants
 * The version of GeographicLib as a single integer, packed as MMmmmmpp where
 * MM is the major version, mmmm is the minor version, and pp is the patch
 * level.  Users should not rely on this particular packing of the components
 * of the version number.  Instead they should use a test such as \code
   #if GEOGRAPHICLIB_VERSION >= GEOGRAPHICLIB_VERSION_NUM(1,37,0)
   ...
   #endif
 * \endcode
 **********************************************************************/
#define GEOGRAPHICLIB_VERSION \
 GEOGRAPHICLIB_VERSION_NUM(GEOGRAPHICLIB_VERSION_MAJOR, \
                           GEOGRAPHICLIB_VERSION_MINOR, \
                           GEOGRAPHICLIB_VERSION_PATCH)

/**
 * @relates GeographicLib::Constants
 * Is the C++11 static_assert available?
 **********************************************************************/
#if !defined(GEOGRAPHICLIB_HAS_STATIC_ASSERT)
#  if __cplusplus >= 201103 || defined(__GXX_EXPERIMENTAL_CXX0X__)
#    define GEOGRAPHICLIB_HAS_STATIC_ASSERT 1
#  elif defined(_MSC_VER) && _MSC_VER >= 1600
// For reference, here is a table of Visual Studio and _MSC_VER
// correspondences:
//
// _MSC_VER  Visual Studio
//   1100     vc5
//   1200     vc6
//   1300     vc7
//   1310     vc7.1 (2003)
//   1400     vc8   (2005)
//   1500     vc9   (2008)
//   1600     vc10  (2010)
//   1700     vc11  (2012)
//   1800     vc12  (2013) First version of VS to include enough C++11 support
//   1900     vc14  (2015)
//   191[0-9] vc15  (2017)
//   192[0-9] vc16  (2019)
#    define GEOGRAPHICLIB_HAS_STATIC_ASSERT 1
#  else
#    define GEOGRAPHICLIB_HAS_STATIC_ASSERT 0
#  endif
#endif

/**
 * @relates GeographicLib::Constants
 * A compile-time assert.  Use C++11 static_assert, if available.
 **********************************************************************/
#if !defined(GEOGRAPHICLIB_STATIC_ASSERT)
#  if GEOGRAPHICLIB_HAS_STATIC_ASSERT
#    define GEOGRAPHICLIB_STATIC_ASSERT static_assert
#  else
#    define GEOGRAPHICLIB_STATIC_ASSERT(cond,reason) \
            { enum{ GEOGRAPHICLIB_STATIC_ASSERT_ENUM = 1/int(cond) }; }
#  endif
#endif

#if defined(_MSC_VER) && defined(GEOGRAPHICLIB_SHARED_LIB) && \
  GEOGRAPHICLIB_SHARED_LIB
#  if GEOGRAPHICLIB_SHARED_LIB > 1
#    error GEOGRAPHICLIB_SHARED_LIB must be 0 or 1
#  elif defined(GeographicLib_SHARED_EXPORTS)
#    define GEOGRAPHICLIB_EXPORT __declspec(dllexport)
#  else
#    define GEOGRAPHICLIB_EXPORT __declspec(dllimport)
#  endif
#else
#  define GEOGRAPHICLIB_EXPORT
#endif

// Use GEOGRAPHICLIB_DEPRECATED to mark functions, types or variables as
// deprecated.  Code inspired by Apache Subversion's svn_types.h file (via
// MPFR).
#if defined(__GNUC__)
#  if __GNUC__ > 4
#    define GEOGRAPHICLIB_DEPRECATED(msg) __attribute__((deprecated(msg)))
#  else
#    define GEOGRAPHICLIB_DEPRECATED(msg) __attribute__((deprecated))
#  endif
#elif defined(_MSC_VER) && _MSC_VER >= 1300
#  define GEOGRAPHICLIB_DEPRECATED(msg) __declspec(deprecated(msg))
#else
#  define GEOGRAPHICLIB_DEPRECATED(msg)
#endif

#include <stdexcept>
#include <string>

/**
 * \brief Namespace for %GeographicLib
 *
 * All of %GeographicLib is defined within the GeographicLib namespace.  In
 * addition all the header files are included via %GeographicLib/Class.hpp.
 * This minimizes the likelihood of conflicts with other packages.
 **********************************************************************/
namespace GeographicLib {

  /**
   * \brief %Constants needed by %GeographicLib
   *
   * Define constants specifying the WGS84 ellipsoid, the UTM and UPS
   * projections, and various unit conversions.
   *
   * Example of use:
   * \include example-Constants.cpp
   **********************************************************************/
  class GEOGRAPHICLIB_EXPORT Constants {
  private:
    Constants();                // Disable constructor

  public:
    /**
     * A synonym for Math::degree<double>().
     **********************************************************************/
    static double degree() { return Math::degree(); }
    /**
     * @return the number of radians in an arcminute.
     **********************************************************************/
    static double arcminute()
    { return Math::degree() / 60; }
    /**
     * @return the number of radians in an arcsecond.
     **********************************************************************/
    static double arcsecond()
    { return Math::degree() / 3600; }

    /** \name Ellipsoid parameters
     **********************************************************************/
    ///@{
    /**
     * @tparam T the type of the returned value.
     * @return the equatorial radius of WGS84 ellipsoid (6378137 m).
     **********************************************************************/
    template<typename T> static T WGS84_a()
    { return 6378137 * meter<T>(); }
    /**
     * A synonym for WGS84_a<double>().
     **********************************************************************/
    static double WGS84_a() { return WGS84_a<double>(); }
    /**
     * @tparam T the type of the returned value.
     * @return the flattening of WGS84 ellipsoid (1/298.257223563).
     **********************************************************************/
    template<typename T> static T WGS84_f() {
      // Evaluating this as 1000000000 / T(298257223563LL) reduces the
      // round-off error by about 10%.  However, expressing the flattening as
      // 1/298.257223563 is well ingrained.
      return 1 / ( T(298257223563LL) / 1000000000 );
    }
    /**
     * A synonym for WGS84_f<double>().
     **********************************************************************/
    static double WGS84_f() { return WGS84_f<double>(); }
    /**
     * @tparam T the type of the returned value.
     * @return the gravitational constant of the WGS84 ellipsoid, \e GM, in
     *   m<sup>3</sup> s<sup>&minus;2</sup>.
     **********************************************************************/
    template<typename T> static T WGS84_GM()
    { return T(3986004) * 100000000 + 41800000; }
    /**
     * A synonym for WGS84_GM<double>().
     **********************************************************************/
    static double WGS84_GM() { return WGS84_GM<double>(); }
    /**
     * @tparam T the type of the returned value.
     * @return the angular velocity of the WGS84 ellipsoid, &omega;, in rad
     *   s<sup>&minus;1</sup>.
     **********************************************************************/
    template<typename T> static T WGS84_omega()
    { return 7292115 / (T(1000000) * 100000); }
    /**
     * A synonym for WGS84_omega<double>().
     **********************************************************************/
    static double WGS84_omega() { return WGS84_omega<double>(); }
    /**
     * @tparam T the type of the returned value.
     * @return the equatorial radius of GRS80 ellipsoid, \e a, in m.
     **********************************************************************/
    template<typename T> static T GRS80_a()
    { return 6378137 * meter<T>(); }
    /**
     * A synonym for GRS80_a<double>().
     **********************************************************************/
    static double GRS80_a() { return GRS80_a<double>(); }
    /**
     * @tparam T the type of the returned value.
     * @return the gravitational constant of the GRS80 ellipsoid, \e GM, in
     *   m<sup>3</sup> s<sup>&minus;2</sup>.
     **********************************************************************/
    template<typename T> static T GRS80_GM()
    { return T(3986005) * 100000000; }
    /**
     * A synonym for GRS80_GM<double>().
     **********************************************************************/
    static double GRS80_GM() { return GRS80_GM<double>(); }
    /**
     * @tparam T the type of the returned value.
     * @return the angular velocity of the GRS80 ellipsoid, &omega;, in rad
     *   s<sup>&minus;1</sup>.
     *
     * This is about 2 &pi; 366.25 / (365.25 &times; 24 &times; 3600) rad
     * s<sup>&minus;1</sup>.  365.25 is the number of days in a Julian year and
     * 365.35/366.25 converts from solar days to sidedouble days.  Using the
     * number of days in a Gregorian year (365.2425) results in a worse
     * approximation (because the Gregorian year includes the precession of the
     * earth's axis).
     **********************************************************************/
    template<typename T> static T GRS80_omega()
    { return 7292115 / (T(1000000) * 100000); }
    /**
     * A synonym for GRS80_omega<double>().
     **********************************************************************/
    static double GRS80_omega() { return GRS80_omega<double>(); }
    /**
     * @tparam T the type of the returned value.
     * @return the dynamical form factor of the GRS80 ellipsoid,
     *   <i>J</i><sub>2</sub>.
     **********************************************************************/
    template<typename T> static T GRS80_J2()
    { return T(108263) / 100000000; }
    /**
     * A synonym for GRS80_J2<double>().
     **********************************************************************/
    static double GRS80_J2() { return GRS80_J2<double>(); }
    /**
     * @tparam T the type of the returned value.
     * @return the central scale factor for UTM (0.9996).
     **********************************************************************/
    template<typename T> static T UTM_k0()
    {return T(9996) / 10000; }
    /**
     * A synonym for UTM_k0<double>().
     **********************************************************************/
    static double UTM_k0() { return UTM_k0<double>(); }
    /**
     * @tparam T the type of the returned value.
     * @return the central scale factor for UPS (0.994).
     **********************************************************************/
    template<typename T> static T UPS_k0()
    { return T(994) / 1000; }
    /**
     * A synonym for UPS_k0<double>().
     **********************************************************************/
    static double UPS_k0() { return UPS_k0<double>(); }
    ///@}

    /** \name SI units
     **********************************************************************/
    ///@{
    /**
     * @tparam T the type of the returned value.
     * @return the number of meters in a meter.
     *
     * This is unity, but this lets the internal system of units be changed if
     * necessary.
     **********************************************************************/
    template<typename T> static T meter() { return T(1); }
    /**
     * A synonym for meter<double>().
     **********************************************************************/
    static double meter() { return meter<double>(); }
    /**
     * @return the number of meters in a kilometer.
     **********************************************************************/
    static double kilometer()
    { return 1000 * meter<double>(); }
    /**
     * @return the number of meters in a nautical mile (approximately 1 arc
     *   minute)
     **********************************************************************/
    static double nauticalmile()
    { return 1852 * meter<double>(); }

    /**
     * @tparam T the type of the returned value.
     * @return the number of square meters in a square meter.
     *
     * This is unity, but this lets the internal system of units be changed if
     * necessary.
     **********************************************************************/
    template<typename T> static T square_meter()
    { return meter<double>() * meter<double>(); }
    /**
     * A synonym for square_meter<double>().
     **********************************************************************/
    static double square_meter()
    { return square_meter<double>(); }
    /**
     * @return the number of square meters in a hectare.
     **********************************************************************/
    static double hectare()
    { return 10000 * square_meter<double>(); }
    /**
     * @return the number of square meters in a square kilometer.
     **********************************************************************/
    static double square_kilometer()
    { return kilometer() * kilometer(); }
    /**
     * @return the number of square meters in a square nautical mile.
     **********************************************************************/
    static double square_nauticalmile()
    { return nauticalmile() * nauticalmile(); }
    ///@}

    /** \name Anachronistic British units
     **********************************************************************/
    ///@{
    /**
     * @return the number of meters in an international foot.
     **********************************************************************/
    static double foot()
    { return double(254 * 12) / 10000 * meter<double>(); }
    /**
     * @return the number of meters in a yard.
     **********************************************************************/
    static double yard() { return 3 * foot(); }
    /**
     * @return the number of meters in a fathom.
     **********************************************************************/
    static double fathom() { return 2 * yard(); }
    /**
     * @return the number of meters in a chain.
     **********************************************************************/
    static double chain() { return 22 * yard(); }
    /**
     * @return the number of meters in a furlong.
     **********************************************************************/
    static double furlong() { return 10 * chain(); }
    /**
     * @return the number of meters in a statute mile.
     **********************************************************************/
    static double mile() { return 8 * furlong(); }
    /**
     * @return the number of square meters in an acre.
     **********************************************************************/
    static double acre() { return chain() * furlong(); }
    /**
     * @return the number of square meters in a square statute mile.
     **********************************************************************/
    static double square_mile() { return mile() * mile(); }
    ///@}

    /** \name Anachronistic US units
     **********************************************************************/
    ///@{
    /**
     * @return the number of meters in a US survey foot.
     **********************************************************************/
    static double surveyfoot()
    { return double(1200) / 3937 * meter<double>(); }
    ///@}
  };

  /**
   * \brief Exception handling for %GeographicLib
   *
   * A class to handle exceptions.  It's derived from std::runtime_error so it
   * can be caught by the usual catch clauses.
   *
   * Example of use:
   * \include example-GeographicErr.cpp
   **********************************************************************/
  class GeographicErr : public std::runtime_error {
  public:

    /**
     * Constructor
     *
     * @param[in] msg a string message, which is accessible in the catch
     *   clause via what().
     **********************************************************************/
    GeographicErr(const std::string& msg) : std::runtime_error(msg) {}
  };

} // namespace GeographicLib
