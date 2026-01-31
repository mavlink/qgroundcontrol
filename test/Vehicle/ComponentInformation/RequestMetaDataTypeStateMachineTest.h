#pragma once

#include "UnitTest.h"

class RequestMetaDataTypeStateMachineTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testStateCreation();
    void _testRequestFlow();
    void _testSkipDeprecatedWhenSupported();
    void _testArduPilotMetadata();
    void _testStateMachineCompletion();
};
