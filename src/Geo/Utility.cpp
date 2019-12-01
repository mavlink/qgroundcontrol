/**
 * \file Utility.cpp
 * \brief Implementation for GeographicLib::Utility class
 *
 * Copyright (c) Charles Karney (2011) <charles@karney.com> and licensed under
 * the MIT/X11 License.  For more information, see
 * https://geographiclib.sourceforge.io/
 **********************************************************************/

#include <cstdlib>
#include "Utility.hpp"

#if defined(_MSC_VER)
// Squelch warnings about unsafe use of getenv
#  pragma warning (disable: 4996)
#endif

namespace GeographicLib {

  using namespace std;

  bool Utility::ParseLine(const std::string& line,
                          std::string& key, std::string& val) {
    const char* spaces = " \t\n\v\f\r";
    string::size_type n0 = line.find_first_not_of(spaces);
    if (n0 == string::npos)
      return false;             // Blank line
    string::size_type n1 = line.find_first_of('#', n0);
    if (n0 == n1)
      return false;             // Only a comment
    val = line.substr(n0, n1 == string::npos ? n1 : n1 - n0);
    n0 = val.find_first_of(spaces);
    key = val.substr(0, n0);
    if (n0 == string::npos) {
      val = "";
      return true;
    }
    n0 = val.find_first_not_of(spaces, n0);
    if (n0 == string::npos) {
      val = "";
      return true;
    }
    n1 = val.find_last_not_of(spaces);
    val = val.substr(n0, n1 + 1 - n0);
    return true;
  }

  int Utility::set_digits(int ndigits) {
#if GEOGRAPHICLIB_PRECISION == 5
    if (ndigits <= 0) {
      char* digitenv = getenv("GEOGRAPHICLIB_DIGITS");
      if (digitenv)
        ndigits = strtol(digitenv, NULL, 0);
      if (ndigits <= 0)
        ndigits = 256;
    }
#endif
    return Math::set_digits(ndigits);
  }

} // namespace GeographicLib
