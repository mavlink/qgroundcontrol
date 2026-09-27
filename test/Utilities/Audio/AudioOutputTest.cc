#include "AudioOutputTest.h"

#include "AudioOutput.h"

void AudioOutputTest::_testSpokenReplacements()
{
    QString result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("-10.5m, -10.5m. -10.5 m"));
    QCOMPARE(result,
             QStringLiteral("negative 10 point 5 meters, negative 10 point 5 meters. negative 10 point 5  meters"));
    result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("-10m -10 m"));
    QCOMPARE(result, QStringLiteral("negative 10 meters negative 10  meters"));
    result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("foo -10m -10 m bar"));
    QCOMPARE(result, QStringLiteral("foo negative 10 meters negative 10  meters bar"));
    result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("-foom"));
    QCOMPARE(result, QStringLiteral("-foom"));
    // A hyphen joined to a preceding word or number is not a minus sign
    result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("GPS-1"));
    QCOMPARE(result, QStringLiteral("G.P.S.-1"));
    result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("1-2"));
    QCOMPARE(result, QStringLiteral("1-2"));
    result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("(-5)"));
    QCOMPARE(result, QStringLiteral("(negative 5)"));
    result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("10 moo"));
    QCOMPARE(result, QStringLiteral("10 moo"));
    result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("10moo"));
    QCOMPARE(result, QStringLiteral("10moo"));
    result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("10ms"));
    QCOMPARE(result, QStringLiteral("10ms"));
    result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("1000ms"));
    QCOMPARE(result, QStringLiteral("1 second"));
    result = AudioOutput::_fixTextMessageForAudio(QStringLiteral("1001ms"));
    QCOMPARE(result, QStringLiteral("1 second and 1 millisecond"));
}

// Issue 15110
void AudioOutputTest::_abbreviationsReplacedAtTokenBoundaries_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expected");

    QTest::addRow("acronym spelled") << "GPS lost" << "G.P.S. lost";
    QTest::addRow("trailing colon") << "GPS: no fix" << "G.P.S.: no fix";
    QTest::addRow("parentheses") << "(GNSS)" << "(G.N.S.S.)";
    QTest::addRow("instance suffix") << "EKF3 IMU0 stopped" << "E.K.F. 3 I.M.U. 0 stopped";
    QTest::addRow("digit inside acronym") << "I2C fail" << "I.2.C. fail";
    QTest::addRow("mixed case expansion") << "PreArm: RC not found" << "pre arm: R.C. not found";
    QTest::addRow("expanded words") << "TKOFF CNT" << "takeoff count";
    QTest::addRow("lowercase words untouched") << "you can see rc ins" << "you can see rc ins";
    QTest::addRow("CAN spoken as word") << "CAN1 node" << "CAN1 node";
    QTest::addRow("parameter name untouched") << "GPS_TYPE" << "GPS_TYPE";
    QTest::addRow("embedded in word untouched") << "GPSX lost" << "GPSX lost";
}

void AudioOutputTest::_abbreviationsReplacedAtTokenBoundaries()
{
    QFETCH(QString, input);
    QFETCH(QString, expected);

    QCOMPARE(AudioOutput::_replaceAbbreviations(input), expected);
}

#include "UnitTest.h"

UT_REGISTER_TEST(AudioOutputTest, TestLabel::Unit, TestLabel::Utilities)
