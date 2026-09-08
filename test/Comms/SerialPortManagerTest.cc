#include "SerialPortManagerTest.h"

#include "SerialPortManager.h"

void SerialPortManagerTest::_exclusiveReservations()
{
    SerialPortManager ports;
    QVERIFY(!ports.reservePort(QString()));
    auto first = ports.reservePort(QStringLiteral(" /test/gps "));
    QVERIFY(first);
    QCOMPARE(first->systemLocation, QStringLiteral("/test/gps"));
    QVERIFY(!ports.reservePort(QStringLiteral("/test/gps")));
    auto copy = first;
    first.reset();
    QVERIFY(ports.isPortReserved(QStringLiteral("/test/gps")));
    auto other = ports.reservePort(QStringLiteral("/test/mavlink"));
    QVERIFY(other);
    copy.reset();
    QVERIFY(ports.canReservePort(QStringLiteral("/test/gps")));
    other.reset();
    QVERIFY(!ports.anyPortReserved());
}

void SerialPortManagerTest::_singlePortInventory()
{
    int scans = 0;
    QList<SerialPortManager::Port> inventory{
        {QStringLiteral("/test/gps"), QStringLiteral("gps"), QGCSerialPortInfo::BoardTypeUnknown, QString()}};
    SerialPortManager ports(nullptr, [&]() {
        ++scans;
        return inventory;
    });
    ports.setSinglePortOnly(true);
    QCOMPARE(ports.availablePorts().size(), 1);
    auto reservation = ports.reservePort(QStringLiteral("/test/gps"));
    QVERIFY(reservation);
    QVERIFY(!ports.canReservePort(QStringLiteral("/test/mavlink")));
    QVERIFY(!ports.reservePort(QStringLiteral("/test/mavlink")));
    inventory.clear();
    QCOMPARE(ports.availablePorts().size(), 1);
    QCOMPARE(scans, 1);
    reservation.reset();
    QTRY_VERIFY_WITH_TIMEOUT(ports.availablePorts().isEmpty(), TestTimeout::mediumMs());
    QCOMPARE(scans, 2);
    QVERIFY(ports.canReservePort(QStringLiteral("/test/mavlink")));
}

UT_REGISTER_TEST(SerialPortManagerTest, TestLabel::Unit)
