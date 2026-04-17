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

void QGCSerialPortInfoTest::_testBoardClassStringToTypeCaseInsensitivity()
{
    // String-to-type lookup is expected to treat whitespace/case gracefully on
    // the Unknown fallback path: anything that doesn't exactly match a known
    // class string must resolve to BoardTypeUnknown rather than asserting.
    QCOMPARE(QGCSerialPortInfo::_boardClassStringToType(QString()),
             QGCSerialPortInfo::BoardTypeUnknown);
    QCOMPARE(QGCSerialPortInfo::_boardClassStringToType(QStringLiteral(" ")),
             QGCSerialPortInfo::BoardTypeUnknown);
    QCOMPARE(QGCSerialPortInfo::_boardClassStringToType(QStringLiteral("pixhawk")),
             QGCSerialPortInfo::BoardTypeUnknown);  // case-sensitive today
}

void QGCSerialPortInfoTest::_testBoardTypeStringRoundTrip()
{
    // For every known board type, `_boardTypeToString` -> `_boardClassStringToType`
    // must round-trip exactly. This pins the invariant that the two lookup
    // tables stay in sync when either is extended.
    const QGCSerialPortInfo::BoardType_t types[] = {
        QGCSerialPortInfo::BoardTypePixhawk,
        QGCSerialPortInfo::BoardTypeSiKRadio,
        QGCSerialPortInfo::BoardTypeOpenPilot,
        QGCSerialPortInfo::BoardTypeRTKGPS,
    };
    for (QGCSerialPortInfo::BoardType_t t : types) {
        const QString s = QGCSerialPortInfo::_boardTypeToString(t);
        QVERIFY2(!s.isEmpty(), "_boardTypeToString must return non-empty for known types");
        QCOMPARE(QGCSerialPortInfo::_boardClassStringToType(s), t);
    }
}

void QGCSerialPortInfoTest::_testBoardInfoListEntriesAreWellFormed()
{
    QVERIFY(QGCSerialPortInfo::_loadJsonData());

    for (const auto& entry : QGCSerialPortInfo::_boardInfoList) {
        // VID/PID are 16-bit USB identifiers; negative or >0xFFFF would mean
        // the JSON parser produced junk.
        QVERIFY2(entry.vendorId  >= 0 && entry.vendorId  <= 0xFFFF,
                 qPrintable(QStringLiteral("vendorId out of range for %1").arg(entry.name)));
        QVERIFY2(entry.productId >= 0 && entry.productId <= 0xFFFF,
                 qPrintable(QStringLiteral("productId out of range for %1").arg(entry.name)));
        QVERIFY2(!entry.name.isEmpty(), "board entry missing name");
        QVERIFY2(entry.boardType != QGCSerialPortInfo::BoardTypeUnknown,
                 qPrintable(QStringLiteral("board %1 has BoardTypeUnknown class")
                                .arg(entry.name)));
    }
}

void QGCSerialPortInfoTest::_testFallbackRegexesCompile()
{
    QVERIFY(QGCSerialPortInfo::_loadJsonData());

    // Any invalid regex in the JSON would make runtime detection silently
    // fail, so assert compile-time validity here instead.
    for (const auto& entry : QGCSerialPortInfo::_boardDescriptionFallbackList) {
        QVERIFY2(entry.regExp.isValid(),
                 qPrintable(QStringLiteral("invalid description regex: %1")
                                .arg(entry.regExp.pattern())));
    }
    for (const auto& entry : QGCSerialPortInfo::_boardManufacturerFallbackList) {
        QVERIFY2(entry.regExp.isValid(),
                 qPrintable(QStringLiteral("invalid manufacturer regex: %1")
                                .arg(entry.regExp.pattern())));
    }
}

UT_REGISTER_TEST(QGCSerialPortInfoTest, TestLabel::Unit, TestLabel::Comms)
