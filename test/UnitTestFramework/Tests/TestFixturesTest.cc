#include "TestFixturesTest.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonObject>
#include <QtCore/QTimer>
#include <QtCore/QUuid>
#include <QtNetwork/QNetworkRequest>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include <cmath>

#include "AppSettings.h"
#include "Fact.h"
#include "Fixtures/TestFixtures.h"
#include "SettingsManager.h"
#include "UnitTest.h"
using namespace TestFixtures;

// ============================================================================
// Helper class for SignalSpyFixture tests
// ============================================================================
class SignalEmitter : public QObject
{
    Q_OBJECT
public:
    explicit SignalEmitter(QObject* parent = nullptr) : QObject(parent)
    {
    }

    void emitValueChanged(int value)
    {
        emit valueChanged(value);
    }

    void emitStateChanged(bool state)
    {
        emit stateChanged(state);
    }

    void emitErrorOccurred(const QString& error)
    {
        emit errorOccurred(error);
    }

signals:
    void valueChanged(int value);
    void stateChanged(bool state);
    void errorOccurred(const QString& error);
};

// ============================================================================
// Coordinate Fixtures Tests
// ============================================================================
void TestFixturesTest::_testCoordOrigin()
{
    const QGeoCoordinate origin = Coord::origin();
    QCOMPARE(origin.latitude(), 0.0);
    QCOMPARE(origin.longitude(), 0.0);
    QCOMPARE(origin.altitude(), 0.0);
    QVERIFY(origin.isValid());
}

void TestFixturesTest::_testCoordStandardLocations()
{
    // Zurich
    const QGeoCoordinate zurich = Coord::zurich();
    QVERIFY(zurich.isValid());
    QVERIFY(zurich.latitude() > 47.0 && zurich.latitude() < 48.0);
    QVERIFY(zurich.longitude() > 8.0 && zurich.longitude() < 9.0);
    // San Francisco
    const QGeoCoordinate sf = Coord::sanFrancisco();
    QVERIFY(sf.isValid());
    QVERIFY(sf.latitude() > 37.0 && sf.latitude() < 38.0);
    QVERIFY(sf.longitude() > -123.0 && sf.longitude() < -122.0);
}

void TestFixturesTest::_testCoordEdgeCases()
{
    // North pole
    const QGeoCoordinate northPole = Coord::northPole();
    QCOMPARE(northPole.latitude(), 90.0);
    QVERIFY(northPole.isValid());
    // South pole
    const QGeoCoordinate southPole = Coord::southPole();
    QCOMPARE(southPole.latitude(), -90.0);
    QVERIFY(southPole.isValid());
    // Date line
    const QGeoCoordinate dateLinePlus = Coord::dateLinePlus();
    QCOMPARE(dateLinePlus.longitude(), 180.0);
    QVERIFY(dateLinePlus.isValid());
    const QGeoCoordinate dateLineMinus = Coord::dateLineMinus();
    QCOMPARE(dateLineMinus.longitude(), -180.0);
    QVERIFY(dateLineMinus.isValid());
}

void TestFixturesTest::_testCoordPolygon()
{
    const QGeoCoordinate center = Coord::zurich();
    // Test triangle
    const QList<QGeoCoordinate> triangle = Coord::polygon(center, 3, 100.0);
    QCOMPARE(triangle.count(), 3);
    for (const QGeoCoordinate& coord : triangle) {
        QVERIFY(coord.isValid());
        // Each vertex should be approximately 100m from center
        const double distance = center.distanceTo(coord);
        QVERIFY(distance > 95.0 && distance < 105.0);
    }
    // Test square
    const QList<QGeoCoordinate> square = Coord::polygon(center, 4, 200.0);
    QCOMPARE(square.count(), 4);
    // Test hexagon
    const QList<QGeoCoordinate> hexagon = Coord::polygon(center, 6, 50.0);
    QCOMPARE(hexagon.count(), 6);
    // Simple shortcuts
    const QList<QGeoCoordinate> simpleSquare = Coord::simpleSquare();
    QCOMPARE(simpleSquare.count(), 4);
    const QList<QGeoCoordinate> simpleTriangle = Coord::simpleTriangle();
    QCOMPARE(simpleTriangle.count(), 3);
}

void TestFixturesTest::_testCoordWaypointPath()
{
    const QGeoCoordinate start = Coord::zurich();
    // Test path generation
    const QList<QGeoCoordinate> path = Coord::waypointPath(start, 5, 100.0, 0.0);
    QCOMPARE(path.count(), 5);
    // First waypoint should be at start
    QVERIFY(start.distanceTo(path[0]) < 1.0);
    // Each subsequent waypoint should be ~100m from previous
    for (int i = 1; i < path.count(); ++i) {
        const double distance = path[i - 1].distanceTo(path[i]);
        QVERIFY2(distance > 95.0 && distance < 105.0,
                 qPrintable(QString("Distance between waypoints %1 and %2 is %3m, expected ~100m")
                                .arg(i - 1)
                                .arg(i)
                                .arg(distance)));
    }
    // Test with different heading
    const QList<QGeoCoordinate> eastPath = Coord::waypointPath(start, 3, 200.0, 90.0);
    QCOMPARE(eastPath.count(), 3);
    // Path should go eastward (longitude increases)
    QVERIFY(eastPath[1].longitude() > eastPath[0].longitude());
    QVERIFY(eastPath[2].longitude() > eastPath[1].longitude());
}

// ============================================================================
// TempFile/TempDir Fixtures Tests
// ============================================================================
void TestFixturesTest::_testTempFileFixture()
{
    QString filePath;
    {
        TempFileFixture tempFile;
        QVERIFY(tempFile.isValid());
        filePath = tempFile.path();
        QVERIFY(!filePath.isEmpty());
        QVERIFY(QFile::exists(filePath));
        // Write and read back
        const QByteArray testData = "Hello, World!";
        QVERIFY(tempFile.write(testData));
        QCOMPARE(tempFile.readAll(), testData);
    }
    // File should be deleted after fixture goes out of scope
    QVERIFY(!QFile::exists(filePath));
}

void TestFixturesTest::_testTempFileFixtureWithTemplate()
{
    TempFileFixture tempFile("testfile_XXXXXX.txt");
    QVERIFY(tempFile.isValid());
    const QString path = tempFile.path();
    QVERIFY(path.contains("testfile_"));
    QVERIFY(path.endsWith(".txt"));
    // Write QString content
    QVERIFY(tempFile.write(QString("Test content")));
    QCOMPARE(tempFile.readAll(), QByteArray("Test content"));
}

void TestFixturesTest::_testTempJsonFileFixture()
{
    TempJsonFileFixture tempJson;
    QVERIFY(tempJson.isValid());
    QVERIFY(tempJson.path().endsWith(".json"));

    const QJsonObject object = {
        {QStringLiteral("name"), QStringLiteral("qgc")},
        {QStringLiteral("value"), 42}
    };
    QVERIFY(tempJson.writeJson(object));

    QJsonParseError error {};
    const QJsonDocument parsed = tempJson.readJson(&error);
    QCOMPARE(error.error, QJsonParseError::NoError);
    QCOMPARE(parsed.object().value(QStringLiteral("name")).toString(), QStringLiteral("qgc"));
    QCOMPARE(parsed.object().value(QStringLiteral("value")).toInt(), 42);
}

void TestFixturesTest::_testTempDirFixture()
{
    QString dirPath;
    {
        TempDirFixture tempDir;
        QVERIFY(tempDir.isValid());
        dirPath = tempDir.path();
        QVERIFY(!dirPath.isEmpty());
        QVERIFY(QDir(dirPath).exists());
    }
    // Directory should be deleted after fixture goes out of scope
    QVERIFY(!QDir(dirPath).exists());
}

void TestFixturesTest::_testTempDirFixtureCreateFile()
{
    TempDirFixture tempDir;
    QVERIFY(tempDir.isValid());
    // Create a file in the temp directory
    const QByteArray content = "File content";
    const QString filePath = tempDir.createFile("subdir/test.txt", content);
    QVERIFY(!filePath.isEmpty());
    QVERIFY(QFile::exists(filePath));
    // Verify content
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), content);
}

void TestFixturesTest::_testNetworkReplyFixture()
{
    const QByteArray body = R"({"ok":true})";
    NetworkReplyFixture reply(QUrl(QStringLiteral("https://example.com/api/v1/source")));
    reply.setHttpStatus(302);
    reply.setRedirectTarget(QUrl(QStringLiteral("../next")));
    reply.setNetworkError(QNetworkReply::ConnectionRefusedError, QStringLiteral("connection refused"));
    reply.setBody(body, QStringLiteral("application/json"));

    QCOMPARE(reply.attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 302);
    QCOMPARE(reply.attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl(), QUrl(QStringLiteral("../next")));
    QCOMPARE(reply.error(), QNetworkReply::ConnectionRefusedError);
    QCOMPARE(reply.errorString(), QStringLiteral("connection refused"));
    QCOMPARE(reply.header(QNetworkRequest::ContentTypeHeader).toString(), QStringLiteral("application/json"));
    QCOMPARE(reply.header(QNetworkRequest::ContentLengthHeader).toLongLong(), static_cast<qint64>(body.size()));
    QCOMPARE(reply.readAll(), body);
}

void TestFixturesTest::_testSingleInstanceLockFixture()
{
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    QSKIP("Single-instance lock fixture is only relevant on desktop targets");
#else
    const QString key = QStringLiteral("SingleInstanceLockFixtureTest_%1")
                            .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));

    SingleInstanceLockFixture primary(key);
    QVERIFY(primary.isLocked());

    SingleInstanceLockFixture secondary(key, false);
    QVERIFY(!secondary.isLocked());
    QVERIFY(!secondary.tryAcquire());

    primary.release();
    QVERIFY(!primary.isLocked());

    QVERIFY(secondary.tryAcquire());
    QVERIFY(secondary.isLocked());
#endif
}

// ============================================================================
// SignalSpyFixture Tests
// ============================================================================
void TestFixturesTest::_testSignalSpyFixtureExpect()
{
    SignalEmitter emitter;
    SignalSpyFixture spy(&emitter);
    spy.expect("valueChanged");
    // Signal not yet emitted - should fail verification
    QVERIFY(!spy.verify());
    // Emit the signal
    emitter.emitValueChanged(42);
    // Now verification should pass
    QVERIFY(spy.verify());
    QVERIFY(spy.wasEmitted("valueChanged"));
}

void TestFixturesTest::_testSignalSpyFixtureExpectExactly()
{
    SignalEmitter emitter;
    SignalSpyFixture spy(&emitter);
    spy.expectExactly("valueChanged", 2);
    // Emit once - should fail (expecting 2)
    emitter.emitValueChanged(1);
    QVERIFY(!spy.verify());
    // Emit twice - should pass
    emitter.emitValueChanged(2);
    QVERIFY(spy.verify());
    // Emit third time - should fail again
    emitter.emitValueChanged(3);
    QVERIFY(!spy.verify());
}

void TestFixturesTest::_testSignalSpyFixtureExpectNot()
{
    SignalEmitter emitter;
    SignalSpyFixture spy(&emitter);
    spy.expectNot("errorOccurred");
    // No error emitted - should pass
    QVERIFY(spy.verify());
    // Emit error - should fail
    emitter.emitErrorOccurred("Test error");
    QVERIFY(!spy.verify());
}

void TestFixturesTest::_testSignalSpyFixtureWaitAndVerify()
{
    SignalEmitter emitter;
    SignalSpyFixture spy(&emitter);
    spy.expect("stateChanged");
    // Schedule signal emission after 50ms
    QTimer::singleShot(50, &emitter, [&emitter]() { emitter.emitStateChanged(true); });
    // Wait and verify - should succeed within timeout
    QVERIFY(spy.waitAndVerify(1000));
}

void TestFixturesTest::_testWaitForSignalCountHelper()
{
    SignalEmitter emitter;
    QSignalSpy spy(&emitter, &SignalEmitter::valueChanged);
    QVERIFY(spy.isValid());

    QTimer::singleShot(20, &emitter, [&emitter]() { emitter.emitValueChanged(1); });
    QTimer::singleShot(40, &emitter, [&emitter]() { emitter.emitValueChanged(2); });

    QVERIFY_SIGNAL_COUNT_WAIT(spy, 2, TestTimeout::mediumMs());
    QCOMPARE(spy.count(), 2);

    QVERIFY(!UnitTest::waitForSignalCount(spy, 3, 100, QStringLiteral("valueChanged")));
}

// ============================================================================
// SettingsFixture Tests
// ============================================================================
void TestFixturesTest::_testSettingsFixtureRestore()
{
    AppSettings* appSettings = SettingsManager::instance()->appSettings();
    Fact* firmwareFact = appSettings->offlineEditingFirmwareClass();
    // Get original value
    const QVariant originalValue = firmwareFact->rawValue();
    {
        SettingsFixture settings;
        // Change firmware type
        settings.setOfflineFirmware(MAV_AUTOPILOT_ARDUPILOTMEGA);
        // Verify it changed
        QVERIFY(firmwareFact->rawValue() != originalValue);
    }
    // After fixture destruction, original value should be restored
    QCOMPARE(firmwareFact->rawValue(), originalValue);
}

void TestFixturesTest::_testSettingsFixtureFactValue()
{
    AppSettings* appSettings = SettingsManager::instance()->appSettings();
    Fact* testFact = appSettings->offlineEditingVehicleClass();
    const QVariant originalValue = testFact->rawValue();
    const QVariant newValue = (originalValue.toInt() + 1) % 5;  // Cycle to a different value
    {
        SettingsFixture settings;
        // Set a fact value
        settings.setFactValue(testFact, newValue);
        // Verify it changed
        QCOMPARE(testFact->rawValue(), newValue);
    }
    // After fixture destruction, original value should be restored
    QCOMPARE(testFact->rawValue(), originalValue);
}

// Required for SignalEmitter Q_OBJECT
#include "TestFixturesTest.moc"

UT_REGISTER_TEST(TestFixturesTest, TestLabel::Unit)
