#include "VehicleCameraControlTest.h"

#include <cstring>
#include <memory>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QScopeGuard>
#include <QtCore/QTemporaryDir>
#include <QtCore/QThreadPool>
#include <QtNetwork/QNetworkReply>
#include <QtTest/QSignalSpy>

#include "AppSettings.h"
#include "Compression.h"
#include "FileFixtures.h"
#include "LinkManager.h"
#include "LocalHttpTestServer.h"
#include "MavlinkCameraControlInterface.h"
#include "MockConfiguration.h"
#include "MockLink.h"
#include "MockLinkFTP.h"
#include "MultiVehicleManager.h"
#include "ProcessFixtures.h"
#include "QGCCameraManager.h"
#include "QGCVideoStreamInfo.h"
#include "SettingsManager.h"
#include "Vehicle.h"
#include "VehicleCameraControl.h"

namespace {

class CameraControlProbe : public VehicleCameraControl
{
public:
    using VehicleCameraControl::VehicleCameraControl;

    QString cacheFile() const { return _cacheFile; }

    bool paramComplete() const { return _paramComplete; }

    bool cached() const { return _cached; }

    void loadDefinition(const QByteArray& definition) { _dataReady(definition); }

    void setMaxDefinitionBytes(qint64 maxBytes) { _maxDefinitionBytes = maxBytes; }
};

MockConfiguration* newMockConfiguration(const QString& name)
{
    auto* mockConfig = new MockConfiguration(name);
    mockConfig->setFirmwareType(MAV_AUTOPILOT_PX4);
    mockConfig->setVehicleType(MAV_TYPE_QUADROTOR);
    mockConfig->setDynamic(true);
    return mockConfig;
}

/// A camera definition with one setting, named @p settingName
QByteArray cameraDefinition(const char* settingName)
{
    return QByteArray(R"(<?xml version="1.0" encoding="UTF-8"?>)"
                      R"(<mavlinkcamera><definition version="1"><model>Cam</model><vendor>Acme</vendor></definition>)"
                      R"(<parameters><parameter name=")") +
           settingName +
           R"(" type="uint32" default="0"><description>Setting</description></parameter></parameters>)"
           R"(</mavlinkcamera>)";
}

/// Points the app save path at rootPath until the returned guard goes out of scope.
[[nodiscard]] auto overrideSavePath(const QString& rootPath)
{
    Fact* savePathFact = SettingsManager::instance()->appSettings()->savePath();
    const QVariant originalSavePath = savePathFact->rawValue();
    savePathFact->setRawValue(rootPath);
    return qScopeGuard([savePathFact, originalSavePath]() { savePathFact->setRawValue(originalSavePath); });
}

}  // namespace

void VehicleCameraControlTest::_testNameFieldReadIsBounded()
{
    // CAMERA_INFORMATION.vendor_name and model_name are uint8_t[32] filled straight from
    // the wire. MAVLink does not guarantee a NUL, so reading them as C strings runs past
    // the array into whatever follows it in the struct.
    struct
    {
        uint8_t vendor[32];
        uint8_t model[32];
    } wire{};

    memset(wire.vendor, 'A', sizeof(wire.vendor));
    memcpy(wire.model, "SHOULD_NOT_APPEAR", 18);

    const QString bounded = QGCMAVLink::boundedStringField(wire.vendor);

    QCOMPARE(bounded.length(), 32);
    QVERIFY(!bounded.contains(QLatin1String("SHOULD_NOT_APPEAR")));

    // A short, properly terminated field must still read normally.
    memset(&wire, 0, sizeof(wire));
    memcpy(wire.vendor, "Sony", 5);
    QCOMPARE(QGCMAVLink::boundedStringField(wire.vendor), QStringLiteral("Sony"));

    // QGC has always decoded these names as UTF-8, so a non-ASCII name must not turn into Latin-1 mojibake.
    static constexpr char utf8Name[] = "C\xC3\xA1mara";
    memset(&wire, 0, sizeof(wire));
    memcpy(wire.vendor, utf8Name, sizeof(utf8Name));
    QCOMPARE(QGCMAVLink::boundedStringField(wire.vendor), QStringLiteral("C\u00e1mara"));
}

void VehicleCameraControlTest::_testCameraNamesCannotSteerCachePath()
{
    // The vendor/model strings are formatted into the camera-definition cache file name.
    // Whatever they contain, the result has to stay a single component inside the cache
    // directory.
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString base = QDir::cleanPath(QDir(tempDir.path()).absolutePath());

    const QList<QPair<QString, QString>> hostile = {
        {QStringLiteral("../../../Desktop/QGC01_PWNED"), QStringLiteral("marker")},
        {QStringLiteral("..\\..\\..\\Desktop\\PWNED"), QStringLiteral("marker")},
        {QStringLiteral("C:\\Windows\\Temp\\PWNED"), QStringLiteral("marker")},
        {QStringLiteral("/etc/cron.d/PWNED"), QStringLiteral("marker")},
        {QStringLiteral("T.txt:"), QStringLiteral("s")},
        {QStringLiteral("vendor"), QStringLiteral("../../evil")},
        {QStringLiteral(".."), QStringLiteral("..")},
        {QString(), QStringLiteral("marker")},
    };

    for (const auto& testCase : hostile) {
        const QString token = VehicleCameraControl::cacheFileToken(testCase.first, testCase.second, 1);
        const QString candidateFilePath = QDir(base).filePath(token + QStringLiteral(".xml"));

        QVERIFY2(VehicleCameraControl::pathIsInside(base, candidateFilePath), qPrintable(candidateFilePath));

        // It must be one component: no separator, and no drive or NTFS stream qualifier.
        QVERIFY2(!token.contains(QLatin1Char('/')), qPrintable(token));
        QVERIFY2(!token.contains(QLatin1Char('\\')), qPrintable(token));
        QVERIFY2(!token.contains(QLatin1Char(':')), qPrintable(token));
    }

    // An unset cache directory must be refused rather than becoming a write at "/".
    QVERIFY(!VehicleCameraControl::pathIsInside(QString(), QStringLiteral("/v_m_001.xml")));
}

void VehicleCameraControlTest::_testOrdinaryCameraNamesUnchanged()
{
    // Containment must not mangle a real camera's cache file name beyond replacing
    // characters that cannot appear in one.
    QCOMPARE(VehicleCameraControl::pathSafeNameToken(QStringLiteral("Sony")), QStringLiteral("Sony"));
    QCOMPARE(VehicleCameraControl::pathSafeNameToken(QStringLiteral("ILCE-7RM4")), QStringLiteral("ILCE-7RM4"));
    QCOMPARE(VehicleCameraControl::pathSafeNameToken(QStringLiteral("Skynode.v2")), QStringLiteral("Skynode.v2"));
    QCOMPARE(VehicleCameraControl::pathSafeNameToken(QStringLiteral("FLIR Boson")), QStringLiteral("FLIR_Boson"));
    QCOMPARE(VehicleCameraControl::pathSafeNameToken(QString()), QStringLiteral("unknown"));

    const QString token = VehicleCameraControl::cacheFileToken(QStringLiteral("Sony"), QStringLiteral("ILCE-7RM4"), 7);
    QVERIFY2(token.startsWith(QStringLiteral("Sony_ILCE-7RM4_")), qPrintable(token));
    QVERIFY2(token.endsWith(QStringLiteral("_007")), qPrintable(token));
    QCOMPARE(token, VehicleCameraControl::cacheFileToken(QStringLiteral("Sony"), QStringLiteral("ILCE-7RM4"), 7));
}

void VehicleCameraControlTest::_testDistinctCameraNamesDoNotShareCacheFile()
{
    // pathSafeNameToken() is lossy; distinct cameras must still get distinct cache files.
    const QList<QPair<QPair<QString, QString>, QPair<QString, QString>>> collidingAfterSanitising = {
        {{QStringLiteral("FLIR"), QStringLiteral("Boson 640")}, {QStringLiteral("FLIR"), QStringLiteral("Boson_640")}},
        {{QStringLiteral("FLIR"), QStringLiteral("Boson/640")}, {QStringLiteral("FLIR"), QStringLiteral("Boson:640")}},
        {{QStringLiteral("\u5927\u7586"), QStringLiteral("Cam")},
         {QStringLiteral("\u6d77\u5eb7"), QStringLiteral("Cam")}},
        {{QStringLiteral(".Cam"), QStringLiteral("X")}, {QStringLiteral("Cam"), QStringLiteral("X")}},
        {{QStringLiteral("A_B"), QStringLiteral("C")}, {QStringLiteral("A"), QStringLiteral("B_C")}},
        {{QStringLiteral("Sony"), QStringLiteral("X")}, {QStringLiteral("sony"), QStringLiteral("X")}},
    };

    for (const auto& pair : collidingAfterSanitising) {
        const QString first = VehicleCameraControl::cacheFileToken(pair.first.first, pair.first.second, 1);
        const QString second = VehicleCameraControl::cacheFileToken(pair.second.first, pair.second.second, 1);
        QVERIFY2(first.compare(second, Qt::CaseInsensitive) != 0, qPrintable(first + QStringLiteral(" == ") + second));
    }
}

void VehicleCameraControlTest::_connectVehicle(MockConfiguration* mockConfig)
{
    QSignalSpy spyVehicle(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);
    QVERIFY(spyVehicle.isValid());

    SharedLinkConfigurationPtr linkConfig = LinkManager::instance()->addConfiguration(mockConfig);
    QVERIFY(LinkManager::instance()->createConnectedLink(linkConfig));
    QVERIFY2(UnitTest::waitForSignal(spyVehicle, TestTimeout::longMs(), QStringLiteral("activeVehicleChanged")),
             "Timeout waiting for vehicle connection");

    _vehicle = MultiVehicleManager::instance()->activeVehicle();
    QVERIFY(_vehicle);
    _mockLink = qobject_cast<MockLink*>(linkConfig->link());
    QVERIFY(_mockLink);

    if (!_vehicle->isInitialConnectComplete()) {
        QSignalSpy spyConnect(_vehicle, &Vehicle::initialConnectComplete);
        QVERIFY(spyConnect.isValid());
        QVERIFY2(UnitTest::waitForSignal(spyConnect, TestTimeout::longMs(), QStringLiteral("initialConnectComplete")),
                 "Timeout waiting for initial connect");
    }
}

MavlinkCameraControlInterface* VehicleCameraControlTest::_firstCamera()
{
    QGCCameraManager* const cameraManager = _vehicle->cameraManager();
    // MockLinkCamera creates two cameras.
    if (!cameraManager || !waitForCondition([cameraManager] { return cameraManager->cameras()->count() >= 2; },
                                            TestTimeout::longMs(), u"two cameras discovered")) {
        return nullptr;
    }
    for (int i = 0; i < cameraManager->cameras()->count(); i++) {
        auto* const camera = qobject_cast<MavlinkCameraControlInterface*>(cameraManager->cameras()->get(i));
        if (camera && (camera->compID() == MAV_COMP_ID_CAMERA)) {
            return camera;
        }
    }
    return nullptr;
}

void VehicleCameraControlTest::_testConstructorBuildsCacheFileFromBoundedNames()
{
    _connectVehicle(newMockConfiguration(QStringLiteral("CameraCacheFileTest")));
    if (QTest::currentTestFailed()) {
        return;
    }

    const QString cacheDir = SettingsManager::instance()->appSettings()->parameterSavePath();
    QVERIFY(!cacheDir.isEmpty());

    // model_name is completely full and lens_id, which follows it, is non-zero, so an
    // unbounded read would pick up the extra byte.
    mavlink_camera_information_t info{};
    memcpy(info.vendor_name, "../../evil", 11);
    memset(info.model_name, 'M', sizeof(info.model_name));
    info.lens_id = 'X';
    info.cam_definition_version = 3;

    auto camera = std::make_unique<CameraControlProbe>(&info, _vehicle, MAV_COMP_ID_CAMERA6);

    const QString expectedModel(static_cast<qsizetype>(sizeof(info.model_name)), QLatin1Char('M'));
    QCOMPARE(camera->vendor(), QStringLiteral("../../evil"));
    QCOMPARE(camera->modelName(), expectedModel);

    const QString expectedToken = VehicleCameraControl::cacheFileToken(QStringLiteral("../../evil"), expectedModel, 3);
    QCOMPARE(camera->cacheFile(), QDir::cleanPath(QDir(cacheDir).filePath(expectedToken + QStringLiteral(".xml"))));
    QVERIFY(VehicleCameraControl::pathIsInside(cacheDir, camera->cacheFile()));

    // No definition URI, so the camera initialises as a basic camera straight away.
    QVERIFY(camera->paramComplete());
}

void VehicleCameraControlTest::_testFtpDefinitionDownloadRefusedFallsBack()
{
    _connectVehicle(newMockConfiguration(QStringLiteral("CameraFtpFallbackTest")));
    if (QTest::currentTestFailed()) {
        return;
    }

    // parameterSavePath() is empty when the save root does not exist. Point the save path
    // at a temporary root and remove it, rather than clearing the setting, which would
    // make AppSettings create its subdirectories in the working directory.
    QTemporaryDir missingRoot;
    QVERIFY(missingRoot.isValid());
    const auto restoreSavePath = overrideSavePath(missingRoot.path());
    QVERIFY(missingRoot.remove());
    QVERIFY(SettingsManager::instance()->appSettings()->parameterSavePath().isEmpty());

    // cam_definition_uri is completely full and the bytes after it are non-zero, so an
    // unbounded read would run past the array into them.
    struct
    {
        mavlink_camera_information_t info;
        char tail[4];
    } wire{};

    memcpy(wire.info.vendor_name, "Acme", 5);
    memcpy(wire.info.model_name, "Cam", 4);
    memset(wire.info.cam_definition_uri, 'A', sizeof(wire.info.cam_definition_uri));
    memcpy(wire.info.cam_definition_uri, "mftp://", 7);
    wire.info.gimbal_device_id = 'B';
    wire.info.camera_device_id = 'C';
    memcpy(wire.tail, "ZZZ", 4);

    const qsizetype uriPayloadLength = static_cast<qsizetype>(sizeof(wire.info.cam_definition_uri)) - 7;
    expectLogMessage("Camera.VehicleCameraControl", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Camera definition cache directory is not set")));
    expectLogMessage("Vehicle.FTPManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Destination directory is not set")));
    expectLogMessage(
        "Camera.VehicleCameraControl", QtWarningMsg,
        QRegularExpression(
            QStringLiteral("Could not start camera definition ftp download: \"mftp://A{%1}\"$").arg(uriPayloadLength)));

    auto camera = std::make_unique<CameraControlProbe>(&wire.info, _vehicle, MAV_COMP_ID_CAMERA6);

    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    QVERIFY(camera->cacheFile().isEmpty());

    // The refused download must not leave the camera waiting forever for a definition.
    QVERIFY(camera->paramComplete());
}

void VehicleCameraControlTest::_testFtpDefinitionDownloadFailureFallsBack_data()
{
    QTest::addColumn<QString>("failure");

    QTest::newRow("transfer fails") << QStringLiteral("transfer");
    QTest::newRow("file is truncated") << QStringLiteral("truncated");
    QTest::newRow("file is larger than the limit") << QStringLiteral("limit");
}

// A MAVLink FTP download of the definition that fails ends as one over HTTP does: the camera is initialised without
// a definition, once, and nothing is cached
void VehicleCameraControlTest::_testFtpDefinitionDownloadFailureFallsBack()
{
    QFETCH(QString, failure);

    _connectVehicle(newMockConfiguration(QStringLiteral("CameraFtpFailureTest")));
    if (QTest::currentTestFailed()) {
        return;
    }

    QTemporaryDir saveRoot;
    QVERIFY(saveRoot.isValid());
    const auto restoreSavePath = overrideSavePath(saveRoot.path());
    const QString cacheDir = SettingsManager::instance()->appSettings()->parameterSavePath();
    QVERIFY(QDir(cacheDir).exists());

    QString vehiclePath = QStringLiteral("/general.json");
    QTemporaryDir servedDir;
    QVERIFY(servedDir.isValid());
    if (failure == QStringLiteral("transfer")) {
        _mockLink->mockLinkFTP()->setErrorMode(MockLinkFTP::errModeNoSecondResponse);
        expectLogMessage("Camera.VehicleCameraControl", QtWarningMsg,
                         QRegularExpression(QStringLiteral("ftp download failed: \"Download failed\"$")));
    } else if (failure == QStringLiteral("truncated")) {
        vehiclePath = QStringLiteral("/camera.xml.gz");
        const QByteArray compressed = TestFixtures::gzipMember(cameraDefinition("CAM_FTP"));
        const QString truncatedFile = servedDir.filePath(QStringLiteral("camera.xml.gz"));
        QVERIFY(TestFixtures::writeFile(truncatedFile, compressed.first(compressed.size() / 2)));
        _mockLink->mockLinkFTP()->setServedFile(vehiclePath, truncatedFile);
        expectLogMessage("Camera.VehicleCameraControl", QtWarningMsg,
                         QRegularExpression(QStringLiteral("Could not read the downloaded camera definition.*gzip")));
    } else {
        expectLogMessage(
            "Camera.VehicleCameraControl", QtWarningMsg,
            QRegularExpression(QStringLiteral("Could not read the downloaded camera definition.*size limit")));
    }

    const QByteArray uri = QStringLiteral("mftp://[;comp=%1]%2").arg(MAV_COMP_ID_AUTOPILOT1).arg(vehiclePath).toUtf8();
    mavlink_camera_information_t info{};
    memcpy(info.vendor_name, "Acme", 5);
    memcpy(info.model_name, "Cam", 4);
    memcpy(info.cam_definition_uri, uri.constData(), static_cast<size_t>(uri.size()));

    auto camera = std::make_unique<CameraControlProbe>(&info, _vehicle, MAV_COMP_ID_CAMERA6);
    if (failure == QStringLiteral("limit")) {
        camera->setMaxDefinitionBytes(16);
    }
    QSignalSpy readySpy(camera.get(), &CameraControlProbe::parametersReady);
    QTRY_VERIFY_WITH_TIMEOUT(camera->paramComplete(), TestTimeout::mediumMs());
    verifyExpectedLogMessage();

    QCOMPARE(readySpy.count(), 1);
    QVERIFY(camera->isBasic());
    QVERIFY(!camera->cached());
    QCOMPARE(QDir(cacheDir).entryList(QDir::Files), QStringList());
}

void VehicleCameraControlTest::_testFtpDefinitionDownloadIsSavedAsCacheFile_data()
{
    QTest::addColumn<QString>("vehiclePath");
    // Served by the test, a camera definition compressed with gzip, rather than JSON metadata by MockLinkFTP
    QTest::addColumn<bool>("gzip");

    QTest::newRow("plain") << QStringLiteral("/general.json") << false;
    QTest::newRow("xz") << QStringLiteral("/general.json.xz") << false;
    QTest::newRow("gzip") << QStringLiteral("/camera.xml.gz") << true;
    QTest::newRow("gzip, with a query") << QStringLiteral("/camera.xml.GZ?version=2") << true;
    // Compressed as only its content shows
    QTest::newRow("gzip, without a compression extension") << QStringLiteral("/camera.xml") << true;
}

void VehicleCameraControlTest::_testFtpDefinitionDownloadIsSavedAsCacheFile()
{
    // The file is downloaded beside _cacheFile, compressed or not, and removed once it is read. A definition that
    // loads is written to _cacheFile, decompressed, so that the next connect finds it; one that does not leaves
    // nothing.
    QFETCH(QString, vehiclePath);
    QFETCH(bool, gzip);

    _connectVehicle(newMockConfiguration(QStringLiteral("CameraFtpCacheNameTest")));
    if (QTest::currentTestFailed()) {
        return;
    }

    QTemporaryDir saveRoot;
    QVERIFY(saveRoot.isValid());
    const auto restoreSavePath = overrideSavePath(saveRoot.path());
    const QString cacheDir = SettingsManager::instance()->appSettings()->parameterSavePath();
    QVERIFY(QDir(cacheDir).exists());

    // MockLinkFTP serves its JSON metadata, which is no camera definition. The test serves a camera definition,
    // which a second camera can load from the cache.
    QByteArray servedContent = cameraDefinition("CAM_FTP");
    if (!gzip) {
        QFile served(QStringLiteral(":MockLink/General.MetaData.json"));
        QVERIFY(served.open(QIODevice::ReadOnly));
        servedContent = served.readAll();
    }
    QTemporaryDir servedDir;
    QVERIFY(servedDir.isValid());
    if (gzip) {
        const QString gzipFile = servedDir.filePath(QStringLiteral("camera.xml.gz"));
        QFile file(gzipFile);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QVERIFY(file.write(TestFixtures::gzipMember(servedContent)) > 0);
        file.close();
        _mockLink->mockLinkFTP()->setServedFile(vehiclePath, gzipFile);
    }
    const QString uri = QStringLiteral("mftp://[;comp=%1]%2").arg(MAV_COMP_ID_AUTOPILOT1).arg(vehiclePath);
    const QByteArray uriBytes = uri.toUtf8();
    mavlink_camera_information_t info{};
    memcpy(info.vendor_name, "Acme", 5);
    memcpy(info.model_name, "Cam", 4);
    QVERIFY(uriBytes.size() < static_cast<qsizetype>(sizeof(info.cam_definition_uri)));
    memcpy(info.cam_definition_uri, uriBytes.constData(), static_cast<size_t>(uriBytes.size()));

    if (!gzip) {
        expectLogMessage("default", QtCriticalMsg,
                         QRegularExpression(QStringLiteral("Unable to parse camera definition")));
        // The second message is Qt's own XML parser error text, which can change between Qt
        // versions, so only its presence is checked.
        expectLogMessage("default", QtCriticalMsg, QRegularExpression(QStringLiteral(".+")));
    }

    auto camera = std::make_unique<CameraControlProbe>(&info, _vehicle, MAV_COMP_ID_CAMERA6);
    if (gzip) {
        QTRY_VERIFY_WITH_TIMEOUT(!camera->isBasic(), TestTimeout::longMs());
        QCOMPARE(camera->activeSettings(), QStringList{QStringLiteral("CAM_FTP")});
    } else {
        QTRY_VERIFY_WITH_TIMEOUT(camera->paramComplete(), TestTimeout::longMs());
        verifyExpectedLogMessage();
        verifyExpectedLogMessage();
    }

    const QString expectedCacheFile = QDir::cleanPath(
        QDir(cacheDir).filePath(VehicleCameraControl::cacheFileToken(QStringLiteral("Acme"), QStringLiteral("Cam"), 0) +
                                QStringLiteral(".xml")));
    QCOMPARE(camera->cacheFile(), expectedCacheFile);

    if (!gzip) {
        // What is no definition is not cached, and the download is gone
        QCOMPARE(QDir(cacheDir).entryList(QDir::Files), QStringList());
        return;
    }

    // The cache file holds the definition, decompressed, and no other file is left
    QCOMPARE(TestFixtures::readFile(expectedCacheFile), servedContent);
    QCOMPARE(QDir(cacheDir).entryList(QDir::Files), QStringList{QFileInfo(expectedCacheFile).fileName()});

    // A camera of the same model loads the definition from the cache, without downloading it
    camera.reset();
    int openedFiles = _mockLink->mockLinkFTP()->openFileROCount();
    auto second = std::make_unique<CameraControlProbe>(&info, _vehicle, MAV_COMP_ID_CAMERA6);
    QTRY_VERIFY_WITH_TIMEOUT(!second->isBasic(), TestTimeout::mediumMs());
    QVERIFY(second->cached());
    QCOMPARE(second->activeSettings(), QStringList{QStringLiteral("CAM_FTP")});
    QCOMPARE(_mockLink->mockLinkFTP()->openFileROCount(), openedFiles);

    // A cache file that cannot be used is removed, and the definition is downloaded again the way its URL says
    second.reset();
    QVERIFY(TestFixtures::writeFile(expectedCacheFile, "not a definition"));
    openedFiles = _mockLink->mockLinkFTP()->openFileROCount();
    expectLogMessage("default", QtWarningMsg, QRegularExpression(QStringLiteral("Could not parse cached camera")));
    auto third = std::make_unique<CameraControlProbe>(&info, _vehicle, MAV_COMP_ID_CAMERA6);
    QTRY_VERIFY_WITH_TIMEOUT(!third->isBasic(), TestTimeout::longMs());
    verifyExpectedLogMessage();
    QVERIFY(!third->cached());
    QCOMPARE(_mockLink->mockLinkFTP()->openFileROCount(), openedFiles + 1);
    QCOMPARE(TestFixtures::readFile(expectedCacheFile), servedContent);
    QCOMPARE(QDir(cacheDir).entryList(QDir::Files), QStringList{QFileInfo(expectedCacheFile).fileName()});
}

void VehicleCameraControlTest::_testHttpDefinitionIsHeldToSizeAndDecompressed_data()
{
    QTest::addColumn<QByteArray>("response");
    QTest::addColumn<QString>("refusal");

    const QByteArray definition = cameraDefinition("CAM_HTTP");
    const QByteArray oversized(8 * 1024, ' ');
    const auto withLength = [](const QByteArray& body) {
        return QByteArray("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Length: ") +
               QByteArray::number(body.size()) + "\r\n\r\n" + body;
    };
    const QByteArray withoutLength("HTTP/1.1 200 OK\r\nConnection: close\r\n\r\n");

    QTest::newRow("compressed") << withLength(TestFixtures::gzipMember(definition)) << QString();
    QTest::newRow("larger than the limit")
        << withLength(definition + oversized) << QStringLiteral("Camera definition is larger than");
    QTest::newRow("larger than the limit, of no announced length")
        << (withoutLength + definition + oversized) << QStringLiteral("Camera definition is larger than");
    QTest::newRow("larger than the limit once decompressed")
        << withLength(TestFixtures::gzipMember(definition + oversized))
        << QStringLiteral("Could not decompress the camera definition.*size limit exceeded");
}

// A definition over HTTP(S) comes from where the camera points to. It is held to a size, also when it is not
// compressed, and decompressed if it is, as over MAVLink FTP. A definition that is refused leaves nothing: the camera
// is initialised without it and nothing is cached.
void VehicleCameraControlTest::_testHttpDefinitionIsHeldToSizeAndDecompressed()
{
    QFETCH(QByteArray, response);
    QFETCH(QString, refusal);

    _connectVehicle(newMockConfiguration(QStringLiteral("CameraHttpDefinitionTest")));
    if (QTest::currentTestFailed()) {
        return;
    }

    QTemporaryDir saveRoot;
    QVERIFY(saveRoot.isValid());
    const auto restoreSavePath = overrideSavePath(saveRoot.path());
    const QString cacheFile =
        QDir(SettingsManager::instance()->appSettings()->parameterSavePath())
            .filePath(VehicleCameraControl::cacheFileToken(QStringLiteral("Acme"), QStringLiteral("Cam"), 1) +
                      QStringLiteral(".xml"));

    TestFixtures::LocalHttpTestServer server;
    QVERIFY(server.listen());
    server.installRawResponder(response);

    mavlink_camera_information_t info{};
    memcpy(info.vendor_name, "Acme", 5);
    memcpy(info.model_name, "Cam", 4);
    info.cam_definition_version = 1;
    const QByteArray uri = server.url(QStringLiteral("/definition.xml")).toUtf8();
    QVERIFY(uri.size() < static_cast<qsizetype>(sizeof(info.cam_definition_uri)));
    memcpy(info.cam_definition_uri, uri.constData(), static_cast<size_t>(uri.size()));

    if (!refusal.isEmpty()) {
        expectLogMessage("Camera.VehicleCameraControl", QtWarningMsg, QRegularExpression(refusal));
    }
    // Nothing arrives before the event loop runs, so the limit is set in time
    auto camera = std::make_unique<CameraControlProbe>(&info, _vehicle, MAV_COMP_ID_CAMERA6);
    camera->setMaxDefinitionBytes(4 * 1024);

    if (!refusal.isEmpty()) {
        QTRY_VERIFY_WITH_TIMEOUT(camera->paramComplete(), TestTimeout::mediumMs());
        verifyExpectedLogMessage();
        QVERIFY(camera->isBasic());
        QVERIFY(!QFile::exists(cacheFile));
        QTRY_VERIFY_WITH_TIMEOUT(camera->findChildren<QNetworkReply*>().isEmpty(), TestTimeout::shortMs());
        return;
    }

    QTRY_VERIFY_WITH_TIMEOUT(!camera->isBasic(), TestTimeout::mediumMs());
    QCOMPARE(camera->activeSettings(), QStringList{QStringLiteral("CAM_HTTP")});
    QFile cache(cacheFile);
    QVERIFY(cache.open(QIODevice::ReadOnly));
    QCOMPARE(cache.readAll().trimmed(), cameraDefinition("CAM_HTTP"));
}

void VehicleCameraControlTest::_testDefinitionIsReadAndParsedOnWorker_data()
{
    QTest::addColumn<QString>("end");

    QTest::newRow("cached definition loaded") << QStringLiteral("none");
    QTest::newRow("newer definition replaces it") << QStringLiteral("newer definition");
    QTest::newRow("camera deleted") << QStringLiteral("delete");
}

// The definition comes from the camera, or from where it points to, and may be large: it is read and parsed on a
// worker thread, and the camera is initialised with it afterwards. A definition that is no longer wanted, replaced by
// a newer one or of a camera that is gone, is dropped.
void VehicleCameraControlTest::_testDefinitionIsReadAndParsedOnWorker()
{
    QFETCH(QString, end);

    _connectVehicle(newMockConfiguration(QStringLiteral("CameraDefinitionWorkerTest")));
    if (QTest::currentTestFailed()) {
        return;
    }

    QTemporaryDir saveRoot;
    QVERIFY(saveRoot.isValid());
    const auto restoreSavePath = overrideSavePath(saveRoot.path());
    const QString cacheFile =
        QDir(SettingsManager::instance()->appSettings()->parameterSavePath())
            .filePath(VehicleCameraControl::cacheFileToken(QStringLiteral("Acme"), QStringLiteral("Cam"), 1) +
                      QStringLiteral(".xml"));
    QFile cache(cacheFile);
    QVERIFY(cache.open(QIODevice::WriteOnly));
    QVERIFY(cache.write(cameraDefinition("CAM_CACHED")) > 0);
    cache.close();

    // Not MAVLink FTP and cached, so nothing is downloaded
    mavlink_camera_information_t info{};
    memcpy(info.vendor_name, "Acme", 5);
    memcpy(info.model_name, "Cam", 4);
    info.cam_definition_version = 1;
    const QByteArray uri("http://127.0.0.1:1/definition.xml");
    memcpy(info.cam_definition_uri, uri.constData(), static_cast<size_t>(uri.size()));

    TestFixtures::ThreadPoolBlocker blocker;
    QVERIFY(blocker.isBlocking());
    auto camera = std::make_unique<CameraControlProbe>(&info, _vehicle, MAV_COMP_ID_CAMERA6);
    QCoreApplication::processEvents();
    QVERIFY(!camera->cached());
    QVERIFY(camera->isBasic());

    if (end != QStringLiteral("none")) {
        camera->loadDefinition(cameraDefinition("CAM_NEWER"));
        QCoreApplication::processEvents();
        QVERIFY(camera->isBasic());
    }
    if (end == QStringLiteral("delete")) {
        camera.reset();
    }

    blocker.release();
    if (end == QStringLiteral("delete")) {
        QVERIFY(QThreadPool::globalInstance()->waitForDone(TestTimeout::mediumMs()));
        QCoreApplication::sendPostedEvents();
        // A definition that was not read from the cache would have been written to it
        QVERIFY(cache.open(QIODevice::ReadOnly));
        QCOMPARE(cache.readAll(), cameraDefinition("CAM_CACHED"));
        return;
    }

    QTRY_VERIFY_WITH_TIMEOUT(!camera->isBasic(), TestTimeout::mediumMs());
    const bool newer = (end == QStringLiteral("newer definition"));
    QCOMPARE(camera->activeSettings(), QStringList{newer ? QStringLiteral("CAM_NEWER") : QStringLiteral("CAM_CACHED")});
    // The cached definition that the newer one replaced was dropped unread, so the newer one is cached now
    QCOMPARE(camera->cached(), !newer);
    QVERIFY(cache.open(QIODevice::ReadOnly));
    QCOMPARE(cache.readAll(), cameraDefinition(newer ? "CAM_NEWER" : "CAM_CACHED"));
}

void VehicleCameraControlTest::initTestCase()
{
    UnitTest::initTestCase();
    MultiVehicleManager::instance()->init();
}

void VehicleCameraControlTest::init()
{
    UnitTest::init();
    _mockLink = nullptr;
    _vehicle = nullptr;
}

void VehicleCameraControlTest::cleanup()
{
    if (_mockLink) {
        QSignalSpy spyDisconnect(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);
        _mockLink->disconnect();
        _mockLink = nullptr;

        if (_vehicle) {
            UnitTest::waitForSignal(spyDisconnect, TestTimeout::longMs(), QStringLiteral("activeVehicleChanged"));
        }
        _vehicle = nullptr;

        UnitTest::settleEventLoopForCleanup();
    }

    dumpFailureContextIfTestFailed(QStringLiteral("cleanup"));
    UnitTest::cleanup();
}

void VehicleCameraControlTest::_testCameraCapFlags_data()
{
    // MockConfiguration camera flags
    QTest::addColumn<bool>("captureVideo");
    QTest::addColumn<bool>("captureImage");
    QTest::addColumn<bool>("hasModes");
    QTest::addColumn<bool>("hasVideoStream");
    QTest::addColumn<bool>("canCaptureImageInVideoMode");
    QTest::addColumn<bool>("canCaptureVideoInImageMode");
    QTest::addColumn<bool>("hasBasicZoom");
    QTest::addColumn<bool>("hasTrackingPoint");
    QTest::addColumn<bool>("hasTrackingRectangle");

    // Expected VehicleCameraControl properties
    QTest::addColumn<bool>("expectedCapturesVideo");
    QTest::addColumn<bool>("expectedCapturesPhotos");
    QTest::addColumn<bool>("expectedHasModes");
    QTest::addColumn<bool>("expectedHasZoom");
    QTest::addColumn<bool>("expectedHasVideoStream");
    QTest::addColumn<bool>("expectedPhotosInVideoMode");
    QTest::addColumn<bool>("expectedVideoInPhotoMode");
    QTest::addColumn<bool>("expectedHasTracking");

    // Note: xCapVid and xCapImg (capturesVideo/capturesPhotos) are "virtual" capabilities that account for
    // local gstreamer support. When a camera has a video stream (HAS_VIDEO_STREAM), QGC can locally record
    // video via gstreamer and screen-grab photos from the stream, even if the camera itself doesn't support
    // native video recording or photo capture. See "video-stream-only" and "stream-no-native-capture" rows.

    //                                       capVid   capImg   modes    stream   imgInVid vidInImg zoom     trkPt    trkRect   xCapVid  xCapImg  xModes   xZoom    xStream  xImgVid  xVidImg  xTrack
    QTest::newRow("all-caps")                << true  << true  << true  << true  << true  << true  << true  << true  << true   << true  << true  << true  << true  << true  << true  << true  << true;
    QTest::newRow("no-caps")                 << false << false << false << false << false << false << false << false << false  << false << false << false << false << false << false << false << false;
    QTest::newRow("photo-only")              << false << true  << false << false << false << false << false << false << false  << false << true  << false << false << false << false << false << false;
    QTest::newRow("video-only")              << true  << false << false << false << false << false << false << false << false  << true  << false << false << false << false << false << false << false;
    QTest::newRow("video-stream-only")       << false << false << false << true  << false << false << false << false << false  << true  << true  << false << false << true  << false << false << false;
    QTest::newRow("modes-zoom")              << false << true  << true  << false << false << false << true  << false << false  << false << true  << true  << true  << false << false << false << false;
    QTest::newRow("tracking-point")          << false << true  << false << false << false << false << false << true  << false  << false << true  << false << false << false << false << false << true;
    QTest::newRow("tracking-rect")           << false << true  << false << false << false << false << false << false << true   << false << true  << false << false << false << false << false << true;
    QTest::newRow("tracking-both")           << false << true  << false << false << false << false << false << true  << true   << false << true  << false << false << false << false << false << true;
    QTest::newRow("image-in-video")          << true  << true  << true  << false << true  << false << false << false << false  << true  << true  << true  << false << false << true  << false << false;
    QTest::newRow("video-in-image")          << true  << true  << true  << false << false << true  << false << false << false  << true  << true  << true  << false << false << false << true  << false;
    QTest::newRow("stream-no-native-capture")<< false << false << false << true  << false << false << false << false << false  << true  << true  << false << false << true  << false << false << false;
    QTest::newRow("stream-plus-photo")       << false << true  << false << true  << false << false << false << false << false  << true  << true  << false << false << true  << false << false << false;
}

void VehicleCameraControlTest::_testCameraCapFlags()
{
    // Fetch test data
    QFETCH(bool, captureVideo);
    QFETCH(bool, captureImage);
    QFETCH(bool, hasModes);
    QFETCH(bool, hasVideoStream);
    QFETCH(bool, canCaptureImageInVideoMode);
    QFETCH(bool, canCaptureVideoInImageMode);
    QFETCH(bool, hasBasicZoom);
    QFETCH(bool, hasTrackingPoint);
    QFETCH(bool, hasTrackingRectangle);

    QFETCH(bool, expectedCapturesVideo);
    QFETCH(bool, expectedCapturesPhotos);
    QFETCH(bool, expectedHasModes);
    QFETCH(bool, expectedHasZoom);
    QFETCH(bool, expectedHasVideoStream);
    QFETCH(bool, expectedPhotosInVideoMode);
    QFETCH(bool, expectedVideoInPhotoMode);
    QFETCH(bool, expectedHasTracking);

    // Create MockConfiguration with camera enabled and custom flags
    auto* mockConfig = newMockConfiguration(QStringLiteral("CameraCapFlagsTest"));
    mockConfig->setEnableCamera(true);
    mockConfig->setCameraCaptureVideo(captureVideo);
    mockConfig->setCameraCaptureImage(captureImage);
    mockConfig->setCameraHasModes(hasModes);
    mockConfig->setCameraHasVideoStream(hasVideoStream);
    mockConfig->setCameraCanCaptureImageInVideoMode(canCaptureImageInVideoMode);
    mockConfig->setCameraCanCaptureVideoInImageMode(canCaptureVideoInImageMode);
    mockConfig->setCameraHasBasicZoom(hasBasicZoom);
    mockConfig->setCameraHasTrackingPoint(hasTrackingPoint);
    mockConfig->setCameraHasTrackingRectangle(hasTrackingRectangle);

    _connectVehicle(mockConfig);
    if (QTest::currentTestFailed()) {
        return;
    }

    // Camera 1 has our configured flags. Camera 2 is always photo-only and not what we're testing.
    MavlinkCameraControlInterface* const camera = _firstCamera();
    QVERIFY2(camera, "Camera 1 (MAV_COMP_ID_CAMERA) not found in camera list");

    // Verify capability properties match expected values
    QCOMPARE(camera->capturesVideo(),     expectedCapturesVideo);
    QCOMPARE(camera->capturesPhotos(),    expectedCapturesPhotos);
    QCOMPARE(camera->hasModes(),          expectedHasModes);
    QCOMPARE(camera->hasZoom(),           expectedHasZoom);
    QCOMPARE(camera->hasVideoStream(),    expectedHasVideoStream);
    QCOMPARE(camera->photosInVideoMode(), expectedPhotosInVideoMode);
    QCOMPARE(camera->videoInPhotoMode(),  expectedVideoInPhotoMode);
    QCOMPARE(camera->hasTracking(),       expectedHasTracking);

    // hasFocus is always false since MockLinkCamera doesn't support CAMERA_CAP_FLAGS_HAS_BASIC_FOCUS
    QCOMPARE(camera->hasFocus(), false);
}

void VehicleCameraControlTest::_testZoomTriggersCameraSettingsRequest()
{
    // A spec-minimal camera does not broadcast CAMERA_SETTINGS after a zoom change,
    // so QGC must re-request it itself once a zoom command is accepted. Otherwise
    // the zoom level (and FOV) shown by QGC goes stale after the first zoom.

    auto* mockConfig = newMockConfiguration(QStringLiteral("CameraZoomSettingsTest"));
    mockConfig->setEnableCamera(true);
    mockConfig->setCameraCaptureImage(true);
    mockConfig->setCameraHasBasicZoom(true);

    _connectVehicle(mockConfig);
    if (QTest::currentTestFailed()) {
        return;
    }

    MavlinkCameraControlInterface* const camera = _firstCamera();
    QVERIFY2(camera, "Camera 1 (MAV_COMP_ID_CAMERA) not found in camera list");
    QVERIFY(camera->hasZoom());

    // Wait for the initial CAMERA_SETTINGS exchange to settle: the mock reports zoom
    // level 1.0, so once QGC shows it the initial request/retry cycle is complete.
    QVERIFY_TRUE_WAIT(qFuzzyCompare(camera->zoomLevel(), 1.0), TestTimeout::longMs());

    // QGC alternates between REQUEST_MESSAGE and the deprecated REQUEST_CAMERA_SETTINGS
    // on retries (dual-transport migration), so accept either as a settings request.
    auto settingsRequestCount = [this]() {
        return _mockLink->receivedRequestMessageCount(MAV_COMP_ID_CAMERA, MAVLINK_MSG_ID_CAMERA_SETTINGS)
             + _mockLink->receivedMavCommandCount(MAV_CMD_REQUEST_CAMERA_SETTINGS, MAV_COMP_ID_CAMERA);
    };

    // An accepted zoom command must cause QGC to re-request CAMERA_SETTINGS on its own
    const int baselineAfterConnect = settingsRequestCount();
    camera->setZoomLevel(50.0);
    QTRY_VERIFY2_WITH_TIMEOUT(settingsRequestCount() > baselineAfterConnect,
                              "QGC did not re-request CAMERA_SETTINGS after setZoomLevel was accepted",
                              TestTimeout::longMs());
    // And the fresh settings update QGC's stale zoom level
    QVERIFY_TRUE_WAIT(qFuzzyCompare(camera->zoomLevel(), 50.0), TestTimeout::longMs());

    // Continuous zoom (start/stop) must trigger a refresh as well
    const int baselineAfterSetZoom = settingsRequestCount();
    camera->startZoom(1);
    camera->stopZoom();
    QTRY_VERIFY2_WITH_TIMEOUT(settingsRequestCount() > baselineAfterSetZoom,
                              "QGC did not re-request CAMERA_SETTINGS after startZoom/stopZoom were accepted",
                              TestTimeout::longMs());
}

void VehicleCameraControlTest::_testExtraStreamsStartAndStop()
{
    auto* mockConfig = newMockConfiguration(QStringLiteral("CameraExtraStreamsTest"));
    mockConfig->setEnableCamera(true);
    mockConfig->setCameraHasVideoStream(true);

    _connectVehicle(mockConfig);
    if (QTest::currentTestFailed()) {
        return;
    }

    MavlinkCameraControlInterface* const camera = _firstCamera();
    QVERIFY2(camera, "Camera 1 (MAV_COMP_ID_CAMERA) not found in camera list");
    QGCCameraManager* const cameraManager = _vehicle->cameraManager();
    // The mock camera advertises its streams 1 and 2.
    QVERIFY_TRUE_WAIT(camera->streamLabels().size() == 2, TestTimeout::longMs());
    QCOMPARE(camera->currentStream(), 0);

    const auto transmitted = [this](uint8_t streamId) {
        return _mockLink->cameraStreamTransmitted(MAV_COMP_ID_CAMERA, streamId);
    };
    const auto started = [this] {
        return _mockLink->receivedMavCommandCount(MAV_CMD_VIDEO_START_STREAMING, MAV_COMP_ID_CAMERA);
    };
    const auto stopped = [this] {
        return _mockLink->receivedMavCommandCount(MAV_CMD_VIDEO_STOP_STREAMING, MAV_COMP_ID_CAMERA);
    };
    // QGC requests a stream's status with REQUEST_MESSAGE or the deprecated command, in turn.
    const auto statusRequests = [this] {
        return _mockLink->receivedRequestMessageCount(MAV_COMP_ID_CAMERA, MAVLINK_MSG_ID_VIDEO_STREAM_STATUS) +
               _mockLink->receivedMavCommandCount(MAV_CMD_REQUEST_VIDEO_STREAM_STATUS, MAV_COMP_ID_CAMERA);
    };

    // A camera transmits its first stream, the current one, without being asked.
    QCOMPARE(started(), 0);
    QVERIFY(transmitted(1));
    QVERIFY(!transmitted(2));

    // A stream shown besides the current one is started.
    camera->setExtraStreams({2});
    QVERIFY_TRUE_WAIT(transmitted(2), TestTimeout::mediumMs());
    QCOMPARE(started(), 1);
    QCOMPARE(stopped(), 0);

    // It becomes the current stream while the former one stays on display, as whoever displays the streams says when
    // told of the change: the camera is asked to stop or start neither. The request for the new current stream's
    // status follows any such command.
    const QMetaObject::Connection display =
        connect(cameraManager, &QGCCameraManager::streamChanged, camera, [camera] { camera->setExtraStreams({1}); });
    const int statusRequestsBefore = statusRequests();
    camera->setCurrentStream(1);
    QVERIFY_TRUE_WAIT(statusRequests() > statusRequestsBefore, TestTimeout::mediumMs());
    QVERIFY(disconnect(display));
    QCOMPARE(started(), 1);
    QCOMPARE(stopped(), 0);
    QVERIFY(transmitted(1));
    QVERIFY(transmitted(2));

    // The former stream leaves the display: it is stopped.
    camera->setExtraStreams({});
    QVERIFY_TRUE_WAIT(!transmitted(1), TestTimeout::mediumMs());
    QVERIFY(transmitted(2));
    QCOMPARE(started(), 1);

    // With no other stream on display, a change of the current stream stops the old one and starts the new one.
    camera->setCurrentStream(0);
    QVERIFY_TRUE_WAIT(transmitted(1) && !transmitted(2), TestTimeout::mediumMs());
    QCOMPARE(started(), 2);
    QCOMPARE(stopped(), 2);

    // Leaving the vehicle stops the extra stream and then the current one, and coming back to it starts them again:
    // each time two commands of one kind. The vehicle refuses a command while the camera has yet to answer the same
    // one, so the second waits for the first's answer.
    camera->setExtraStreams({2});
    QVERIFY_TRUE_WAIT(transmitted(2), TestTimeout::mediumMs());
    camera->setExtraStreams({});
    camera->stopStream();
    QVERIFY_TRUE_WAIT(!transmitted(1) && !transmitted(2), TestTimeout::mediumMs());
    camera->resumeStream();
    camera->setExtraStreams({2});
    QVERIFY_TRUE_WAIT(transmitted(1) && transmitted(2), TestTimeout::mediumMs());
    QCOMPARE(started(), 5);
    QCOMPARE(stopped(), 4);

    // A camera told to stop before its stream is known, as one found while the video is off, is asked to stop the
    // stream once it is announced, since it transmits its first stream without being asked. The mock vehicle's
    // second camera has no stream of its own, and denies the command.
    MavlinkCameraControlInterface* lateCamera = nullptr;
    for (int i = 0; i < cameraManager->cameras()->count(); i++) {
        auto* const candidate = qobject_cast<MavlinkCameraControlInterface*>(cameraManager->cameras()->get(i));
        if (candidate && (candidate->compID() == MAV_COMP_ID_CAMERA2)) {
            lateCamera = candidate;
        }
    }
    QVERIFY(lateCamera);
    QVERIFY(lateCamera->streamLabels().isEmpty());
    lateCamera->stopStream();
    mavlink_video_stream_information_t announced{};
    announced.stream_id = 1;
    announced.count = 1;
    announced.type = VIDEO_STREAM_TYPE_RTPUDP;
    announced.flags = VIDEO_STREAM_STATUS_FLAGS_RUNNING;
    qstrncpy(announced.name, "Late", sizeof(announced.name));
    qstrncpy(announced.uri, "udp://127.0.0.1:5610", sizeof(announced.uri));
    lateCamera->handleVideoStreamInformation(announced);
    QVERIFY_TRUE_WAIT(_mockLink->receivedMavCommandCount(MAV_CMD_VIDEO_STOP_STREAMING, MAV_COMP_ID_CAMERA2) == 1,
                      TestTimeout::mediumMs());
    QCOMPARE(_mockLink->receivedMavCommandCount(MAV_CMD_VIDEO_START_STREAMING, MAV_COMP_ID_CAMERA2), 0);
}

void VehicleCameraControlTest::_testThermalStreamStartsOnceNamed_data()
{
    QTest::addColumn<MockConfiguration::CameraThermalStream>("thermalStream");
    QTest::addColumn<int>("thermalCompId");
    QTest::addColumn<int>("thermalStreamId");

    QTest::newRow("beside a camera's visible-light stream")
        << MockConfiguration::CameraThermalStreamSecondStream << static_cast<int>(MAV_COMP_ID_CAMERA) << 2;
    // As mavlink-camera-manager advertises it, which also sends no stream count.
    QTest::newRow("a camera's only stream")
        << MockConfiguration::CameraThermalStreamSecondCamera << static_cast<int>(MAV_COMP_ID_CAMERA2) << 1;
}

void VehicleCameraControlTest::_testThermalStreamStartsOnceNamed()
{
    QFETCH(MockConfiguration::CameraThermalStream, thermalStream);
    QFETCH(int, thermalCompId);
    QFETCH(int, thermalStreamId);

    auto* mockConfig = newMockConfiguration(QStringLiteral("CameraThermalStreamTest"));
    mockConfig->setEnableCamera(true);
    mockConfig->setCameraHasVideoStream(true);
    mockConfig->setCameraThermalStream(thermalStream);

    _connectVehicle(mockConfig);
    if (QTest::currentTestFailed()) {
        return;
    }

    MavlinkCameraControlInterface* const firstCamera = _firstCamera();
    QVERIFY2(firstCamera, "Camera 1 (MAV_COMP_ID_CAMERA) not found in camera list");
    QGCCameraManager* const cameraManager = _vehicle->cameraManager();
    MavlinkCameraControlInterface* thermalCamera = nullptr;
    for (int i = 0; i < cameraManager->cameras()->count(); i++) {
        auto* const candidate = qobject_cast<MavlinkCameraControlInterface*>(cameraManager->cameras()->get(i));
        if (candidate && (candidate->compID() == thermalCompId)) {
            thermalCamera = candidate;
        }
    }
    QVERIFY(thermalCamera);
    QVERIFY_TRUE_WAIT((firstCamera->streamLabels().size() == 1) && thermalCamera->thermalStreamInstance(),
                      TestTimeout::longMs());
    const QGCVideoStreamInfo* const thermal = thermalCamera->thermalStreamInstance();
    QCOMPARE(thermal->streamID(), thermalStreamId);

    const auto transmitted = [this, thermalCompId, thermalStreamId] {
        return _mockLink->cameraStreamTransmitted(static_cast<uint8_t>(thermalCompId),
                                                  static_cast<uint8_t>(thermalStreamId));
    };
    const auto started = [this, thermalCompId] {
        return _mockLink->receivedMavCommandCount(MAV_CMD_VIDEO_START_STREAMING, thermalCompId);
    };
    const auto stopped = [this, thermalCompId] {
        return _mockLink->receivedMavCommandCount(MAV_CMD_VIDEO_STOP_STREAMING, thermalCompId);
    };

    // A thermal stream is none of the streams to select from: a camera that has no other has no current stream.
    QVERIFY(!thermalCamera->streamLabels().contains(thermal->name()));
    QCOMPARE(thermalCamera->streamLabels().isEmpty(), thermalCamera != firstCamera);
    QCOMPARE(thermalCamera->currentStreamInstance() == nullptr, thermalCamera != firstCamera);

    // So the camera isn't asked for it until it is named as a stream on display, and is asked to stop it once it
    // no longer is.
    QCOMPARE(started(), 0);
    QVERIFY(!transmitted());
    thermalCamera->setExtraStreams({static_cast<quint8>(thermalStreamId)});
    QVERIFY_TRUE_WAIT(transmitted(), TestTimeout::mediumMs());
    QCOMPARE(started(), 1);
    QCOMPARE(stopped(), 0);
    thermalCamera->setExtraStreams({});
    QVERIFY_TRUE_WAIT(!transmitted(), TestTimeout::mediumMs());
    QCOMPARE(started(), 1);
    QCOMPARE(stopped(), 1);
}

void VehicleCameraControlTest::_testStreamAnnouncedAnewIsUpdated()
{
    auto* mockConfig = newMockConfiguration(QStringLiteral("CameraStreamAnnouncedAnewTest"));
    mockConfig->setEnableCamera(true);
    mockConfig->setCameraHasVideoStream(true);

    _connectVehicle(mockConfig);
    if (QTest::currentTestFailed()) {
        return;
    }

    MavlinkCameraControlInterface* const camera = _firstCamera();
    QVERIFY2(camera, "Camera 1 (MAV_COMP_ID_CAMERA) not found in camera list");
    QVERIFY_TRUE_WAIT(camera->streamLabels().size() == 2, TestTimeout::longMs());
    QGCVideoStreamInfo* const stream = camera->currentStreamInstance();
    QVERIFY(stream);

    // As a camera whose stream was set up anew announces it: the same stream, elsewhere and under another name.
    mavlink_video_stream_information_t moved{};
    moved.stream_id = stream->streamID();
    moved.count = 2;
    moved.type = VIDEO_STREAM_TYPE_RTSP;
    moved.flags = VIDEO_STREAM_STATUS_FLAGS_RUNNING;
    moved.encoding = stream->encoding();
    moved.resolution_h = static_cast<uint16_t>(stream->resolution().width());
    moved.resolution_v = static_cast<uint16_t>(stream->resolution().height());
    qstrncpy(moved.name, "Moved", sizeof(moved.name));
    qstrncpy(moved.uri, "rtsp://127.0.0.1:8554/moved", sizeof(moved.uri));

    QSignalSpy infoSpy(stream, &QGCVideoStreamInfo::infoChanged);
    QSignalSpy labelsSpy(camera, &MavlinkCameraControlInterface::streamLabelsChanged);
    camera->handleVideoStreamInformation(moved);
    QCOMPARE(infoSpy.count(), 1);
    QCOMPARE(labelsSpy.count(), 1);
    // The stream whoever plays it holds, which is still the current one under its new name.
    QCOMPARE(camera->currentStreamInstance(), stream);
    QCOMPARE(stream->uri(), QStringLiteral("rtsp://127.0.0.1:8554/moved"));
    QCOMPARE(camera->streamLabels().first(), QStringLiteral("Moved"));
    QCOMPARE(camera->streamLabels().size(), 2);
}

UT_REGISTER_TEST(VehicleCameraControlTest, TestLabel::Integration, TestLabel::Vehicle)
