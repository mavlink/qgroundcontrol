#include "AudioOutputTest.h"

#include "AudioOutput.h"

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QScopeGuard>
#include <QtCore/QTemporaryDir>

#include "PiperTtsEngine.h"
#endif

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

void AudioOutputTest::_piperAvailabilityRunsNoCandidate()
{
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    // A candidate that records being run, and a voice, so that Piper counts as installed.
    const QString marker = dir.filePath(QStringLiteral("ran"));
    QFile candidate(dir.filePath(QStringLiteral("piper-tts")));
    QVERIFY(candidate.open(QIODevice::WriteOnly));
    (void) candidate.write("#!/bin/sh\ntouch \"" + marker.toUtf8() + "\"\necho --model\n");
    candidate.close();
    QVERIFY(candidate.setPermissions(candidate.permissions() | QFileDevice::ExeOwner));

    QVERIFY(QDir(dir.path()).mkdir(QStringLiteral("piper-voices")));
    QFile model(dir.filePath(QStringLiteral("piper-voices/test.onnx")));
    QVERIFY(model.open(QIODevice::WriteOnly));
    model.close();
    QFile config(model.fileName() + QStringLiteral(".json"));
    QVERIFY(config.open(QIODevice::WriteOnly));
    (void) config.write(R"({"language": {"code": "en_US"}})");
    config.close();

    const QByteArray path = qgetenv("PATH");
    const bool dataDirsSet = qEnvironmentVariableIsSet("XDG_DATA_DIRS");
    const QByteArray dataDirs = qgetenv("XDG_DATA_DIRS");
    const auto restoreEnvironment = qScopeGuard([&]() {
        (void) qputenv("PATH", path);
        if (dataDirsSet) {
            (void) qputenv("XDG_DATA_DIRS", dataDirs);
        } else {
            (void) qunsetenv("XDG_DATA_DIRS");
        }
    });
    QVERIFY(qputenv("PATH", dir.path().toUtf8() + ':' + path));
    QVERIFY(qputenv("XDG_DATA_DIRS", dir.path().toUtf8()));

    QVERIFY(PiperTtsEngine::isAvailable());
    QVERIFY2(!QFile::exists(marker), "Checking for Piper while the application starts must not run it");
#else
    QSKIP("Piper text-to-speech is only built on Linux");
#endif
}

#include "UnitTest.h"

UT_REGISTER_TEST(AudioOutputTest, TestLabel::Unit, TestLabel::Utilities)
