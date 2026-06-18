#include "QGCSerialPortInfoTest.h"

#include "QGCSerialPortInfo.h"

#include <QtTest/QTest>

#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonParseError>
#include <QtCore/QJsonValue>

void QGCSerialPortInfoTest::_testLoadJsonData()
{
    const QGCSerialPortInfo::BoardDatabase &db = QGCSerialPortInfo::_boardDatabase();
    QVERIFY(db.valid);
    QVERIFY(!db.boardInfo.isEmpty());
    QVERIFY(!db.descriptionFallback.isEmpty());
    QVERIFY(!db.manufacturerFallback.isEmpty());
}

void QGCSerialPortInfoTest::_testLoadJsonDataIdempotent()
{
    const QGCSerialPortInfo::BoardDatabase &first = QGCSerialPortInfo::_boardDatabase();
    const QGCSerialPortInfo::BoardDatabase &second = QGCSerialPortInfo::_boardDatabase();
    QCOMPARE(&first, &second);  // magic-static parses exactly once
    QCOMPARE(second.boardInfo.count(), first.boardInfo.count());
    QCOMPARE(second.descriptionFallback.count(), first.descriptionFallback.count());
    QCOMPARE(second.manufacturerFallback.count(), first.manufacturerFallback.count());
}

void QGCSerialPortInfoTest::_testBoardTypeStringMapping_data()
{
    QTest::addColumn<QGCSerialPortInfo::BoardType_t>("type");
    QTest::addColumn<QString>("string");
    QTest::addColumn<bool>("roundTrips");

    QTest::newRow("Pixhawk")   << QGCSerialPortInfo::BoardTypePixhawk   << QStringLiteral("Pixhawk")   << true;
    QTest::newRow("SiK Radio") << QGCSerialPortInfo::BoardTypeSiKRadio  << QStringLiteral("SiK Radio") << true;
    QTest::newRow("OpenPilot") << QGCSerialPortInfo::BoardTypeOpenPilot << QStringLiteral("OpenPilot") << true;
    QTest::newRow("RTK GPS")   << QGCSerialPortInfo::BoardTypeRTKGPS    << QStringLiteral("RTK GPS")   << true;
    // BoardTypeUnknown stringifies to "Unknown", but no class string parses back to it.
    QTest::newRow("Unknown")   << QGCSerialPortInfo::BoardTypeUnknown   << QStringLiteral("Unknown")   << false;
}

void QGCSerialPortInfoTest::_testBoardTypeStringMapping()
{
    QFETCH(const QGCSerialPortInfo::BoardType_t, type);
    QFETCH(const QString, string);
    QFETCH(const bool, roundTrips);

    QCOMPARE(QGCSerialPortInfo::_boardTypeToString(type), string);
    if (roundTrips) {
        QCOMPARE(QGCSerialPortInfo::_boardClassStringToType(string), type);
    }
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

void QGCSerialPortInfoTest::_testBoardInfoListEntriesAreWellFormed()
{
    const QGCSerialPortInfo::BoardDatabase &db = QGCSerialPortInfo::_boardDatabase();
    QVERIFY(db.valid);

    for (const auto& entry : db.boardInfo) {
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
    const QGCSerialPortInfo::BoardDatabase &db = QGCSerialPortInfo::_boardDatabase();
    QVERIFY(db.valid);

    // Any invalid regex in the JSON would make runtime detection silently
    // fail, so assert compile-time validity here instead.
    for (const auto& entry : db.descriptionFallback) {
        QVERIFY2(entry.regExp.isValid(),
                 qPrintable(QStringLiteral("invalid description regex: %1")
                                .arg(entry.regExp.pattern())));
    }
    for (const auto& entry : db.manufacturerFallback) {
        QVERIFY2(entry.regExp.isValid(),
                 qPrintable(QStringLiteral("invalid manufacturer regex: %1")
                                .arg(entry.regExp.pattern())));
    }
}


void QGCSerialPortInfoTest::_testFallbackSchemaIsPlatformNeutral()
{
    QFile file(QStringLiteral(":/json/USBBoardInfo.json"));
    QVERIFY(file.open(QIODevice::ReadOnly));

    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    QCOMPARE(error.error, QJsonParseError::NoError);

    const auto assertNoPlatformKeys = [](const QJsonArray &fallbacks) {
        for (const QJsonValue &value : fallbacks) {
            QVERIFY(value.isObject());
            const QJsonObject fallback = value.toObject();
            QVERIFY2(!fallback.contains(QStringLiteral("androidOnly")),
                     qPrintable(QStringLiteral("platform-specific key in fallback regex: %1")
                                    .arg(fallback[QStringLiteral("regExp")].toString())));
        }
    };

    const QJsonObject root = document.object();
    assertNoPlatformKeys(root[QStringLiteral("boardDescriptionFallback")].toArray());
    assertNoPlatformKeys(root[QStringLiteral("boardManufacturerFallback")].toArray());
}

void QGCSerialPortInfoTest::_testLinuxSystemPortFiltering()
{
#ifdef Q_OS_LINUX
    const auto makePort = [](const QString &systemLocation) {
        QGCSerialPortInfo::Data data;
        data.portName = systemLocation.section(QLatin1Char('/'), -1);
        data.systemLocation = systemLocation;
        return QGCSerialPortInfo(std::move(data));
    };

    QVERIFY(QGCSerialPortInfo::isSystemPort(makePort(QStringLiteral("/dev/ttyS0"))));
    QVERIFY(QGCSerialPortInfo::isSystemPort(makePort(QStringLiteral("/dev/rfcomm0"))));
    QVERIFY(QGCSerialPortInfo::isSystemPort(makePort(QStringLiteral("/dev/ttyACM0"))));
    QVERIFY(!QGCSerialPortInfo::isSystemPort(makePort(QStringLiteral("/dev/ttyUSB0"))));

    QGCSerialPortInfo::Data usbBoard;
    usbBoard.portName = QStringLiteral("ttyACM0");
    usbBoard.systemLocation = QStringLiteral("/dev/ttyACM0");
    usbBoard.vendorIdentifier = 9900;
    usbBoard.productIdentifier = 17;
    usbBoard.hasVendorIdentifier = true;
    usbBoard.hasProductIdentifier = true;
    QVERIFY(!QGCSerialPortInfo::isSystemPort(QGCSerialPortInfo(std::move(usbBoard))));
#endif
}

UT_REGISTER_TEST(QGCSerialPortInfoTest, TestLabel::Unit, TestLabel::Comms)
