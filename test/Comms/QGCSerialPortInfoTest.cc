/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGCSerialPortInfoTest.h"
#include "QGCSerialPortInfo.h"

#include <QtTest/QTest>

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
