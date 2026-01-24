#include "AudioOutputTest.h"
#include "AudioOutput.h"

#include <QtTest/QTest>

// ============================================================================
// Abbreviation Replacement Tests
// ============================================================================

void AudioOutputTest::_testAbbreviationERR()
{
    QString result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("ERR detected"));
    QCOMPARE(result, QStringLiteral("error detected"));
}

void AudioOutputTest::_testAbbreviationPOSCTL()
{
    QString result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("POSCTL mode"));
    QCOMPARE(result, QStringLiteral("Position Control mode"));
}

void AudioOutputTest::_testAbbreviationALTCTL()
{
    QString result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("ALTCTL engaged"));
    QCOMPARE(result, QStringLiteral("Altitude Control engaged"));
}

void AudioOutputTest::_testAbbreviationRTL()
{
    QString result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("RTL activated"));
    QCOMPARE(result, QStringLiteral("return To launch activated"));

    result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("AUTO_RTL mode"));
    QCOMPARE(result, QStringLiteral("auto return to launch mode"));
}

void AudioOutputTest::_testAbbreviationACCEL()
{
    QString result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("ACCEL calibration"));
    QCOMPARE(result, QStringLiteral("accelerometer calibration"));
}

void AudioOutputTest::_testAbbreviationWP()
{
    QString result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("WP 5 reached"));
    QCOMPARE(result, QStringLiteral("waypoint 5 reached"));
}

void AudioOutputTest::_testAbbreviationCMD()
{
    QString result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("CMD accepted"));
    QCOMPARE(result, QStringLiteral("command accepted"));
}

void AudioOutputTest::_testAbbreviationPARAMS()
{
    QString result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("PARAMS loaded"));
    QCOMPARE(result, QStringLiteral("parameters loaded"));
}

void AudioOutputTest::_testAbbreviationID()
{
    QString result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("ID 123"));
    QCOMPARE(result, QStringLiteral("I.D. 123"));
}

void AudioOutputTest::_testAbbreviationADSB()
{
    QString result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("ADSB vehicle"));
    QCOMPARE(result, QStringLiteral("A.D.S.B. vehicle"));
}

void AudioOutputTest::_testAbbreviationEKF()
{
    QString result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("EKF error"));
    QCOMPARE(result, QStringLiteral("E.K.F. error"));
}

void AudioOutputTest::_testAbbreviationPREARM()
{
    QString result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("PREARM check failed"));
    QCOMPARE(result, QStringLiteral("pre arm check failed"));
}

void AudioOutputTest::_testAbbreviationMultiple()
{
    QString result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("ERR EKF PREARM"));
    QCOMPARE(result, QStringLiteral("error E.K.F. pre arm"));
}

void AudioOutputTest::_testAbbreviationCaseInsensitive()
{
    QString result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("err detected"));
    QCOMPARE(result, QStringLiteral("error detected"));

    result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("Err detected"));
    QCOMPARE(result, QStringLiteral("error detected"));
}

// ============================================================================
// Negative Sign Replacement Tests
// ============================================================================

void AudioOutputTest::_testNegativeNumber()
{
    QString result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("-10"));
    QCOMPARE(result, QStringLiteral("negative 10"));
}

void AudioOutputTest::_testNegativeWithSpace()
{
    QString result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("- 5"));
    QCOMPARE(result, QStringLiteral("negative 5"));
}

void AudioOutputTest::_testNegativeInSentence()
{
    QString result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("altitude -100 feet"));
    QCOMPARE(result, QStringLiteral("altitude negative 100 feet"));
}

void AudioOutputTest::_testNonNegativeNotReplaced()
{
    QString result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("-foom"));
    QCOMPARE(result, QStringLiteral("-foom"));

    result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("a-b"));
    QCOMPARE(result, QStringLiteral("a-b"));
}

// ============================================================================
// Decimal Point Replacement Tests
// ============================================================================

void AudioOutputTest::_testDecimalPointSimple()
{
    QString result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("10.5"));
    QCOMPARE(result, QStringLiteral("10 point 5"));
}

void AudioOutputTest::_testDecimalPointMultiple()
{
    QString result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("10.5 and 20.3"));
    QCOMPARE(result, QStringLiteral("10 point 5 and 20 point 3"));
}

void AudioOutputTest::_testDecimalPointWithUnits()
{
    QString result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("10.5m"));
    QCOMPARE(result, QStringLiteral("10 point 5 meters"));
}

// ============================================================================
// Meters Replacement Tests
// ============================================================================

void AudioOutputTest::_testMetersSimple()
{
    QString result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("10m"));
    QCOMPARE(result, QStringLiteral("10 meters"));
}

void AudioOutputTest::_testMetersWithSpace()
{
    QString result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("10 m"));
    QCOMPARE(result, QStringLiteral("10  meters"));
}

void AudioOutputTest::_testMetersMultiple()
{
    QString result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("-10.5m, -10.5m. -10.5 m"));
    QCOMPARE(result, QStringLiteral("negative 10 point 5 meters, negative 10 point 5 meters. negative 10 point 5  meters"));
}

void AudioOutputTest::_testMetersNotOtherUnits()
{
    QString result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("10 moo"));
    QCOMPARE(result, QStringLiteral("10 moo"));

    result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("10moo"));
    QCOMPARE(result, QStringLiteral("10moo"));

    result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("10ms"));
    QCOMPARE(result, QStringLiteral("10ms"));
}

void AudioOutputTest::_testMetersEdgeCases()
{
    QString result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("foo -10m -10 m bar"));
    QCOMPARE(result, QStringLiteral("foo negative 10 meters negative 10  meters bar"));

    result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("-10m -10 m"));
    QCOMPARE(result, QStringLiteral("negative 10 meters negative 10  meters"));
}

// ============================================================================
// Milliseconds Conversion Tests
// ============================================================================

void AudioOutputTest::_testMillisecondsUnder1000()
{
    QString result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("10ms"));
    QCOMPARE(result, QStringLiteral("10ms"));

    result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("999ms"));
    QCOMPARE(result, QStringLiteral("999ms"));
}

void AudioOutputTest::_testMilliseconds1000()
{
    QString result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("1000ms"));
    QCOMPARE(result, QStringLiteral("1 second"));

    result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("2000ms"));
    QCOMPARE(result, QStringLiteral("2 seconds"));
}

void AudioOutputTest::_testMillisecondsWithRemainder()
{
    QString result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("1001ms"));
    QCOMPARE(result, QStringLiteral("1 second and 1 millisecond"));

    result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("2500ms"));
    QCOMPARE(result, QStringLiteral("2 seconds and 500 millisecond"));
}

void AudioOutputTest::_testMillisecondsMinutes()
{
    QString result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("60000ms"));
    QCOMPARE(result, QStringLiteral("1 minute"));

    result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("120000ms"));
    QCOMPARE(result, QStringLiteral("2 minutes"));
}

void AudioOutputTest::_testMillisecondsMinutesAndSeconds()
{
    QString result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("90000ms"));
    QCOMPARE(result, QStringLiteral("1 minute and 30 seconds"));

    result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("61000ms"));
    QCOMPARE(result, QStringLiteral("1 minute and 1 second"));
}

// ============================================================================
// Combined Transformation Tests
// ============================================================================

void AudioOutputTest::_testCombinedTransformations()
{
    QString result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("ERR at -10.5m"));
    QCOMPARE(result, QStringLiteral("error at negative 10 point 5 meters"));

    result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("WP 5 altitude -100.5 m"));
    QCOMPARE(result, QStringLiteral("waypoint 5 altitude negative 100 point 5  meters"));
}

// ============================================================================
// Edge Case Tests
// ============================================================================

void AudioOutputTest::_testEmptyString()
{
    QString result = AudioOutput::_fixTextMessageForAudio(QString());
    QVERIFY(result.isEmpty());
}

void AudioOutputTest::_testNoTransformationNeeded()
{
    QString result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("normal text"));
    QCOMPARE(result, QStringLiteral("normal text"));

    result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("altitude 100 feet"));
    QCOMPARE(result, QStringLiteral("altitude 100 feet"));
}

void AudioOutputTest::_testWhitespaceOnly()
{
    QString result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("   "));
    QCOMPARE(result, QStringLiteral("   "));
}
