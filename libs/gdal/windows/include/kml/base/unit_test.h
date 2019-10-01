// Copyright 2008, Google Inc. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//  3. Neither the name of Google Inc. nor the names of its contributors may be
//     used to endorse or promote products derived from this software without
//     specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
// WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
// EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef KML_BASE_UNIT_TEST_H__
#define KML_BASE_UNIT_TEST_H__

/**
 * This file provides a simple main() function for running unit tests.
 * Include this file with any unit test compilation.
 */
#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/config/SourcePrefix.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/TestRunner.h>

#define TEST_MAIN int main( int argc, char **argv) {\
  /* Create the event manager and test controller */\
  CPPUNIT_NS::TestResult controller;\
\
  /* Add a listener that colllects test result */\
  CPPUNIT_NS::TestResultCollector result;\
  controller.addListener(&result);\
\
  /* Add a listener that print dots as test run. */\
  CPPUNIT_NS::BriefTestProgressListener progress;\
  controller.addListener(&progress);\
\
  /* Add the top suite to the test runner */\
  CPPUNIT_NS::TestRunner runner;\
  runner.addTest(CPPUNIT_NS::TestFactoryRegistry::getRegistry().makeTest());\
  runner.run(controller);\
\
  /* Print test in a compiler compatible format. */\
  CPPUNIT_NS::CompilerOutputter outputter(&result, CPPUNIT_NS::stdCOut());\
  outputter.write();\
\
  return result.wasSuccessful() ? 0 : 1;\
}

#endif  // KML_BASE_UNIT_TEST_H__
