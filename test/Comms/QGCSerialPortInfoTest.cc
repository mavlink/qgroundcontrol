#include "QGCSerialPortInfoTest.h"
#include "QGCSerialPortInfo.h"

#include <QtTest/QTest>

// ============================================================================
// JSON Data Loading Tests
// ============================================================================

void QGCSerialPortInfoTest::_testLoadJsonData()
{
    // Reset state to force reload
    QGCSerialPortInfo::_jsonLoaded = false;
    QGCSerialPortInfo::_jsonDataValid = false;
    QGCSerialPortInfo::_boardInfoList.clear();
    QGCSerialPortInfo::_boardDescriptionFallbackList.clear();
    QGCSerialPortInfo::_boardManufacturerFallbackList.clear();

    QCOMPARE_EQ(QGCSerialPortInfo::_jsonLoaded, false);
    QVERIFY(QGCSerialPortInfo::_loadJsonData());
    QCOMPARE_EQ(QGCSerialPortInfo::_jsonLoaded, true);
    QCOMPARE_EQ(QGCSerialPortInfo::_jsonDataValid, true);
}

void QGCSerialPortInfoTest::_testJsonDataStructure()
{
    // Ensure JSON is loaded
    if (!QGCSerialPortInfo::_jsonLoaded) {
        QVERIFY(QGCSerialPortInfo::_loadJsonData());
    }

    // Verify data structures are populated
    QGC_VERIFY_NOT_EMPTY(QGCSerialPortInfo::_boardInfoList);
    QGC_VERIFY_NOT_EMPTY(QGCSerialPortInfo::_boardDescriptionFallbackList);
    QGC_VERIFY_NOT_EMPTY(QGCSerialPortInfo::_boardManufacturerFallbackList);

    // Verify board info entries have valid data
    for (const auto& info : QGCSerialPortInfo::_boardInfoList) {
        QCOMPARE_GE(info.vendorId, 0);
        QCOMPARE_GE(info.productId, 0);
        QVERIFY(!info.name.isEmpty());
    }

    // Verify fallback entries have valid regex patterns
    for (const auto& fallback : QGCSerialPortInfo::_boardDescriptionFallbackList) {
        QVERIFY(!fallback.regExp.isEmpty());
    }

    for (const auto& fallback : QGCSerialPortInfo::_boardManufacturerFallbackList) {
        QVERIFY(!fallback.regExp.isEmpty());
    }
}

// ============================================================================
// Board Type Conversion Tests
// ============================================================================

void QGCSerialPortInfoTest::_testBoardClassStringToType()
{
    // Test known board class strings
    QCOMPARE_EQ(QGCSerialPortInfo::_boardClassStringToType("Pixhawk"),
                QGCSerialPortInfo::BoardTypePixhawk);
    QCOMPARE_EQ(QGCSerialPortInfo::_boardClassStringToType("SiK Radio"),
                QGCSerialPortInfo::BoardTypeSiKRadio);
    QCOMPARE_EQ(QGCSerialPortInfo::_boardClassStringToType("OpenPilot"),
                QGCSerialPortInfo::BoardTypeOpenPilot);
    QCOMPARE_EQ(QGCSerialPortInfo::_boardClassStringToType("RTK GPS"),
                QGCSerialPortInfo::BoardTypeRTKGPS);

    // Test unknown string
    QCOMPARE_EQ(QGCSerialPortInfo::_boardClassStringToType("Unknown Board"),
                QGCSerialPortInfo::BoardTypeUnknown);
    QCOMPARE_EQ(QGCSerialPortInfo::_boardClassStringToType(""),
                QGCSerialPortInfo::BoardTypeUnknown);
}

void QGCSerialPortInfoTest::_testBoardTypeToString()
{
    // Test known board types
    QCOMPARE_EQ(QGCSerialPortInfo::_boardTypeToString(QGCSerialPortInfo::BoardTypePixhawk),
                QStringLiteral("Pixhawk"));
    QCOMPARE_EQ(QGCSerialPortInfo::_boardTypeToString(QGCSerialPortInfo::BoardTypeSiKRadio),
                QStringLiteral("SiK Radio"));
    QCOMPARE_EQ(QGCSerialPortInfo::_boardTypeToString(QGCSerialPortInfo::BoardTypeOpenPilot),
                QStringLiteral("OpenPilot"));
    QCOMPARE_EQ(QGCSerialPortInfo::_boardTypeToString(QGCSerialPortInfo::BoardTypeRTKGPS),
                QStringLiteral("RTK GPS"));
    QCOMPARE_EQ(QGCSerialPortInfo::_boardTypeToString(QGCSerialPortInfo::BoardTypeUnknown),
                QStringLiteral("Unknown"));
}

// ============================================================================
// System Port Detection Tests
// ============================================================================

void QGCSerialPortInfoTest::_testIsSystemPortKnownPorts()
{
    // We can only test the static method signature works
    // Actual system port detection depends on available hardware

    // Create a default QSerialPortInfo and verify the method doesn't crash
    QSerialPortInfo emptyInfo;
    // This should return false for an empty/invalid port info
    // (the actual result depends on platform)
    const bool result = QGCSerialPortInfo::isSystemPort(emptyInfo);
    Q_UNUSED(result)
    // If we reach here, the function executed successfully
}

// ============================================================================
// Available Ports Tests
// ============================================================================

void QGCSerialPortInfoTest::_testAvailablePortsReturnsValidList()
{
    // Get available ports - list may be empty if no serial devices connected
    const QList<QGCSerialPortInfo> ports = QGCSerialPortInfo::availablePorts();

    // Verify each port in the list has a valid name
    for (const QGCSerialPortInfo& port : ports) {
        QVERIFY2(!port.portName().isEmpty(), "Port name should not be empty");
    }
    // If we reach here, all ports (if any) have valid names
}
