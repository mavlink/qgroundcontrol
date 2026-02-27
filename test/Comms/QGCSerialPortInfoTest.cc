#include "QGCSerialPortInfoTest.h"

#include "QGCSerialPortInfo.h"

void QGCSerialPortInfoTest::_testLoadJsonData()
{
    QVERIFY(!QGCSerialPortInfo::_jsonLoaded);
    QVERIFY(QGCSerialPortInfo::_loadJsonData());
    QVERIFY(QGCSerialPortInfo::_jsonLoaded);
    QVERIFY(QGCSerialPortInfo::_jsonDataValid);
    QVERIFY(!QGCSerialPortInfo::_boardInfoList.isEmpty());
    QVERIFY(!QGCSerialPortInfo::_boardDescriptionFallbackList.isEmpty());
    QVERIFY(!QGCSerialPortInfo::_boardManufacturerFallbackList.isEmpty());
}

void QGCSerialPortInfoTest::_testLoadJsonDataIdempotent()
{
    QVERIFY(QGCSerialPortInfo::_loadJsonData());
    const int boardCount = QGCSerialPortInfo::_boardInfoList.count();
    const int descriptionFallbackCount = QGCSerialPortInfo::_boardDescriptionFallbackList.count();
    const int manufacturerFallbackCount = QGCSerialPortInfo::_boardManufacturerFallbackList.count();

    QVERIFY(QGCSerialPortInfo::_loadJsonData());
    QCOMPARE(QGCSerialPortInfo::_boardInfoList.count(), boardCount);
    QCOMPARE(QGCSerialPortInfo::_boardDescriptionFallbackList.count(), descriptionFallbackCount);
    QCOMPARE(QGCSerialPortInfo::_boardManufacturerFallbackList.count(), manufacturerFallbackCount);
}

void QGCSerialPortInfoTest::_testBoardClassStringToType()
{
    QCOMPARE(QGCSerialPortInfo::_boardClassStringToType(QStringLiteral("Pixhawk")),
             QGCSerialPortInfo::BoardTypePixhawk);
    QCOMPARE(QGCSerialPortInfo::_boardClassStringToType(QStringLiteral("SiK Radio")),
             QGCSerialPortInfo::BoardTypeSiKRadio);
    QCOMPARE(QGCSerialPortInfo::_boardClassStringToType(QStringLiteral("OpenPilot")),
             QGCSerialPortInfo::BoardTypeOpenPilot);
    QCOMPARE(QGCSerialPortInfo::_boardClassStringToType(QStringLiteral("RTK GPS")),
             QGCSerialPortInfo::BoardTypeRTKGPS);
    QCOMPARE(QGCSerialPortInfo::_boardClassStringToType(QStringLiteral("UnknownClass")),
             QGCSerialPortInfo::BoardTypeUnknown);
}

void QGCSerialPortInfoTest::_testBoardTypeToString()
{
    QCOMPARE(QGCSerialPortInfo::_boardTypeToString(QGCSerialPortInfo::BoardTypePixhawk),
             QStringLiteral("Pixhawk"));
    QCOMPARE(QGCSerialPortInfo::_boardTypeToString(QGCSerialPortInfo::BoardTypeSiKRadio),
             QStringLiteral("SiK Radio"));
    QCOMPARE(QGCSerialPortInfo::_boardTypeToString(QGCSerialPortInfo::BoardTypeOpenPilot),
             QStringLiteral("OpenPilot"));
    QCOMPARE(QGCSerialPortInfo::_boardTypeToString(QGCSerialPortInfo::BoardTypeRTKGPS),
             QStringLiteral("RTK GPS"));
    QCOMPARE(QGCSerialPortInfo::_boardTypeToString(QGCSerialPortInfo::BoardTypeUnknown),
             QStringLiteral("Unknown"));
}

UT_REGISTER_TEST(QGCSerialPortInfoTest, TestLabel::Unit, TestLabel::Comms)
