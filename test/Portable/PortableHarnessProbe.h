#pragma once

#include "PortableTest.h"

/// Test functions that misbehave, to show that the portable harness fails a test that does. Each is run alone, as a
/// CTest entry of its own (test/Portable/CMakeLists.txt): one that fails on purpose passes on the failure it is to
/// cause. The probe is no suite to run as a whole, and the application's harness, which shares the checks
/// (UnitTestBase), does not run it
class PortableHarnessProbe : public PortableTest
{
    Q_OBJECT

private slots:
    void _unexpectedWarning();
    void _unverifiedExpectation();
    void _expectationWithoutMessage();
    void _debugAndInfoAreOff();
    void _warningFromAnotherThread();
};
