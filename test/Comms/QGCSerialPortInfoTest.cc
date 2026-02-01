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

UT_REGISTER_TEST(QGCSerialPortInfoTest, TestLabel::Unit, TestLabel::Comms)
