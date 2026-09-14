#pragma once

#include "UnitTest.h"

class RTCMMavlinkTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testOutputFanout();
    void _testPartialOutput_data();
    void _testPartialOutput();
    void _testOutputReplacementAndDeletion();
    void _testFinalAdmissionRetirement_data();
    void _testFinalAdmissionRetirement();
    void _testEmpty();
    void _testPackCompatibility();
    void _testSequenceAdvances();
};
