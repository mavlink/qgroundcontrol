#pragma once

#include "TestFixtures.h"

/// Unit tests for AudioOutput text-to-speech functionality.
/// Tests text transformations for better audio output readability.
class AudioOutputTest : public OfflineTest
{
    Q_OBJECT

public:
    AudioOutputTest() = default;

private slots:
    // Abbreviation replacement tests
    void _testAbbreviationERR();
    void _testAbbreviationPOSCTL();
    void _testAbbreviationALTCTL();
    void _testAbbreviationRTL();
    void _testAbbreviationACCEL();
    void _testAbbreviationWP();
    void _testAbbreviationCMD();
    void _testAbbreviationPARAMS();
    void _testAbbreviationID();
    void _testAbbreviationADSB();
    void _testAbbreviationEKF();
    void _testAbbreviationPREARM();
    void _testAbbreviationMultiple();
    void _testAbbreviationCaseInsensitive();

    // Negative sign replacement tests
    void _testNegativeNumber();
    void _testNegativeWithSpace();
    void _testNegativeInSentence();
    void _testNonNegativeNotReplaced();

    // Decimal point replacement tests
    void _testDecimalPointSimple();
    void _testDecimalPointMultiple();
    void _testDecimalPointWithUnits();

    // Meters replacement tests
    void _testMetersSimple();
    void _testMetersWithSpace();
    void _testMetersMultiple();
    void _testMetersNotOtherUnits();
    void _testMetersEdgeCases();

    // Milliseconds conversion tests
    void _testMillisecondsUnder1000();
    void _testMilliseconds1000();
    void _testMillisecondsWithRemainder();
    void _testMillisecondsMinutes();
    void _testMillisecondsMinutesAndSeconds();

    // Combined transformation tests
    void _testCombinedTransformations();

    // Edge case tests
    void _testEmptyString();
    void _testNoTransformationNeeded();
    void _testWhitespaceOnly();
};
