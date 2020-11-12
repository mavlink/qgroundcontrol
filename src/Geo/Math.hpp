/**
 * \file Math.hpp
 * \brief Header for GeographicLib::Math class
 *
 * Copyright (c) Charles Karney (2008-2019) <charles@karney.com> and licensed
 * under the MIT/X11 License.  For more information, see
 * https://geographiclib.sourceforge.io/
 **********************************************************************/

// Constants.hpp includes Math.hpp.  Place this include outside Math.hpp's
// include guard to enforce this ordering.
#include "Constants.hpp"

#if !defined(GEOGRAPHICLIB_MATH_HPP)
#define GEOGRAPHICLIB_MATH_HPP 1

/**
 * Are C++11 math functions available?
 **********************************************************************/
#if !defined(GEOGRAPHICLIB_CXX11_MATH)
// Recent versions of g++ -std=c++11 (4.7 and later?) set __cplusplus to 201103
// and support the new C++11 mathematical functions, std::atanh, etc.  However
// the Android toolchain, which uses g++ -std=c++11 (4.8 as of 2014-03-11,
// according to Pullan Lu), does not support std::atanh.  Android toolchains
// might define __ANDROID__ or ANDROID; so need to check both.  With OSX the
// version is GNUC version 4.2 and __cplusplus is set to 201103, so remove the
// version check on GNUC.
#  if defined(__GNUC__) && __cplusplus >= 201103 && \
  !(defined(__ANDROID__) || defined(ANDROID) || defined(__CYGWIN__))
#    define GEOGRAPHICLIB_CXX11_MATH 1
// Visual C++ 12 supports these functions
#  elif defined(_MSC_VER) && _MSC_VER >= 1800
#    define GEOGRAPHICLIB_CXX11_MATH 1
#  else
#    define GEOGRAPHICLIB_CXX11_MATH 0
#  endif
#endif

#if !defined(GEOGRAPHICLIB_WORDS_BIGENDIAN)
#  define GEOGRAPHICLIB_WORDS_BIGENDIAN 0
#endif

#if !defined(GEOGRAPHICLIB_HAVE_LONG_DOUBLE)
#  define GEOGRAPHICLIB_HAVE_LONG_DOUBLE 0
#endif

#if !defined(GEOGRAPHICLIB_PRECISION)
/**
 * The precision of floating point numbers used in %GeographicLib.  1 means
 * float (single precision); 2 (the default) means double; 3 means long double;
 * 4 is reserved for quadruple precision.  Nearly all the testing has been
 * carried out with doubles and that's the recommended configuration.  In order
 * for long double to be used, GEOGRAPHICLIB_HAVE_LONG_DOUBLE needs to be
 * defined.  Note that with Microsoft Visual Studio, long double is the same as
 * double.
 **********************************************************************/
#  define GEOGRAPHICLIB_PRECISION 2
#endif

#include <cmath>
#include <algorithm>
#include <limits>

#if GEOGRAPHICLIB_PRECISION == 4
#include <boost/version.hpp>
#include <boost/multiprecision/float128.hpp>
#include <boost/math/special_functions.hpp>
#elif GEOGRAPHICLIB_PRECISION == 5
#include <mpreal.h>
#endif

#if GEOGRAPHICLIB_PRECISION > 3
// volatile keyword makes no sense for multiprec types
#define GEOGRAPHICLIB_VOLATILE
// Signal a convergence failure with multiprec types by throwing an exception
// at loop exit.
#define GEOGRAPHICLIB_PANIC \
  (throw GeographicLib::GeographicErr("Convergence failure"), false)
#else
#define GEOGRAPHICLIB_VOLATILE volatile
// Ignore convergence failures with standard floating points types by allowing
// loop to exit cleanly.
#define GEOGRAPHICLIB_PANIC false
#endif

namespace GeographicLib {

  /**
   * \brief Mathematical functions needed by %GeographicLib
   *
   * Define mathematical functions in order to localize system dependencies and
   * to provide generic versions of the functions.  In addition define a real
   * type to be used by %GeographicLib.
   *
   * Example of use:
   * \include example-Math.cpp
   **********************************************************************/
  class Math {
  private:
    void dummy();               // Static check for GEOGRAPHICLIB_PRECISION
    Math();                     // Disable constructor
  public:

#if GEOGRAPHICLIB_HAVE_LONG_DOUBLE
    /**
     * The extended precision type for real numbers, used for some testing.
     * This is long double on computers with this type; otherwise it is double.
     **********************************************************************/
    typedef long double extended;
#else
    typedef double extended;
#endif

#if GEOGRAPHICLIB_PRECISION == 2
    /**
     * The real type for %GeographicLib. Nearly all the testing has been done
     * with \e real = double.  However, the algorithms should also work with
     * float and long double (where available).  (<b>CAUTION</b>: reasonable
     * accuracy typically cannot be obtained using floats.)
     **********************************************************************/
    typedef double real;
#elif GEOGRAPHICLIB_PRECISION == 1
    typedef float real;
#elif GEOGRAPHICLIB_PRECISION == 3
    typedef extended real;
#elif GEOGRAPHICLIB_PRECISION == 4
    typedef boost::multiprecision::float128 real;
#elif GEOGRAPHICLIB_PRECISION == 5
    typedef mpfr::mpreal real;
#else
    typedef double real;
#endif

    /**
     * @return the number of bits of precision in a real number.
     **********************************************************************/
    static int digits();

    /**
     * Set the binary precision of a real number.
     *
     * @param[in] ndigits the number of bits of precision.
     * @return the resulting number of bits of precision.
     *
     * This only has an effect when GEOGRAPHICLIB_PRECISION = 5.  See also
     * Utility::set_digits for caveats about when this routine should be
     * called.
     **********************************************************************/
    static int set_digits(int ndigits);

    /**
     * @return the number of decimal digits of precision in a real number.
     **********************************************************************/
    static int digits10();

    /**
     * Number of additional decimal digits of precision for real relative to
     * double (0 for float).
     **********************************************************************/
    static int extra_digits();

    /**
     * true if the machine is big-endian.
     **********************************************************************/
    static const bool bigendian = GEOGRAPHICLIB_WORDS_BIGENDIAN;

    /**
     * @tparam T the type of the returned value.
     * @return &pi;.
     **********************************************************************/
    template<typename T> static T pi() {
      using std::atan2;
      static const T pi = atan2(T(0), T(-1));
      return pi;
    }
    /**
     * A synonym for pi<real>().
     **********************************************************************/
    static real pi() { return pi<real>(); }

    /**
     * @tparam T the type of the returned value.
     * @return the number of radians in a degree.
     **********************************************************************/
    template<typename T> static T degree() {
      static const T degree = pi<T>() / 180;
      return degree;
    }
    /**
     * A synonym for degree<real>().
     **********************************************************************/
    static real degree() { return degree<real>(); }

    /**
     * Square a number.
     *
     * @tparam T the type of the argument and the returned value.
     * @param[in] x
     * @return <i>x</i><sup>2</sup>.
     **********************************************************************/
    template<typename T> static T sq(T x)
    { return x * x; }

    /**
     * The hypotenuse function avoiding underflow and overflow.
     *
     * @tparam T the type of the arguments and the returned value.
     * @param[in] x
     * @param[in] y
     * @return sqrt(<i>x</i><sup>2</sup> + <i>y</i><sup>2</sup>).
     **********************************************************************/
    template<typename T> static T hypot(T x, T y);

    /**
     * exp(\e x) &minus; 1 accurate near \e x = 0.
     *
     * @tparam T the type of the argument and the returned value.
     * @param[in] x
     * @return exp(\e x) &minus; 1.
     **********************************************************************/
    template<typename T> static T expm1(T x);

    /**
     * log(1 + \e x) accurate near \e x = 0.
     *
     * @tparam T the type of the argument and the returned value.
     * @param[in] x
     * @return log(1 + \e x).
     **********************************************************************/
    template<typename T> static T log1p(T x);

    /**
     * The inverse hyperbolic sine function.
     *
     * @tparam T the type of the argument and the returned value.
     * @param[in] x
     * @return asinh(\e x).
     **********************************************************************/
    template<typename T> static T asinh(T x);

    /**
     * The inverse hyperbolic tangent function.
     *
     * @tparam T the type of the argument and the returned value.
     * @param[in] x
     * @return atanh(\e x).
     **********************************************************************/
    template<typename T> static T atanh(T x);

    /**
     * Copy the sign.
     *
     * @tparam T the type of the argument.
     * @param[in] x gives the magitude of the result.
     * @param[in] y gives the sign of the result.
     * @return value with the magnitude of \e x and with the sign of \e y.
     *
     * This routine correctly handles the case \e y = &minus;0, returning
     * &minus|<i>x</i>|.
     **********************************************************************/
    template<typename T> static T copysign(T x, T y);

    /**
     * The cube root function.
     *
     * @tparam T the type of the argument and the returned value.
     * @param[in] x
     * @return the real cube root of \e x.
     **********************************************************************/
    template<typename T> static T cbrt(T x);

    /**
     * The remainder function.
     *
     * @tparam T the type of the arguments and the returned value.
     * @param[in] x
     * @param[in] y
     * @return the remainder of \e x/\e y in the range [&minus;\e y/2, \e y/2].
     **********************************************************************/
    template<typename T> static T remainder(T x, T y);

    /**
     * The remquo function.
     *
     * @tparam T the type of the arguments and the returned value.
     * @param[in] x
     * @param[in] y
     * @param[out] n the low 3 bits of the quotient
     * @return the remainder of \e x/\e y in the range [&minus;\e y/2, \e y/2].
     **********************************************************************/
    template<typename T> static T remquo(T x, T y, int* n);

    /**
     * The round function.
     *
     * @tparam T the type of the argument and the returned value.
     * @param[in] x
     * @return \e x round to the nearest integer (ties round away from 0).
     **********************************************************************/
    template<typename T> static T round(T x);

    /**
     * The lround function.
     *
     * @tparam T the type of the argument.
     * @param[in] x
     * @return \e x round to the nearest integer as a long int (ties round away
     *   from 0).
     *
     * If the result does not fit in a long int, the return value is undefined.
     **********************************************************************/
    template<typename T> static long lround(T x);

    /**
     * Fused multiply and add.
     *
     * @tparam T the type of the arguments and the returned value.
     * @param[in] x
     * @param[in] y
     * @param[in] z
     * @return <i>xy</i> + <i>z</i>, correctly rounded (on those platforms with
     *   support for the <code>fma</code> instruction).
     *
     * On platforms without the <code>fma</code> instruction, no attempt is
     * made to improve on the result of a rounded multiplication followed by a
     * rounded addition.
     **********************************************************************/
    template<typename T> static T fma(T x, T y, T z);

    /**
     * Normalize a two-vector.
     *
     * @tparam T the type of the argument and the returned value.
     * @param[in,out] x on output set to <i>x</i>/hypot(<i>x</i>, <i>y</i>).
     * @param[in,out] y on output set to <i>y</i>/hypot(<i>x</i>, <i>y</i>).
     **********************************************************************/
    template<typename T> static void norm(T& x, T& y)
    { T h = hypot(x, y); x /= h; y /= h; }

    /**
     * The error-free sum of two numbers.
     *
     * @tparam T the type of the argument and the returned value.
     * @param[in] u
     * @param[in] v
     * @param[out] t the exact error given by (\e u + \e v) - \e s.
     * @return \e s = round(\e u + \e v).
     *
     * See D. E. Knuth, TAOCP, Vol 2, 4.2.2, Theorem B.  (Note that \e t can be
     * the same as one of the first two arguments.)
     **********************************************************************/
    template<typename T> static T sum(T u, T v, T& t);

    /**
     * Evaluate a polynomial.
     *
     * @tparam T the type of the arguments and returned value.
     * @param[in] N the order of the polynomial.
     * @param[in] p the coefficient array (of size \e N + 1).
     * @param[in] x the variable.
     * @return the value of the polynomial.
     *
     * Evaluate <i>y</i> = &sum;<sub><i>n</i>=0..<i>N</i></sub>
     * <i>p</i><sub><i>n</i></sub> <i>x</i><sup><i>N</i>&minus;<i>n</i></sup>.
     * Return 0 if \e N &lt; 0.  Return <i>p</i><sub>0</sub>, if \e N = 0 (even
     * if \e x is infinite or a nan).  The evaluation uses Horner's method.
     **********************************************************************/
    template<typename T> static T polyval(int N, const T p[], T x)
    // This used to employ Math::fma; but that's too slow and it seemed not to
    // improve the accuracy noticeably.  This might change when there's direct
    // hardware support for fma.
    { T y = N < 0 ? 0 : *p++; while (--N >= 0) y = y * x + *p++; return y; }

    /**
     * Normalize an angle.
     *
     * @tparam T the type of the argument and returned value.
     * @param[in] x the angle in degrees.
     * @return the angle reduced to the range (&minus;180&deg;, 180&deg;].
     *
     * The range of \e x is unrestricted.
     **********************************************************************/
    template<typename T> static T AngNormalize(T x) {
      x = remainder(x, T(360)); return x != -180 ? x : 180;
    }

    /**
     * Normalize a latitude.
     *
     * @tparam T the type of the argument and returned value.
     * @param[in] x the angle in degrees.
     * @return x if it is in the range [&minus;90&deg;, 90&deg;], otherwise
     *   return NaN.
     **********************************************************************/
    template<typename T> static T LatFix(T x)
    { using std::abs; return abs(x) > 90 ? NaN<T>() : x; }

    /**
     * The exact difference of two angles reduced to
     * (&minus;180&deg;, 180&deg;].
     *
     * @tparam T the type of the arguments and returned value.
     * @param[in] x the first angle in degrees.
     * @param[in] y the second angle in degrees.
     * @param[out] e the error term in degrees.
     * @return \e d, the truncated value of \e y &minus; \e x.
     *
     * This computes \e z = \e y &minus; \e x exactly, reduced to
     * (&minus;180&deg;, 180&deg;]; and then sets \e z = \e d + \e e where \e d
     * is the nearest representable number to \e z and \e e is the truncation
     * error.  If \e d = &minus;180, then \e e &gt; 0; If \e d = 180, then \e e
     * &le; 0.
     **********************************************************************/
    template<typename T> static T AngDiff(T x, T y, T& e) {
      T t, d = AngNormalize(sum(remainder(-x, T(360)),
                                remainder( y, T(360)), t));
      // Here y - x = d + t (mod 360), exactly, where d is in (-180,180] and
      // abs(t) <= eps (eps = 2^-45 for doubles).  The only case where the
      // addition of t takes the result outside the range (-180,180] is d = 180
      // and t > 0.  The case, d = -180 + eps, t = -eps, can't happen, since
      // sum would have returned the exact result in such a case (i.e., given t
      // = 0).
      return sum(d == 180 && t > 0 ? -180 : d, t, e);
    }

    /**
     * Difference of two angles reduced to [&minus;180&deg;, 180&deg;]
     *
     * @tparam T the type of the arguments and returned value.
     * @param[in] x the first angle in degrees.
     * @param[in] y the second angle in degrees.
     * @return \e y &minus; \e x, reduced to the range [&minus;180&deg;,
     *   180&deg;].
     *
     * The result is equivalent to computing the difference exactly, reducing
     * it to (&minus;180&deg;, 180&deg;] and rounding the result.  Note that
     * this prescription allows &minus;180&deg; to be returned (e.g., if \e x
     * is tiny and negative and \e y = 180&deg;).
     **********************************************************************/
    template<typename T> static T AngDiff(T x, T y)
    { T e; return AngDiff(x, y, e); }

    /**
     * Coarsen a value close to zero.
     *
     * @tparam T the type of the argument and returned value.
     * @param[in] x
     * @return the coarsened value.
     *
     * The makes the smallest gap in \e x = 1/16 &minus; nextafter(1/16, 0) =
     * 1/2<sup>57</sup> for reals = 0.7 pm on the earth if \e x is an angle in
     * degrees.  (This is about 1000 times more resolution than we get with
     * angles around 90&deg;.)  We use this to avoid having to deal with near
     * singular cases when \e x is non-zero but tiny (e.g.,
     * 10<sup>&minus;200</sup>).  This converts &minus;0 to +0; however tiny
     * negative numbers get converted to &minus;0.
     **********************************************************************/
    template<typename T> static T AngRound(T x);

    /**
     * Evaluate the sine and cosine function with the argument in degrees
     *
     * @tparam T the type of the arguments.
     * @param[in] x in degrees.
     * @param[out] sinx sin(<i>x</i>).
     * @param[out] cosx cos(<i>x</i>).
     *
     * The results obey exactly the elementary properties of the trigonometric
     * functions, e.g., sin 9&deg; = cos 81&deg; = &minus; sin 123456789&deg;.
     * If x = &minus;0, then \e sinx = &minus;0; this is the only case where
     * &minus;0 is returned.
     **********************************************************************/
    template<typename T> static void sincosd(T x, T& sinx, T& cosx);

    /**
     * Evaluate the sine function with the argument in degrees
     *
     * @tparam T the type of the argument and the returned value.
     * @param[in] x in degrees.
     * @return sin(<i>x</i>).
     **********************************************************************/
    template<typename T> static T sind(T x);

    /**
     * Evaluate the cosine function with the argument in degrees
     *
     * @tparam T the type of the argument and the returned value.
     * @param[in] x in degrees.
     * @return cos(<i>x</i>).
     **********************************************************************/
    template<typename T> static T cosd(T x);

    /**
     * Evaluate the tangent function with the argument in degrees
     *
     * @tparam T the type of the argument and the returned value.
     * @param[in] x in degrees.
     * @return tan(<i>x</i>).
     *
     * If \e x = &plusmn;90&deg;, then a suitably large (but finite) value is
     * returned.
     **********************************************************************/
    template<typename T> static T tand(T x);

    /**
     * Evaluate the atan2 function with the result in degrees
     *
     * @tparam T the type of the arguments and the returned value.
     * @param[in] y
     * @param[in] x
     * @return atan2(<i>y</i>, <i>x</i>) in degrees.
     *
     * The result is in the range (&minus;180&deg; 180&deg;].  N.B.,
     * atan2d(&plusmn;0, &minus;1) = +180&deg;; atan2d(&minus;&epsilon;,
     * &minus;1) = &minus;180&deg;, for &epsilon; positive and tiny;
     * atan2d(&plusmn;0, +1) = &plusmn;0&deg;.
     **********************************************************************/
    template<typename T> static T atan2d(T y, T x);

    /**
     * Evaluate the atan function with the result in degrees
     *
     * @tparam T the type of the argument and the returned value.
     * @param[in] x
     * @return atan(<i>x</i>) in degrees.
     **********************************************************************/
   template<typename T> static T atand(T x);

    /**
     * Evaluate <i>e</i> atanh(<i>e x</i>)
     *
     * @tparam T the type of the argument and the returned value.
     * @param[in] x
     * @param[in] es the signed eccentricity =  sign(<i>e</i><sup>2</sup>)
     *    sqrt(|<i>e</i><sup>2</sup>|)
     * @return <i>e</i> atanh(<i>e x</i>)
     *
     * If <i>e</i><sup>2</sup> is negative (<i>e</i> is imaginary), the
     * expression is evaluated in terms of atan.
     **********************************************************************/
    template<typename T> static T eatanhe(T x, T es);

    /**
     * tan&chi; in terms of tan&phi;
     *
     * @tparam T the type of the argument and the returned value.
     * @param[in] tau &tau; = tan&phi;
     * @param[in] es the signed eccentricity = sign(<i>e</i><sup>2</sup>)
     *   sqrt(|<i>e</i><sup>2</sup>|)
     * @return &tau;&prime; = tan&chi;
     *
     * See Eqs. (7--9) of
     * C. F. F. Karney,
     * <a href="https://doi.org/10.1007/s00190-011-0445-3">
     * Transverse Mercator with an accuracy of a few nanometers,</a>
     * J. Geodesy 85(8), 475--485 (Aug. 2011)
     * (preprint
     * <a href="https://arxiv.org/abs/1002.1417">arXiv:1002.1417</a>).
     **********************************************************************/
    template<typename T> static T taupf(T tau, T es);

    /**
     * tan&phi; in terms of tan&chi;
     *
     * @tparam T the type of the argument and the returned value.
     * @param[in] taup &tau;&prime; = tan&chi;
     * @param[in] es the signed eccentricity = sign(<i>e</i><sup>2</sup>)
     *   sqrt(|<i>e</i><sup>2</sup>|)
     * @return &tau; = tan&phi;
     *
     * See Eqs. (19--21) of
     * C. F. F. Karney,
     * <a href="https://doi.org/10.1007/s00190-011-0445-3">
     * Transverse Mercator with an accuracy of a few nanometers,</a>
     * J. Geodesy 85(8), 475--485 (Aug. 2011)
     * (preprint
     * <a href="https://arxiv.org/abs/1002.1417">arXiv:1002.1417</a>).
     **********************************************************************/
    template<typename T> static T tauf(T taup, T es);

    /**
     * Test for finiteness.
     *
     * @tparam T the type of the argument.
     * @param[in] x
     * @return true if number is finite, false if NaN or infinite.
     **********************************************************************/
    template<typename T> static bool isfinite(T x);

    /**
     * The NaN (not a number)
     *
     * @tparam T the type of the returned value.
     * @return NaN if available, otherwise return the max real of type T.
     **********************************************************************/
    template<typename T> static T NaN();

    /**
     * A synonym for NaN<real>().
     **********************************************************************/
    static real NaN() { return NaN<real>(); }

    /**
     * Test for NaN.
     *
     * @tparam T the type of the argument.
     * @param[in] x
     * @return true if argument is a NaN.
     **********************************************************************/
    template<typename T> static bool isnan(T x);

    /**
     * Infinity
     *
     * @tparam T the type of the returned value.
     * @return infinity if available, otherwise return the max real.
     **********************************************************************/
    template<typename T> static T infinity();

    /**
     * A synonym for infinity<real>().
     **********************************************************************/
    static real infinity() { return infinity<real>(); }

    /**
     * Swap the bytes of a quantity
     *
     * @tparam T the type of the argument and the returned value.
     * @param[in] x
     * @return x with its bytes swapped.
     **********************************************************************/
    template<typename T> static T swab(T x) {
      union {
        T r;
        unsigned char c[sizeof(T)];
      } b;
      b.r = x;
      for (int i = sizeof(T)/2; i--; )
        std::swap(b.c[i], b.c[sizeof(T) - 1 - i]);
      return b.r;
    }

  };

} // namespace GeographicLib

#endif  // GEOGRAPHICLIB_MATH_HPP
