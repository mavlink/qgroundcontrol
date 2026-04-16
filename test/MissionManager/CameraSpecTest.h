#pragma once

#include "UnitTest.h"

/// Unit tests for the CameraSpec data model.
///
/// CameraSpec is a pure persistence/data model wrapping eight SettingsFacts
/// (sensor width/height, image width/height, focal length, landscape flag,
/// fixedOrientation flag, minimum trigger interval). It exposes a JSON
/// `save()` / `load()` pair, a dirty flag, and an assignment operator.
/// Previously no direct test existed — indirect coverage came only through
/// CameraCalc, which obscured regressions in the JSON round trip and the
/// dirty-signal logic.
class CameraSpecTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testDefaultValues();

    void _testSaveProducesExpectedKeysAndTypes();
    void _testLoadValidJsonRoundTrip();
    void _testLoadMissingKeyFails();
    void _testLoadWrongTypeFails();

    void _testAssignmentCopiesAllFacts();

    void _testDirtyInitiallyFalse();
    void _testSetDirtyEmitsOnlyOnChange();
};
