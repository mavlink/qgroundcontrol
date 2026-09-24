#include "MAVLinkLogManagerTest.h"

#include <QtCore/QFileInfo>
#include <QtCore/QTemporaryDir>

#include "MAVLinkLogManager.h"
#include "MultiVehicleManager.h"
#include "UnitTest.h"
#include "Vehicle.h"

void MAVLinkLogManagerTest::_testInitMAVLinkLogManager_data()
{
    QTest::addColumn<quint32>("systemId");
    QTest::addColumn<QString>("prefix");
    QTest::newRow("legacy") << quint32(1) << QStringLiteral("001-");
    QTest::newRow("wide") << quint32(0x80000000U) << QStringLiteral("2147483648-");
    QTest::newRow("maximum") << quint32(0xffffffffU) << QStringLiteral("4294967295-");
}

void MAVLinkLogManagerTest::_testInitMAVLinkLogManager()
{
    QFETCH(quint32, systemId);
    QFETCH(QString, prefix);
    _connectMockLinkNoInitialConnectSequence();
    MultiVehicleManager* const vehicleMgr = MultiVehicleManager::instance();
    Vehicle* const vehicle = vehicleMgr->activeVehicle();
    QVERIFY(vehicle);
    MAVLinkLogManager* const mavlinkLogManager = new MAVLinkLogManager(vehicle, this);
    QVERIFY(mavlinkLogManager);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    MAVLinkLogProcessor processor;
    QVERIFY(processor.create(mavlinkLogManager, directory.path(), systemId));
    const QFileInfo logFile(processor.fileName());
    QVERIFY(logFile.exists());
    QVERIFY2(logFile.fileName().startsWith(prefix), qPrintable(logFile.fileName()));
}

UT_REGISTER_TEST(MAVLinkLogManagerTest, TestLabel::Integration, TestLabel::Vehicle)
