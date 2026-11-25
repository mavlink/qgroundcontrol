#include "AudioOutputTest.h"

#include "AudioOutput.h"

void AudioOutputTest::_testSpokenReplacements_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expected");

    QTest::addRow("negative decimal meters")
        << "-10.5m, -10.5m. -10.5 m"
        << "negative 10 point 5 meters, negative 10 point 5 meters. negative 10 point 5  meters";
    QTest::addRow("negative meters") << "-10m -10 m" << "negative 10 meters negative 10  meters";
    QTest::addRow("inside a sentence") << "foo -10m -10 m bar" << "foo negative 10 meters negative 10  meters bar";
    QTest::addRow("hyphen before a word") << "-foom" << "-foom";
    // A hyphen joined to a preceding word or number is not a minus sign
    QTest::addRow("hyphen after an acronym") << "GPS-1" << "G.P.S.-1";
    QTest::addRow("hyphen between numbers") << "1-2" << "1-2";
    QTest::addRow("negative in parentheses") << "(-5)" << "(negative 5)";
    QTest::addRow("word starting with m after a number") << "10 moo" << "10 moo";
    QTest::addRow("word starting with m joined to a number") << "10moo" << "10moo";
    QTest::addRow("milliseconds below a second") << "10ms" << "10ms";
    QTest::addRow("one second") << "1000ms" << "1 second";
    QTest::addRow("second and millisecond") << "1001ms" << "1 second and 1 millisecond";
}

void AudioOutputTest::_testSpokenReplacements()
{
    QFETCH(QString, input);
    QFETCH(QString, expected);

    QCOMPARE(AudioOutput::_fixTextMessageForAudio(input), expected);
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

UT_REGISTER_TEST(AudioOutputTest, TestLabel::Unit, TestLabel::Utilities)
