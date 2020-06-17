/**
 * \file Utility.hpp
 * \brief Header for GeographicLib::Utility class
 *
 * Copyright (c) Charles Karney (2011-2019) <charles@karney.com> and licensed
 * under the MIT/X11 License.  For more information, see
 * https://geographiclib.sourceforge.io/
 **********************************************************************/

#if !defined(GEOGRAPHICLIB_UTILITY_HPP)
#define GEOGRAPHICLIB_UTILITY_HPP 1

#include "Constants.hpp"
#include <iomanip>
#include <vector>
#include <sstream>
#include <cctype>
#include <ctime>
#include <cstring>

#if defined(_MSC_VER)
// Squelch warnings about constant conditional expressions and unsafe gmtime
#  pragma warning (push)
#  pragma warning (disable: 4127 4996)
#endif

namespace GeographicLib {

  /**
   * \brief Some utility routines for %GeographicLib
   *
   * Example of use:
   * \include example-Utility.cpp
   **********************************************************************/
  class GEOGRAPHICLIB_EXPORT Utility {
  private:
    static bool gregorian(int y, int m, int d) {
      // The original cut over to the Gregorian calendar in Pope Gregory XIII's
      // time had 1582-10-04 followed by 1582-10-15. Here we implement the
      // switch over used by the English-speaking world where 1752-09-02 was
      // followed by 1752-09-14. We also assume that the year always begins
      // with January 1, whereas in reality it often was reckoned to begin in
      // March.
      return 100 * (100 * y + m) + d >= 17520914; // or 15821015
    }
    static bool gregorian(int s) {
      return s >= 639799;       // 1752-09-14
    }
  public:

    /**
     * Convert a date to the day numbering sequentially starting with
     * 0001-01-01 as day 1.
     *
     * @param[in] y the year (must be positive).
     * @param[in] m the month, Jan = 1, etc. (must be positive).  Default = 1.
     * @param[in] d the day of the month (must be positive).  Default = 1.
     * @return the sequential day number.
     **********************************************************************/
    static int day(int y, int m = 1, int d = 1) {
      // Convert from date to sequential day and vice versa
      //
      // Here is some code to convert a date to sequential day and vice
      // versa. The sequential day is numbered so that January 1, 1 AD is day 1
      // (a Saturday). So this is offset from the "Julian" day which starts the
      // numbering with 4713 BC.
      //
      // This is inspired by a talk by John Conway at the John von Neumann
      // National Supercomputer Center when he described his Doomsday algorithm
      // for figuring the day of the week. The code avoids explicitly doing ifs
      // (except for the decision of whether to use the Julian or Gregorian
      // calendar). Instead the equivalent result is achieved using integer
      // arithmetic. I got this idea from the routine for the day of the week
      // in MACLisp (I believe that that routine was written by Guy Steele).
      //
      // There are three issues to take care of
      //
      // 1. the rules for leap years,
      // 2. the inconvenient placement of leap days at the end of February,
      // 3. the irregular pattern of month lengths.
      //
      // We deal with these as follows:
      //
      // 1. Leap years are given by simple rules which are straightforward to
      // accommodate.
      //
      // 2. We simplify the calculations by moving January and February to the
      // previous year. Here we internally number the months March–December,
      // January, February as 0–9, 10, 11.
      //
      // 3. The pattern of month lengths from March through January is regular
      // with a 5-month period—31, 30, 31, 30, 31, 31, 30, 31, 30, 31, 31. The
      // 5-month period is 153 days long. Since February is now at the end of
      // the year, we don't need to include its length in this part of the
      // calculation.
      bool greg = gregorian(y, m, d);
      y += (m + 9) / 12 - 1; // Move Jan and Feb to previous year,
      m = (m + 9) % 12;      // making March month 0.
      return
        (1461 * y) / 4 // Julian years converted to days.  Julian year is 365 +
                       // 1/4 = 1461/4 days.
        // Gregorian leap year corrections.  The 2 offset with respect to the
        // Julian calendar synchronizes the vernal equinox with that at the
        // time of the Council of Nicea (325 AD).
        + (greg ? (y / 100) / 4 - (y / 100) + 2 : 0)
        + (153 * m + 2) / 5     // The zero-based start of the m'th month
        + d - 1                 // The zero-based day
        - 305; // The number of days between March 1 and December 31.
               // This makes 0001-01-01 day 1
    }

    /**
     * Convert a date to the day numbering sequentially starting with
     * 0001-01-01 as day 1.
     *
     * @param[in] y the year (must be positive).
     * @param[in] m the month, Jan = 1, etc. (must be positive).  Default = 1.
     * @param[in] d the day of the month (must be positive).  Default = 1.
     * @param[in] check whether to check the date.
     * @exception GeographicErr if the date is invalid and \e check is true.
     * @return the sequential day number.
     **********************************************************************/
    static int day(int y, int m, int d, bool check) {
      int s = day(y, m, d);
      if (!check)
        return s;
      int y1, m1, d1;
      date(s, y1, m1, d1);
      if (!(s > 0 && y == y1 && m == m1 && d == d1))
        throw GeographicErr("Invalid date " +
                            str(y) + "-" + str(m) + "-" + str(d)
                            + (s > 0 ? "; use " +
                               str(y1) + "-" + str(m1) + "-" + str(d1) :
                               " before 0001-01-01"));
      return s;
    }

    /**
     * Given a day (counting from 0001-01-01 as day 1), return the date.
     *
     * @param[in] s the sequential day number (must be positive)
     * @param[out] y the year.
     * @param[out] m the month, Jan = 1, etc.
     * @param[out] d the day of the month.
     **********************************************************************/
    static void date(int s, int& y, int& m, int& d) {
      int c = 0;
      bool greg = gregorian(s);
      s += 305;                 // s = 0 on March 1, 1BC
      if (greg) {
        s -= 2;                 // The 2 day Gregorian offset
        // Determine century with the Gregorian rules for leap years.  The
        // Gregorian year is 365 + 1/4 - 1/100 + 1/400 = 146097/400 days.
        c = (4 * s + 3) / 146097;
        s -= (c * 146097) / 4;  // s = 0 at beginning of century
      }
      y = (4 * s + 3) / 1461;   // Determine the year using Julian rules.
      s -= (1461 * y) / 4;      // s = 0 at start of year, i.e., March 1
      y += c * 100;             // Assemble full year
      m = (5 * s + 2) / 153;    // Determine the month
      s -= (153 * m + 2) / 5;   // s = 0 at beginning of month
      d = s + 1;                // Determine day of month
      y += (m + 2) / 12;        // Move Jan and Feb back to original year
      m = (m + 2) % 12 + 1;     // Renumber the months so January = 1
    }

    /**
     * Given a date as a string in the format yyyy, yyyy-mm, or yyyy-mm-dd,
     * return the numeric values for the year, month, and day.  No checking is
     * done on these values.  The string "now" is interpreted as the present
     * date (in UTC).
     *
     * @param[in] s the date in string format.
     * @param[out] y the year.
     * @param[out] m the month, Jan = 1, etc.
     * @param[out] d the day of the month.
     * @exception GeographicErr is \e s is malformed.
     **********************************************************************/
    static void date(const std::string& s, int& y, int& m, int& d) {
      if (s == "now") {
        std::time_t t = std::time(0);
        struct tm* now = gmtime(&t);
        y = now->tm_year + 1900;
        m = now->tm_mon + 1;
        d = now->tm_mday;
        return;
      }
      int y1, m1 = 1, d1 = 1;
      const char* digits = "0123456789";
      std::string::size_type p1 = s.find_first_not_of(digits);
      if (p1 == std::string::npos)
        y1 = val<int>(s);
      else if (s[p1] != '-')
        throw GeographicErr("Delimiter not hyphen in date " + s);
      else if (p1 == 0)
        throw GeographicErr("Empty year field in date " + s);
      else {
        y1 = val<int>(s.substr(0, p1));
        if (++p1 == s.size())
          throw GeographicErr("Empty month field in date " + s);
        std::string::size_type p2 = s.find_first_not_of(digits, p1);
        if (p2 == std::string::npos)
          m1 = val<int>(s.substr(p1));
        else if (s[p2] != '-')
          throw GeographicErr("Delimiter not hyphen in date " + s);
        else if (p2 == p1)
          throw GeographicErr("Empty month field in date " + s);
        else {
          m1 = val<int>(s.substr(p1, p2 - p1));
          if (++p2 == s.size())
            throw GeographicErr("Empty day field in date " + s);
          d1 = val<int>(s.substr(p2));
        }
      }
      y = y1; m = m1; d = d1;
    }

    /**
     * Given the date, return the day of the week.
     *
     * @param[in] y the year (must be positive).
     * @param[in] m the month, Jan = 1, etc. (must be positive).
     * @param[in] d the day of the month (must be positive).
     * @return the day of the week with Sunday, Monday--Saturday = 0,
     *   1--6.
     **********************************************************************/
    static int dow(int y, int m, int d) { return dow(day(y, m, d)); }

    /**
     * Given the sequential day, return the day of the week.
     *
     * @param[in] s the sequential day (must be positive).
     * @return the day of the week with Sunday, Monday--Saturday = 0,
     *   1--6.
     **********************************************************************/
    static int dow(int s) {
      return (s + 5) % 7;  // The 5 offset makes day 1 (0001-01-01) a Saturday.
    }

    /**
     * Convert a string representing a date to a fractional year.
     *
     * @tparam T the type of the argument.
     * @param[in] s the string to be converted.
     * @exception GeographicErr if \e s can't be interpreted as a date.
     * @return the fractional year.
     *
     * The string is first read as an ordinary number (e.g., 2010 or 2012.5);
     * if this is successful, the value is returned.  Otherwise the string
     * should be of the form yyyy-mm or yyyy-mm-dd and this is converted to a
     * number with 2010-01-01 giving 2010.0 and 2012-07-03 giving 2012.5.
     **********************************************************************/
    template<typename T> static T fractionalyear(const std::string& s) {
      try {
        return val<T>(s);
      }
      catch (const std::exception&) {}
      int y, m, d;
      date(s, y, m, d);
      int t = day(y, m, d, true);
      return T(y) + T(t - day(y)) / T(day(y + 1) - day(y));
    }

    /**
     * Convert a object of type T to a string.
     *
     * @tparam T the type of the argument.
     * @param[in] x the value to be converted.
     * @param[in] p the precision used (default &minus;1).
     * @exception std::bad_alloc if memory for the string can't be allocated.
     * @return the string representation.
     *
     * If \e p &ge; 0, then the number fixed format is used with p bits of
     * precision.  With p < 0, there is no manipulation of the format.
     **********************************************************************/
    template<typename T> static std::string str(T x, int p = -1) {
      std::ostringstream s;
      if (p >= 0) s << std::fixed << std::setprecision(p);
      s << x; return s.str();
    }

    /**
     * Convert a Math::real object to a string.
     *
     * @param[in] x the value to be converted.
     * @param[in] p the precision used (default &minus;1).
     * @exception std::bad_alloc if memory for the string can't be allocated.
     * @return the string representation.
     *
     * If \e p &ge; 0, then the number fixed format is used with p bits of
     * precision.  With p < 0, there is no manipulation of the format.  This is
     * an overload of str<T> which deals with inf and nan.
     **********************************************************************/
    static std::string str(Math::real x, int p = -1) {
      if (!Math::isfinite(x))
        return x < 0 ? std::string("-inf") :
          (x > 0 ? std::string("inf") : std::string("nan"));
      std::ostringstream s;
#if GEOGRAPHICLIB_PRECISION == 4
      // boost-quadmath treats precision == 0 as "use as many digits as
      // necessary" (see https://svn.boost.org/trac/boost/ticket/10103), so...
      using std::floor; using std::fmod;
      if (p == 0) {
        x += Math::real(0.5);
        Math::real ix = floor(x);
        // Implement the "round ties to even" rule
        x = (ix == x && fmod(ix, Math::real(2)) == 1) ? ix - 1 : ix;
        s << std::fixed << std::setprecision(1) << x;
        std::string r(s.str());
        // strip off trailing ".0"
        return r.substr(0, (std::max)(int(r.size()) - 2, 0));
      }
#endif
      if (p >= 0) s << std::fixed << std::setprecision(p);
      s << x; return s.str();
    }

    /**
     * Trim the white space from the beginning and end of a string.
     *
     * @param[in] s the string to be trimmed
     * @return the trimmed string
     **********************************************************************/
    static std::string trim(const std::string& s) {
      unsigned
        beg = 0,
        end = unsigned(s.size());
      while (beg < end && isspace(s[beg]))
        ++beg;
      while (beg < end && isspace(s[end - 1]))
        --end;
      return std::string(s, beg, end-beg);
    }

    /**
     * Convert a string to type T.
     *
     * @tparam T the type of the return value.
     * @param[in] s the string to be converted.
     * @exception GeographicErr is \e s is not readable as a T.
     * @return object of type T.
     *
     * White space at the beginning and end of \e s is ignored.
     *
     * Special handling is provided for some types.
     *
     * If T is a floating point type, then inf and nan are recognized.
     *
     * If T is bool, then \e s should either be string a representing 0 (false)
     * or 1 (true) or one of the strings
     * - "false", "f", "nil", "no", "n", "off", or "" meaning false,
     * - "true", "t", "yes", "y", or "on" meaning true;
     * .
     * case is ignored.
     *
     * If T is std::string, then \e s is returned (with the white space at the
     * beginning and end removed).
     **********************************************************************/
    template<typename T> static T val(const std::string& s) {
      // If T is bool, then the specialization val<bool>() defined below is
      // used.
      T x;
      std::string errmsg, t(trim(s));
      do {                     // Executed once (provides the ability to break)
        std::istringstream is(t);
        if (!(is >> x)) {
          errmsg = "Cannot decode " + t;
          break;
        }
        int pos = int(is.tellg()); // Returns -1 at end of string?
        if (!(pos < 0 || pos == int(t.size()))) {
          errmsg = "Extra text " + t.substr(pos) + " at end of " + t;
          break;
        }
        return x;
      } while (false);
      x = std::numeric_limits<T>::is_integer ? 0 : nummatch<T>(t);
      if (x == 0)
        throw GeographicErr(errmsg);
      return x;
    }
    /**
     * \deprecated An old name for val<T>(s).
     **********************************************************************/
    template<typename T>
      GEOGRAPHICLIB_DEPRECATED("Use Utility::val<T>(s)")
      static T num(const std::string& s) {
      return val<T>(s);
    }

    /**
     * Match "nan" and "inf" (and variants thereof) in a string.
     *
     * @tparam T the type of the return value (this should be a floating point
     *   type).
     * @param[in] s the string to be matched.
     * @return appropriate special value (&plusmn;&infin;, nan) or 0 if none is
     *   found.
     *
     * White space is not allowed at the beginning or end of \e s.
     **********************************************************************/
    template<typename T> static T nummatch(const std::string& s) {
      if (s.length() < 3)
        return 0;
      std::string t(s);
      for (std::string::iterator p = t.begin(); p != t.end(); ++p)
        *p = char(std::toupper(*p));
      for (size_t i = s.length(); i--;)
        t[i] = char(std::toupper(s[i]));
      int sign = t[0] == '-' ? -1 : 1;
      std::string::size_type p0 = t[0] == '-' || t[0] == '+' ? 1 : 0;
      std::string::size_type p1 = t.find_last_not_of('0');
      if (p1 == std::string::npos || p1 + 1 < p0 + 3)
        return 0;
      // Strip off sign and trailing 0s
      t = t.substr(p0, p1 + 1 - p0);  // Length at least 3
      if (t == "NAN" || t == "1.#QNAN" || t == "1.#SNAN" || t == "1.#IND" ||
          t == "1.#R")
        return Math::NaN<T>();
      else if (t == "INF" || t == "1.#INF")
        return sign * Math::infinity<T>();
      return 0;
    }

    /**
     * Read a simple fraction, e.g., 3/4, from a string to an object of type T.
     *
     * @tparam T the type of the return value.
     * @param[in] s the string to be converted.
     * @exception GeographicErr is \e s is not readable as a fraction of type
     *   T.
     * @return object of type T
     *
     * \note The msys shell under Windows converts arguments which look
     * like pathnames into their Windows equivalents.  As a result the argument
     * "-1/300" gets mangled into something unrecognizable.  A workaround is to
     * use a floating point number in the numerator, i.e., "-1.0/300".
     **********************************************************************/
    template<typename T> static T fract(const std::string& s) {
      std::string::size_type delim = s.find('/');
      return
        !(delim != std::string::npos && delim >= 1 && delim + 2 <= s.size()) ?
        val<T>(s) :
        // delim in [1, size() - 2]
        val<T>(s.substr(0, delim)) / val<T>(s.substr(delim + 1));
    }

    /**
     * Lookup up a character in a string.
     *
     * @param[in] s the string to be searched.
     * @param[in] c the character to look for.
     * @return the index of the first occurrence character in the string or
     *   &minus;1 is the character is not present.
     *
     * \e c is converted to upper case before search \e s.  Therefore, it is
     * intended that \e s should not contain any lower case letters.
     **********************************************************************/
    static int lookup(const std::string& s, char c) {
      std::string::size_type r = s.find(char(toupper(c)));
      return r == std::string::npos ? -1 : int(r);
    }

    /**
     * Lookup up a character in a char*.
     *
     * @param[in] s the char* string to be searched.
     * @param[in] c the character to look for.
     * @return the index of the first occurrence character in the string or
     *   &minus;1 is the character is not present.
     *
     * \e c is converted to upper case before search \e s.  Therefore, it is
     * intended that \e s should not contain any lower case letters.
     **********************************************************************/
    static int lookup(const char* s, char c) {
      const char* p = std::strchr(s, toupper(c));
      return p != NULL ? int(p - s) : -1;
    }

    /**
     * Read data of type ExtT from a binary stream to an array of type IntT.
     * The data in the file is in (bigendp ? big : little)-endian format.
     *
     * @tparam ExtT the type of the objects in the binary stream (external).
     * @tparam IntT the type of the objects in the array (internal).
     * @tparam bigendp true if the external storage format is big-endian.
     * @param[in] str the input stream containing the data of type ExtT
     *   (external).
     * @param[out] array the output array of type IntT (internal).
     * @param[in] num the size of the array.
     * @exception GeographicErr if the data cannot be read.
     **********************************************************************/
    template<typename ExtT, typename IntT, bool bigendp>
      static void readarray(std::istream& str, IntT array[], size_t num) {
#if GEOGRAPHICLIB_PRECISION < 4
      if (sizeof(IntT) == sizeof(ExtT) &&
          std::numeric_limits<IntT>::is_integer ==
          std::numeric_limits<ExtT>::is_integer)
        {
          // Data is compatible (aside from the issue of endian-ness).
          str.read(reinterpret_cast<char*>(array), num * sizeof(ExtT));
          if (!str.good())
            throw GeographicErr("Failure reading data");
          if (bigendp != Math::bigendian) { // endian mismatch -> swap bytes
            for (size_t i = num; i--;)
              array[i] = Math::swab<IntT>(array[i]);
          }
        }
      else
#endif
        {
          const int bufsize = 1024; // read this many values at a time
          ExtT buffer[bufsize];     // temporary buffer
          int k = int(num);         // data values left to read
          int i = 0;                // index into output array
          while (k) {
            int n = (std::min)(k, bufsize);
            str.read(reinterpret_cast<char*>(buffer), n * sizeof(ExtT));
            if (!str.good())
              throw GeographicErr("Failure reading data");
            for (int j = 0; j < n; ++j)
              // fix endian-ness and cast to IntT
              array[i++] = IntT(bigendp == Math::bigendian ? buffer[j] :
                                Math::swab<ExtT>(buffer[j]));
            k -= n;
          }
        }
      return;
    }

    /**
     * Read data of type ExtT from a binary stream to a vector array of type
     * IntT.  The data in the file is in (bigendp ? big : little)-endian
     * format.
     *
     * @tparam ExtT the type of the objects in the binary stream (external).
     * @tparam IntT the type of the objects in the array (internal).
     * @tparam bigendp true if the external storage format is big-endian.
     * @param[in] str the input stream containing the data of type ExtT
     *   (external).
     * @param[out] array the output vector of type IntT (internal).
     * @exception GeographicErr if the data cannot be read.
     **********************************************************************/
    template<typename ExtT, typename IntT, bool bigendp>
      static void readarray(std::istream& str, std::vector<IntT>& array) {
      if (array.size() > 0)
        readarray<ExtT, IntT, bigendp>(str, &array[0], array.size());
    }

    /**
     * Write data in an array of type IntT as type ExtT to a binary stream.
     * The data in the file is in (bigendp ? big : little)-endian format.
     *
     * @tparam ExtT the type of the objects in the binary stream (external).
     * @tparam IntT the type of the objects in the array (internal).
     * @tparam bigendp true if the external storage format is big-endian.
     * @param[out] str the output stream for the data of type ExtT (external).
     * @param[in] array the input array of type IntT (internal).
     * @param[in] num the size of the array.
     * @exception GeographicErr if the data cannot be written.
     **********************************************************************/
    template<typename ExtT, typename IntT, bool bigendp>
      static void writearray(std::ostream& str, const IntT array[], size_t num)
    {
#if GEOGRAPHICLIB_PRECISION < 4
      if (sizeof(IntT) == sizeof(ExtT) &&
          std::numeric_limits<IntT>::is_integer ==
          std::numeric_limits<ExtT>::is_integer &&
          bigendp == Math::bigendian)
        {
          // Data is compatible (including endian-ness).
          str.write(reinterpret_cast<const char*>(array), num * sizeof(ExtT));
          if (!str.good())
            throw GeographicErr("Failure writing data");
        }
      else
#endif
        {
          const int bufsize = 1024; // write this many values at a time
          ExtT buffer[bufsize];     // temporary buffer
          int k = int(num);         // data values left to write
          int i = 0;                // index into output array
          while (k) {
            int n = (std::min)(k, bufsize);
            for (int j = 0; j < n; ++j)
              // cast to ExtT and fix endian-ness
              buffer[j] = bigendp == Math::bigendian ? ExtT(array[i++]) :
                Math::swab<ExtT>(ExtT(array[i++]));
            str.write(reinterpret_cast<const char*>(buffer), n * sizeof(ExtT));
            if (!str.good())
              throw GeographicErr("Failure writing data");
            k -= n;
          }
        }
      return;
    }

    /**
     * Write data in an array of type IntT as type ExtT to a binary stream.
     * The data in the file is in (bigendp ? big : little)-endian format.
     *
     * @tparam ExtT the type of the objects in the binary stream (external).
     * @tparam IntT the type of the objects in the array (internal).
     * @tparam bigendp true if the external storage format is big-endian.
     * @param[out] str the output stream for the data of type ExtT (external).
     * @param[in] array the input vector of type IntT (internal).
     * @exception GeographicErr if the data cannot be written.
     **********************************************************************/
    template<typename ExtT, typename IntT, bool bigendp>
      static void writearray(std::ostream& str, std::vector<IntT>& array) {
      if (array.size() > 0)
        writearray<ExtT, IntT, bigendp>(str, &array[0], array.size());
    }

    /**
     * Parse a KEY VALUE line.
     *
     * @param[in] line the input line.
     * @param[out] key the key.
     * @param[out] val the value.
     * @exception std::bad_alloc if memory for the internal strings can't be
     *   allocated.
     * @return whether a key was found.
     *
     * A # character and everything after it are discarded.  If the result is
     * just white space, the routine returns false (and \e key and \e val are
     * not set).  Otherwise the first token is taken to be the key and the rest
     * of the line (trimmed of leading and trailing white space) is the value.
     **********************************************************************/
    static bool ParseLine(const std::string& line,
                          std::string& key, std::string& val);

    /**
     * Set the binary precision of a real number.
     *
     * @param[in] ndigits the number of bits of precision.  If ndigits is 0
     *   (the default), then determine the precision from the environment
     *   variable GEOGRAPHICLIB_DIGITS.  If this is undefined, use ndigits =
     *   256 (i.e., about 77 decimal digits).
     * @return the resulting number of bits of precision.
     *
     * This only has an effect when GEOGRAPHICLIB_PRECISION = 5.  The
     * precision should only be set once and before calls to any other
     * GeographicLib functions.  (Several functions, for example Math::pi(),
     * cache the return value in a static local variable.  The precision needs
     * to be set before a call to any such functions.)  In multi-threaded
     * applications, it is necessary also to set the precision in each thread
     * (see the example GeoidToGTX.cpp).
     **********************************************************************/
    static int set_digits(int ndigits = 0);

  };

  /**
   * The specialization of Utility::val<T>() for strings.
   **********************************************************************/
  template<> inline std::string Utility::val<std::string>(const std::string& s)
  { return trim(s); }

  /**
   * The specialization of Utility::val<T>() for bools.
   **********************************************************************/
  template<> inline bool Utility::val<bool>(const std::string& s) {
    std::string t(trim(s));
    if (t.empty()) return false;
    bool x;
    std::istringstream is(t);
    if (is >> x) {
      int pos = int(is.tellg()); // Returns -1 at end of string?
      if (!(pos < 0 || pos == int(t.size())))
        throw GeographicErr("Extra text " + t.substr(pos) +
                            " at end of " + t);
      return x;
    }
    for (std::string::iterator p = t.begin(); p != t.end(); ++p)
      *p = char(std::tolower(*p));
    switch (t[0]) {             // already checked that t isn't empty
    case 'f':
      if (t == "f" || t == "false") return false;
      break;
    case 'n':
      if (t == "n" || t == "nil" || t == "no") return false;
      break;
    case 'o':
      if (t == "off") return false;
      else if (t == "on") return true;
      break;
    case 't':
      if (t == "t" || t == "true") return true;
      break;
    case 'y':
      if (t == "y" || t == "yes") return true;
      break;
    }
    throw GeographicErr("Cannot decode " + t + " as a bool");
  }

} // namespace GeographicLib

#if defined(_MSC_VER)
#  pragma warning (pop)
#endif

#endif  // GEOGRAPHICLIB_UTILITY_HPP
