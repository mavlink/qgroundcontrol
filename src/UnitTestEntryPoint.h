#pragma once

#include "QGCCommandLineParser.h"

/// @file
/// @brief What the application calls in the unit test library (QGC_UNITTEST_BUILD)
///
/// Declared under src/ because application sources compile without test include paths.
/// test/UnitTestList.cc defines it.

namespace QGCUnitTest {

/// Handle all test-related command line options (--list-tests, --unittest, etc.)
/// @param args Parsed command line arguments
/// @return 0 on success, negative number indicating failure count
int handleTestOptions(const QGCCommandLineParser::CommandLineParseResult& args);

}  // namespace QGCUnitTest
