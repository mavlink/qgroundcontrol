#pragma once

#include "UnitTest.h"

class RTCMMavlinkTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testEmpty();
    void _testUnfragmentedSmall();
    void _testUnfragmentedExact180();
    void _testFragmented181();
    void _testExactMultiple360Terminator();
    void _testExactMultiple540Terminator();
    void _testFourFragmentsPartialTail();
    void _testExact720NoTerminator();
    void _testOversizedStreamsUnfragmented();
    void _testSequenceAdvances();
    void _testFlagsBitLayout();
};
