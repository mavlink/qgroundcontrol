#include "FirmwarePluginTest.h"

#include <QtCore/QString>

#include "FirmwarePlugin.h"

void FirmwarePluginTest::_stableFirmwareNotifiedOncePerVersion_data()
{
    QTest::addColumn<QString>("latest");
    QTest::addColumn<QString>("seen");
    QTest::addColumn<bool>("expected");

    QTest::newRow("never seen") << "4.6.2" << "" << true;
    QTest::newRow("already seen") << "4.6.2" << "4.6.2" << false;
    QTest::newRow("newer than seen") << "4.6.3" << "4.6.2" << true;
    QTest::newRow("older than seen") << "4.6.1" << "4.6.2" << false;
    QTest::newRow("corrupt setting") << "4.6.2" << "junk" << true;
    QTest::newRow("unparseable latest") << "junk" << "" << false;
}

void FirmwarePluginTest::_stableFirmwareNotifiedOncePerVersion()
{
    QFETCH(QString, latest);
    QFETCH(QString, seen);
    QFETCH(bool, expected);

    QCOMPARE(FirmwarePlugin::isStableFirmwareVersionUnseen(latest, seen), expected);
}

UT_REGISTER_TEST(FirmwarePluginTest, TestLabel::Unit, TestLabel::Vehicle)
