#include "SigningStatusTest.h"

#include <QtCore/QMetaEnum>
#include <QtCore/QMetaType>
#include <QtTest/QTest>

#include "SigningStatus.h"

void SigningStatusTest::_testDefaults()
{
    const SigningStatus s;
    QCOMPARE(s.state, SigningStatus::State::Off);
    QVERIFY(!s.enabled);
    QVERIFY(s.keyName.isEmpty());
    QVERIFY(s.statusText.isEmpty());
    QCOMPARE(s.streamCount, 0);
    QVERIFY(!s.pending());
}

void SigningStatusTest::_testPendingDerivation()
{
    SigningStatus s;
    s.state = SigningStatus::State::Off;
    QVERIFY(!s.pending());
    s.state = SigningStatus::State::On;
    QVERIFY(!s.pending());
    s.state = SigningStatus::State::Enabling;
    QVERIFY(s.pending());
    s.state = SigningStatus::State::Disabling;
    QVERIFY(s.pending());
}

void SigningStatusTest::_testEquality()
{
    SigningStatus a;
    SigningStatus b;
    QCOMPARE(a, b);

    b.state = SigningStatus::State::On;
    QVERIFY(a != b);

    a = b;
    QCOMPARE(a, b);

    a.keyName = QStringLiteral("k");
    QVERIFY(a != b);
}

void SigningStatusTest::_testEnumValues()
{
    const QMetaEnum me = QMetaEnum::fromType<SigningStatus::State>();
    QCOMPARE(me.keyCount(), 4);
    QCOMPARE(QString::fromLatin1(me.valueToKey(static_cast<int>(SigningStatus::State::Off))), QStringLiteral("Off"));
    QCOMPARE(QString::fromLatin1(me.valueToKey(static_cast<int>(SigningStatus::State::Enabling))),
             QStringLiteral("Enabling"));
    QCOMPARE(QString::fromLatin1(me.valueToKey(static_cast<int>(SigningStatus::State::On))), QStringLiteral("On"));
    QCOMPARE(QString::fromLatin1(me.valueToKey(static_cast<int>(SigningStatus::State::Disabling))),
             QStringLiteral("Disabling"));
}

void SigningStatusTest::_testRegisteredAsMetaType()
{
    QVERIFY(QMetaType::fromType<SigningStatus>().isValid());
    const SigningStatus s{SigningStatus::State::On, true, QStringLiteral("k"), QStringLiteral("On"), 2};
    const QVariant v = QVariant::fromValue(s);
    QVERIFY(v.canConvert<SigningStatus>());
    const SigningStatus rt = v.value<SigningStatus>();
    QCOMPARE(rt, s);
}

UT_REGISTER_TEST(SigningStatusTest, TestLabel::Unit)
