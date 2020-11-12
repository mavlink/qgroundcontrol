/**
 * \file Math.cpp
 * \brief Implementation for GeographicLib::Math class
 *
 * Copyright (c) Charles Karney (2015-2019) <charles@karney.com> and licensed
 * under the MIT/X11 License.  For more information, see
 * https://geographiclib.sourceforge.io/
 **********************************************************************/

#include "Math.hpp"

#if defined(_MSC_VER)
// Squelch warnings about constant conditional expressions
#  pragma warning (disable: 4127)
#endif

namespace GeographicLib {

  using namespace std;

  void Math::dummy() {
    GEOGRAPHICLIB_STATIC_ASSERT(GEOGRAPHICLIB_PRECISION >= 1 &&
                                GEOGRAPHICLIB_PRECISION <= 5,
                                "Bad value of precision");
  }

  int Math::digits() {
#if GEOGRAPHICLIB_PRECISION != 5
    return std::numeric_limits<real>::digits;
#else
    return std::numeric_limits<real>::digits();
#endif
  }

  int Math::set_digits(int ndigits) {
#if GEOGRAPHICLIB_PRECISION != 5
    (void)ndigits;
#else
    mpfr::mpreal::set_default_prec(ndigits >= 2 ? ndigits : 2);
#endif
    return digits();
  }

  int Math::digits10() {
#if GEOGRAPHICLIB_PRECISION != 5
    return std::numeric_limits<real>::digits10;
#else
    return std::numeric_limits<real>::digits10();
#endif
  }

  int Math::extra_digits() {
    return
      digits10() > std::numeric_limits<double>::digits10 ?
      digits10() - std::numeric_limits<double>::digits10 : 0;
  }

  template<typename T> T Math::hypot(T x, T y) {
#if GEOGRAPHICLIB_CXX11_MATH
    using std::hypot; return hypot(x, y);
#else
    x = abs(x); y = abs(y);
    if (x < y) std::swap(x, y); // Now x >= y >= 0
    y /= (x != 0 ? x : 1);
    return x * sqrt(1 + y * y);
    // For an alternative (square-root free) method see
    // C. Moler and D. Morrision (1983) https://doi.org/10.1147/rd.276.0577
    // and A. A. Dubrulle (1983) https://doi.org/10.1147/rd.276.0582
#endif
  }

  template<typename T> T Math::expm1(T x) {
#if GEOGRAPHICLIB_CXX11_MATH
    using std::expm1; return expm1(x);
#else
    GEOGRAPHICLIB_VOLATILE T
      y = exp(x),
      z = y - 1;
    // The reasoning here is similar to that for log1p.  The expression
    // mathematically reduces to exp(x) - 1, and the factor z/log(y) = (y -
    // 1)/log(y) is a slowly varying quantity near y = 1 and is accurately
    // computed.
    return abs(x) > 1 ? z : (z == 0 ? x : x * z / log(y));
#endif
  }

  template<typename T> T Math::log1p(T x) {
#if GEOGRAPHICLIB_CXX11_MATH
    using std::log1p; return log1p(x);
#else
    GEOGRAPHICLIB_VOLATILE T
      y = 1 + x,
      z = y - 1;
    // Here's the explanation for this magic: y = 1 + z, exactly, and z
    // approx x, thus log(y)/z (which is nearly constant near z = 0) returns
    // a good approximation to the true log(1 + x)/x.  The multiplication x *
    // (log(y)/z) introduces little additional error.
    return z == 0 ? x : x * log(y) / z;
#endif
  }

  template<typename T> T Math::asinh(T x) {
#if GEOGRAPHICLIB_CXX11_MATH
    using std::asinh; return asinh(x);
#else
    T y = abs(x); // Enforce odd parity
    y = log1p(y * (1 + y/(hypot(T(1), y) + 1)));
    return x > 0 ? y : (x < 0 ? -y : x); // asinh(-0.0) = -0.0
#endif
  }

  template<typename T> T Math::atanh(T x) {
#if GEOGRAPHICLIB_CXX11_MATH
    using std::atanh; return atanh(x);
#else
    T y = abs(x); // Enforce odd parity
    y = log1p(2 * y/(1 - y))/2;
    return x > 0 ? y : (x < 0 ? -y : x); // atanh(-0.0) = -0.0
#endif
  }

  template<typename T> T Math::copysign(T x, T y) {
#if GEOGRAPHICLIB_CXX11_MATH
    using std::copysign; return copysign(x, y);
#else
    // NaN counts as positive
    return abs(x) * (y < 0 || (y == 0 && 1/y < 0) ? -1 : 1);
#endif
  }

  template<typename T> T Math::cbrt(T x) {
#if GEOGRAPHICLIB_CXX11_MATH
    using std::cbrt; return cbrt(x);
#else
    T y = pow(abs(x), 1/T(3)); // Return the real cube root
    return x > 0 ? y : (x < 0 ? -y : x); // cbrt(-0.0) = -0.0
#endif
  }

  template<typename T> T Math::remainder(T x, T y) {
#if GEOGRAPHICLIB_CXX11_MATH
    using std::remainder; return remainder(x, y);
#else
    y = abs(y);               // The result doesn't depend on the sign of y
    T z = fmod(x, y);
    if (z == 0)
      // This shouldn't be necessary.  However, before version 14 (2015),
      // Visual Studio had problems dealing with -0.0.  Specifically
      //   VC 10,11,12 and 32-bit compile: fmod(-0.0, 360.0) -> +0.0
      // python 2.7 on Windows 32-bit machines has the same problem.
      z = copysign(z, x);
    else if (2 * abs(z) == y)
      z -= fmod(x, 2 * y) - z; // Implement ties to even
    else if (2 * abs(z) > y)
      z += (z < 0 ? y : -y);  // Fold remaining cases to (-y/2, y/2)
    return z;
#endif
  }

  template<typename T> T Math::remquo(T x, T y, int* n) {
    // boost::math::remquo doesn't handle nans correctly
#if GEOGRAPHICLIB_CXX11_MATH && GEOGRAPHICLIB_PRECISION <= 3
    using std::remquo; return remquo(x, y, n);
#else
    T z = remainder(x, y);
    if (n) {
      T
        a = remainder(x, 2 * y),
        b = remainder(x, 4 * y),
        c = remainder(x, 8 * y);
      *n  = (a > z ? 1 : (a < z ? -1 : 0));
      *n += (b > a ? 2 : (b < a ? -2 : 0));
      *n += (c > b ? 4 : (c < b ? -4 : 0));
      if (y < 0) *n *= -1;
      if (y != 0) {
        if (x/y > 0 && *n <= 0)
          *n += 8;
        else if (x/y < 0 && *n >= 0)
          *n -= 8;
      }
    }
    return z;
#endif
  }

  template<typename T> T Math::round(T x) {
#if GEOGRAPHICLIB_CXX11_MATH
    using std::round; return round(x);
#else
    // The handling of corner cases is copied from boost; see
    //   https://github.com/boostorg/math/pull/8
    // with improvements to return -0 when appropriate.
    if      (0 < x && x <  T(0.5))
      return +T(0);
    else if (0 > x && x > -T(0.5))
      return -T(0);
    else if   (x > 0) {
      T t = ceil(x);
      return t - x > T(0.5) ? t - 1 : t;
    } else if (x < 0) {
      T t = floor(x);
      return x - t > T(0.5) ? t + 1 : t;
    } else                    // +/-0 and NaN
      return x;               // Retain sign of 0
#endif
  }

  template<typename T> long Math::lround(T x) {
#if GEOGRAPHICLIB_CXX11_MATH && GEOGRAPHICLIB_PRECISION != 5
    using std::lround; return lround(x);
#else
    // Default value for overflow + NaN + (x == LONG_MIN)
    long r = std::numeric_limits<long>::min();
    x = round(x);
    if (abs(x) < -T(r))       // Assume T(LONG_MIN) is exact
      r = long(x);
    return r;
#endif
  }

  template<typename T> T Math::fma(T x, T y, T z) {
#if GEOGRAPHICLIB_CXX11_MATH
    using std::fma; return fma(x, y, z);
#else
    return x * y + z;
#endif
  }

  template<typename T> T Math::sum(T u, T v, T& t) {
    GEOGRAPHICLIB_VOLATILE T s = u + v;
    GEOGRAPHICLIB_VOLATILE T up = s - v;
    GEOGRAPHICLIB_VOLATILE T vpp = s - up;
    up -= u;
    vpp -= v;
    t = -(up + vpp);
    // u + v =       s      + t
    //       = round(u + v) + t
    return s;
  }

  template<typename T> T Math::AngRound(T x) {
    static const T z = 1/T(16);
    if (x == 0) return 0;
    GEOGRAPHICLIB_VOLATILE T y = abs(x);
    // The compiler mustn't "simplify" z - (z - y) to y
    y = y < z ? z - (z - y) : y;
    return x < 0 ? -y : y;
  }

  template<typename T> void Math::sincosd(T x, T& sinx, T& cosx) {
    // In order to minimize round-off errors, this function exactly reduces
    // the argument to the range [-45, 45] before converting it to radians.
    T r; int q;
    // N.B. the implementation of remquo in glibc pre 2.22 were buggy.  See
    // https://sourceware.org/bugzilla/show_bug.cgi?id=17569
    // This was fixed in version 2.22 on 2015-08-05
    r = remquo(x, T(90), &q);   // now abs(r) <= 45
    r *= degree<T>();
    // g++ -O turns these two function calls into a call to sincos
    T s = sin(r), c = cos(r);
#if defined(_MSC_VER) && _MSC_VER < 1900
    // Before version 14 (2015), Visual Studio had problems dealing
    // with -0.0.  Specifically
    //   VC 10,11,12 and 32-bit compile: fmod(-0.0, 360.0) -> +0.0
    //   VC 12       and 64-bit compile:  sin(-0.0)        -> +0.0
    // AngNormalize has a similar fix.
    // python 2.7 on Windows 32-bit machines has the same problem.
    if (x == 0) s = x;
#endif
    switch (unsigned(q) & 3U) {
    case 0U: sinx =  s; cosx =  c; break;
    case 1U: sinx =  c; cosx = -s; break;
    case 2U: sinx = -s; cosx = -c; break;
    default: sinx = -c; cosx =  s; break; // case 3U
    }
    // Set sign of 0 results.  -0 only produced for sin(-0)
    if (x != 0) { sinx += T(0); cosx += T(0); }
  }

  template<typename T> T Math::sind(T x) {
    // See sincosd
    T r; int q;
    r = remquo(x, T(90), &q); // now abs(r) <= 45
    r *= degree<T>();
    unsigned p = unsigned(q);
    r = p & 1U ? cos(r) : sin(r);
    if (p & 2U) r = -r;
    if (x != 0) r += T(0);
    return r;
  }

  template<typename T> T Math::cosd(T x) {
    // See sincosd
    T r; int q;
    r = remquo(x, T(90), &q); // now abs(r) <= 45
    r *= degree<T>();
    unsigned p = unsigned(q + 1);
    r = p & 1U ? cos(r) : sin(r);
    if (p & 2U) r = -r;
    return T(0) + r;
  }

  template<typename T> T Math::tand(T x) {
    static const T overflow = 1 / sq(std::numeric_limits<T>::epsilon());
    T s, c;
    sincosd(x, s, c);
    return c != 0 ? s / c : (s < 0 ? -overflow : overflow);
  }

  template<typename T> T Math::atan2d(T y, T x) {
    // In order to minimize round-off errors, this function rearranges the
    // arguments so that result of atan2 is in the range [-pi/4, pi/4] before
    // converting it to degrees and mapping the result to the correct
    // quadrant.
    int q = 0;
    if (abs(y) > abs(x)) { std::swap(x, y); q = 2; }
    if (x < 0) { x = -x; ++q; }
    // here x >= 0 and x >= abs(y), so angle is in [-pi/4, pi/4]
    T ang = atan2(y, x) / degree<T>();
    switch (q) {
      // Note that atan2d(-0.0, 1.0) will return -0.  However, we expect that
      // atan2d will not be called with y = -0.  If need be, include
      //
      //   case 0: ang = 0 + ang; break;
      //
      // and handle mpfr as in AngRound.
    case 1: ang = (y >= 0 ? 180 : -180) - ang; break;
    case 2: ang =  90 - ang; break;
    case 3: ang = -90 + ang; break;
    }
    return ang;
  }

  template<typename T> T Math::atand(T x)
  { return atan2d(x, T(1)); }

  template<typename T> T Math::eatanhe(T x, T es)  {
    return es > T(0) ? es * atanh(es * x) : -es * atan(es * x);
  }

  template<typename T> T Math::taupf(T tau, T es) {
    T tau1 = hypot(T(1), tau),
      sig = sinh( eatanhe(tau / tau1, es ) );
    return hypot(T(1), sig) * tau - sig * tau1;
  }

  template<typename T> T Math::tauf(T taup, T es) {
    const int numit = 5;
    const T tol = sqrt(numeric_limits<T>::epsilon()) / T(10);
    T e2m = T(1) - sq(es),
      // To lowest order in e^2, taup = (1 - e^2) * tau = _e2m * tau; so use
      // tau = taup/_e2m as a starting guess.  (This starting guess is the
      // geocentric latitude which, to first order in the flattening, is equal
      // to the conformal latitude.)  Only 1 iteration is needed for |lat| <
      // 3.35 deg, otherwise 2 iterations are needed.  If, instead, tau = taup
      // is used the mean number of iterations increases to 1.99 (2 iterations
      // are needed except near tau = 0).
      tau = taup/e2m,
      stol = tol * max(T(1), abs(taup));
    // min iterations = 1, max iterations = 2; mean = 1.94
    for (int i = 0; i < numit || GEOGRAPHICLIB_PANIC; ++i) {
      T taupa = taupf(tau, es),
        dtau = (taup - taupa) * (1 + e2m * sq(tau)) /
        ( e2m * hypot(T(1), tau) * hypot(T(1), taupa) );
      tau += dtau;
      if (!(abs(dtau) >= stol))
        break;
    }
    return tau;
  }

    template<typename T> bool Math::isfinite(T x) {
#if GEOGRAPHICLIB_CXX11_MATH
      using std::isfinite; return isfinite(x);
#else
#if defined(_MSC_VER)
      return abs(x) <= (std::numeric_limits<T>::max)();
#else
      // There's a problem using MPFR C++ 3.6.3 and g++ -std=c++14 (reported on
      // 2015-05-04) with the parens around std::numeric_limits<T>::max.  Of
      // course, these parens are only needed to deal with Windows stupidly
      // defining max as a macro.  So don't insert the parens on non-Windows
      // platforms.
      return abs(x) <= std::numeric_limits<T>::max();
#endif
#endif
    }

    template<typename T> T Math::NaN() {
#if defined(_MSC_VER)
      return std::numeric_limits<T>::has_quiet_NaN ?
        std::numeric_limits<T>::quiet_NaN() :
        (std::numeric_limits<T>::max)();
#else
      return std::numeric_limits<T>::has_quiet_NaN ?
        std::numeric_limits<T>::quiet_NaN() :
        std::numeric_limits<T>::max();
#endif
    }

    template<typename T> bool Math::isnan(T x) {
#if GEOGRAPHICLIB_CXX11_MATH
      using std::isnan; return isnan(x);
#else
      return x != x;
#endif
    }

  template<typename T> T Math::infinity() {
#if defined(_MSC_VER)
      return std::numeric_limits<T>::has_infinity ?
        std::numeric_limits<T>::infinity() :
        (std::numeric_limits<T>::max)();
#else
      return std::numeric_limits<T>::has_infinity ?
        std::numeric_limits<T>::infinity() :
        std::numeric_limits<T>::max();
#endif
    }

  /// \cond SKIP
  // Instantiate
  template Math::real GEOGRAPHICLIB_EXPORT
  Math::hypot<Math::real>(Math::real, Math::real);
  template Math::real GEOGRAPHICLIB_EXPORT
  Math::expm1<Math::real>(Math::real);
  template Math::real GEOGRAPHICLIB_EXPORT
  Math::log1p<Math::real>(Math::real);
  template Math::real GEOGRAPHICLIB_EXPORT
  Math::asinh<Math::real>(Math::real);
  template Math::real GEOGRAPHICLIB_EXPORT
  Math::atanh<Math::real>(Math::real);
  template Math::real GEOGRAPHICLIB_EXPORT
  Math::cbrt<Math::real>(Math::real);
  template Math::real GEOGRAPHICLIB_EXPORT
  Math::remainder<Math::real>(Math::real, Math::real);
  template Math::real GEOGRAPHICLIB_EXPORT
  Math::remquo<Math::real>(Math::real, Math::real, int*);
  template Math::real GEOGRAPHICLIB_EXPORT
  Math::round<Math::real>(Math::real);
  template long       GEOGRAPHICLIB_EXPORT
  Math::lround<Math::real>(Math::real);
  template Math::real GEOGRAPHICLIB_EXPORT
  Math::copysign<Math::real>(Math::real, Math::real);
  template Math::real GEOGRAPHICLIB_EXPORT
  Math::fma<Math::real>(Math::real, Math::real, Math::real);
  template Math::real GEOGRAPHICLIB_EXPORT
  Math::sum<Math::real>(Math::real, Math::real, Math::real&);
  template Math::real GEOGRAPHICLIB_EXPORT
  Math::AngRound<Math::real>(Math::real);
  template void       GEOGRAPHICLIB_EXPORT
  Math::sincosd<Math::real>(Math::real, Math::real&, Math::real&);
  template Math::real GEOGRAPHICLIB_EXPORT
  Math::sind<Math::real>(Math::real);
  template Math::real GEOGRAPHICLIB_EXPORT
  Math::cosd<Math::real>(Math::real);
  template Math::real GEOGRAPHICLIB_EXPORT
  Math::tand<Math::real>(Math::real);
  template Math::real GEOGRAPHICLIB_EXPORT
  Math::atan2d<Math::real>(Math::real, Math::real);
  template Math::real GEOGRAPHICLIB_EXPORT
  Math::atand<Math::real>(Math::real);
  template Math::real GEOGRAPHICLIB_EXPORT
  Math::eatanhe<Math::real>(Math::real, Math::real);
  template Math::real GEOGRAPHICLIB_EXPORT
  Math::taupf<Math::real>(Math::real, Math::real);
  template Math::real GEOGRAPHICLIB_EXPORT
  Math::tauf<Math::real>(Math::real, Math::real);
  template bool       GEOGRAPHICLIB_EXPORT
  Math::isfinite<Math::real>(Math::real);
  template Math::real GEOGRAPHICLIB_EXPORT
  Math::NaN<Math::real>();
  template bool       GEOGRAPHICLIB_EXPORT
  Math::isnan<Math::real>(Math::real);
  template Math::real GEOGRAPHICLIB_EXPORT
  Math::infinity<Math::real>();
#if GEOGRAPHICLIB_PRECISION != 2
  // Always have double versions available
  template double GEOGRAPHICLIB_EXPORT
  Math::hypot<double>(double, double);
  template double GEOGRAPHICLIB_EXPORT
  Math::expm1<double>(double);
  template double GEOGRAPHICLIB_EXPORT
  Math::log1p<double>(double);
  template double GEOGRAPHICLIB_EXPORT
  Math::asinh<double>(double);
  template double GEOGRAPHICLIB_EXPORT
  Math::atanh<double>(double);
  template double GEOGRAPHICLIB_EXPORT
  Math::cbrt<double>(double);
  template double GEOGRAPHICLIB_EXPORT
  Math::remainder<double>(double, double);
  template double GEOGRAPHICLIB_EXPORT
  Math::remquo<double>(double, double, int*);
  template double GEOGRAPHICLIB_EXPORT
  Math::round<double>(double);
  template long   GEOGRAPHICLIB_EXPORT
  Math::lround<double>(double);
  template double GEOGRAPHICLIB_EXPORT
  Math::copysign<double>(double, double);
  template double GEOGRAPHICLIB_EXPORT
  Math::fma<double>(double, double, double);
  template double GEOGRAPHICLIB_EXPORT
  Math::sum<double>(double, double, double&);
  template double GEOGRAPHICLIB_EXPORT
  Math::AngRound<double>(double);
  template void   GEOGRAPHICLIB_EXPORT
  Math::sincosd<double>(double, double&, double&);
  template double GEOGRAPHICLIB_EXPORT
  Math::sind<double>(double);
  template double GEOGRAPHICLIB_EXPORT
  Math::cosd<double>(double);
  template double GEOGRAPHICLIB_EXPORT
  Math::tand<double>(double);
  template double GEOGRAPHICLIB_EXPORT
  Math::atan2d<double>(double, double);
  template double GEOGRAPHICLIB_EXPORT
  Math::atand<double>(double);
  template double GEOGRAPHICLIB_EXPORT
  Math::eatanhe<double>(double, double);
  template double GEOGRAPHICLIB_EXPORT
  Math::taupf<double>(double, double);
  template double GEOGRAPHICLIB_EXPORT
  Math::tauf<double>(double, double);
  template bool   GEOGRAPHICLIB_EXPORT
  Math::isfinite<double>(double);
  template double GEOGRAPHICLIB_EXPORT
  Math::NaN<double>();
  template bool   GEOGRAPHICLIB_EXPORT
  Math::isnan<double>(double);
  template double GEOGRAPHICLIB_EXPORT
  Math::infinity<double>();
#endif
#if GEOGRAPHICLIB_HAVE_LONG_DOUBLE && GEOGRAPHICLIB_PRECISION != 3
  // And always have long double versions available (as long as long double is
  // a really different from double).
  template long double GEOGRAPHICLIB_EXPORT
  Math::hypot<long double>(long double, long double);
  template long double GEOGRAPHICLIB_EXPORT
  Math::expm1<long double>(long double);
  template long double GEOGRAPHICLIB_EXPORT
  Math::log1p<long double>(long double);
  template long double GEOGRAPHICLIB_EXPORT
  Math::asinh<long double>(long double);
  template long double GEOGRAPHICLIB_EXPORT
  Math::atanh<long double>(long double);
  template long double GEOGRAPHICLIB_EXPORT
  Math::cbrt<long double>(long double);
  template long double GEOGRAPHICLIB_EXPORT
  Math::remainder<long double>(long double, long double);
  template long double GEOGRAPHICLIB_EXPORT
  Math::remquo<long double>(long double, long double, int*);
  template long double GEOGRAPHICLIB_EXPORT
  Math::round<long double>(long double);
  template long        GEOGRAPHICLIB_EXPORT
  Math::lround<long double>(long double);
  template long double GEOGRAPHICLIB_EXPORT
  Math::copysign<long double>(long double, long double);
  template long double GEOGRAPHICLIB_EXPORT
  Math::fma<long double>(long double, long double, long double);
  template long double GEOGRAPHICLIB_EXPORT
  Math::sum<long double>(long double, long double, long double&);
  template long double GEOGRAPHICLIB_EXPORT
  Math::AngRound<long double>(long double);
  template void        GEOGRAPHICLIB_EXPORT
  Math::sincosd<long double>(long double, long double&, long double&);
  template long double GEOGRAPHICLIB_EXPORT
  Math::sind<long double>(long double);
  template long double GEOGRAPHICLIB_EXPORT
  Math::cosd<long double>(long double);
  template long double GEOGRAPHICLIB_EXPORT
  Math::tand<long double>(long double);
  template long double GEOGRAPHICLIB_EXPORT
  Math::atan2d<long double>(long double, long double);
  template long double GEOGRAPHICLIB_EXPORT
  Math::atand<long double>(long double);
  template long double GEOGRAPHICLIB_EXPORT
  Math::eatanhe<long double>(long double, long double);
  template long double GEOGRAPHICLIB_EXPORT
  Math::taupf<long double>(long double, long double);
  template long double GEOGRAPHICLIB_EXPORT
  Math::tauf<long double>(long double, long double);
  template bool        GEOGRAPHICLIB_EXPORT
  Math::isfinite<long double>(long double);
  template long double GEOGRAPHICLIB_EXPORT
  Math::NaN<long double>();
  template bool        GEOGRAPHICLIB_EXPORT
  Math::isnan<long double>(long double);
  template long double GEOGRAPHICLIB_EXPORT
  Math::infinity<long double>();
#endif
  // Also we need int versions for Utility::nummatch
  template int GEOGRAPHICLIB_EXPORT Math::NaN<int>();
  template int GEOGRAPHICLIB_EXPORT Math::infinity<int>();
  /// \endcond

} // namespace GeographicLib
