#include "QGCVersionCheckTest.h"

#include <QtCore/QString>

#include "QGCVersionCheck.h"

void QGCVersionCheckTest::_newerVersionDetected_data()
{
    QTest::addColumn<QString>("current");
    QTest::addColumn<QString>("latest");
    QTest::addColumn<bool>("expected");

    QTest::newRow("newer patch") << "v5.1.2" << "v5.1.3" << true;
    QTest::newRow("newer minor") << "v5.1.9" << "v5.2.0" << true;
    QTest::newRow("newer major") << "v4.9.9" << "v5.0.0" << true;
    QTest::newRow("numeric not lexical") << "v5.9.0" << "v5.10.0" << true;
    QTest::newRow("equal") << "v5.1.2" << "v5.1.2" << false;
    QTest::newRow("older") << "v5.2.0" << "v5.1.9" << false;
    QTest::newRow("suffixed current") << "v5.1.2-45-gabcdef" << "v5.1.3" << true;
    QTest::newRow("unparseable latest") << "v5.1.2" << "garbage" << false;
    QTest::newRow("unparseable current") << "garbage" << "v5.1.3" << false;
    QTest::newRow("missing v prefix") << "5.1.2" << "v5.1.3" << false;
    QTest::newRow("leading garbage") << "v5.1.2" << "xv5.1.3" << false;
    QTest::newRow("leading whitespace") << "v5.1.2" << " v5.1.3" << true;
    QTest::newRow("empty latest") << "v5.1.2" << "" << false;
}

void QGCVersionCheckTest::_newerVersionDetected()
{
    QFETCH(QString, current);
    QFETCH(QString, latest);
    QFETCH(bool, expected);

    QCOMPARE(QGCVersionCheck::isNewerVersion(current, latest), expected);
}

void QGCVersionCheckTest::_notifyOncePerVersion_data()
{
    QTest::addColumn<QString>("latest");
    QTest::addColumn<QString>("lastNotified");
    QTest::addColumn<bool>("expected");

    QTest::newRow("never notified") << "v5.2.0" << "" << true;
    QTest::newRow("already notified") << "v5.2.0" << "v5.2.0" << false;
    QTest::newRow("newer than notified") << "v5.3.0" << "v5.2.0" << true;
    QTest::newRow("older than notified") << "v5.1.0" << "v5.2.0" << false;
    QTest::newRow("corrupt setting") << "v5.2.0" << "junk" << true;
    QTest::newRow("unparseable latest") << "junk" << "" << false;
}

void QGCVersionCheckTest::_notifyOncePerVersion()
{
    QFETCH(QString, latest);
    QFETCH(QString, lastNotified);
    QFETCH(bool, expected);

    QCOMPARE(QGCVersionCheck::shouldNotify(latest, lastNotified), expected);
}

UT_REGISTER_TEST(QGCVersionCheckTest, TestLabel::Unit, TestLabel::Utilities)
