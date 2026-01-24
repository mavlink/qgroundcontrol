#pragma once

#include "UnitTest.h"

/// Unit tests for VideoReceiver base class functionality.
/// Tests enum validation, state management, and signal emission.
class VideoReceiverTest : public UnitTest
{
    Q_OBJECT

public:
    VideoReceiverTest() = default;

private slots:
    void _testFileFormatValidation();
    void _testStatusValidation();
    void _testIsThermal();
    void _testPropertySettersEmitSignals();
    void _testUriChanges();
    void _testLowLatencyMode();
};
