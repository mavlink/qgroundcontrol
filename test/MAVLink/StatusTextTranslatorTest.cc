#include "StatusTextTranslatorTest.h"

#include <QtCore/QLocale>
#include <QtTest/QTest>

#include "StatusTextTranslator.h"

// The expected values below are corpus literals (src/MAVLink/statustext_zhHans.json).
// They pin the translator algorithm against the production corpus; updating a corpus
// entry requires updating the matching assertion here.

void StatusTextTranslatorTest::init()
{
    StatusTextTranslator::setLocale(QLocale(QLocale::Chinese));
}

void StatusTextTranslatorTest::cleanup()
{
    // Restore the application default: unit tests run with the system locale.
    StatusTextTranslator::setLocale(QLocale::system());
}

void StatusTextTranslatorTest::_testExactMatch()
{
    QCOMPARE(StatusTextTranslator::translate(QStringLiteral("PreArm: Baro: not healthy")),
             QStringLiteral("PreArm: Baro: 气压计不健康"));
    QCOMPARE(StatusTextTranslator::translate(QStringLiteral("PreArm: AHRS: waiting for home")),
             QStringLiteral("PreArm: AHRS: 等待 Home 点"));
    QCOMPARE(StatusTextTranslator::translate(QStringLiteral("PreArm: No SD card")),
             QStringLiteral("PreArm: 未检测到 SD 卡"));
}

void StatusTextTranslatorTest::_testArmingPrefixSwapRestoresRawToken()
{
    // AP_Arming sends the same check failure as "PreArm: X" / "Pre-Arm: X" / "Arm: X".
    // Only "PreArm: Baro: not healthy" is in the corpus; the translator must retry the
    // other prefixes and put the raw prefix token back into the display form.
    QCOMPARE(StatusTextTranslator::translate(QStringLiteral("Arm: Baro: not healthy")),
             QStringLiteral("Arm: Baro: 气压计不健康"));
    QCOMPARE(StatusTextTranslator::translate(QStringLiteral("Pre-Arm: Baro: not healthy")),
             QStringLiteral("Pre-Arm: Baro: 气压计不健康"));
    QCOMPARE(StatusTextTranslator::translate(QStringLiteral("Arm: Logging failed")),
             QStringLiteral("Arm: 日志记录失败"));
}

void StatusTextTranslatorTest::_testRegexWithMultipleCaptureGroups()
{
    // {0} {1} {2} map to capture groups 1 2 3
    QCOMPARE(StatusTextTranslator::translate(
                 QStringLiteral("PreArm: AHRS: EKF3 Roll/Pitch inconsistent 2 deg. Wait or reboot")),
             QStringLiteral("PreArm: AHRS: EKF3 Roll/Pitch 不一致，偏差 2°，请等待或重启"));
    QCOMPARE(StatusTextTranslator::translate(QStringLiteral("Battery 1 is low 3.50V used 300 mAh")),
             QStringLiteral("Battery 1 电量低（3.50V，已用 300 mAh）"));
    QCOMPARE(StatusTextTranslator::translate(QStringLiteral("GPS 1: detected u-blox M9N")),
             QStringLiteral("GPS 1: 已检测到 u-blox M9N"));
}

void StatusTextTranslatorTest::_testSpecificEntriesBeatGenericWrapper()
{
    // The specific "EKF3 core" regex must win over the generic "^PreArm: AHRS: (.+)$"
    // wrapper which would only echo the body back, so it is ordered first in the corpus.
    QCOMPARE(StatusTextTranslator::translate(QStringLiteral("PreArm: AHRS: EKF3 core 0 unhealthy")),
             QStringLiteral("PreArm: AHRS: EKF3 核心 0 不健康"));
}

void StatusTextTranslatorTest::_testUnknownTextPassesThrough()
{
    QCOMPARE(StatusTextTranslator::translate(QStringLiteral("Totally unknown message 42")),
             QStringLiteral("Totally unknown message 42"));
    // The generic AHRS wrapper matches but leaves the unmatched body untouched.
    QCOMPARE(StatusTextTranslator::translate(QStringLiteral("PreArm: AHRS: custom wait message")),
             QStringLiteral("PreArm: AHRS: custom wait message"));
}

void StatusTextTranslatorTest::_testNonChineseLocalePassesThrough()
{
    StatusTextTranslator::setLocale(QLocale(QLocale::English));
    QCOMPARE(StatusTextTranslator::translate(QStringLiteral("PreArm: Baro: not healthy")),
             QStringLiteral("PreArm: Baro: not healthy"));
    QCOMPARE(StatusTextTranslator::translate(QStringLiteral("Battery 1 is low 3.50V used 300 mAh")),
             QStringLiteral("Battery 1 is low 3.50V used 300 mAh"));
    QCOMPARE(StatusTextTranslator::translate(QStringLiteral("Arm: Baro: not healthy")),
             QStringLiteral("Arm: Baro: not healthy"));
}

void StatusTextTranslatorTest::_testEmptyTextPassesThrough()
{
    QCOMPARE(StatusTextTranslator::translate(QString()), QString());
}

UT_REGISTER_TEST(StatusTextTranslatorTest, TestLabel::Unit)
