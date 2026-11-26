#include "RequestMetaDataTypeStateMachineTest.h"

#include <memory>

#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QFutureWatcher>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QLocale>
#include <QtCore/QRandomGenerator>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QStandardPaths>
#include <QtCore/QTemporaryDir>
#include <QtCore/QThreadPool>
#include <QtCore/QUuid>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QSignalSpy>

#include "ArchiveFixtures.h"
#include "CompInfoEvents.h"
#include "CompInfoGeneral.h"
#include "CompInfoParam.h"
#include "ComponentInformationCache.h"
#include "ComponentInformationManager.h"
#include "FTPManager.h"
#include "FactMetaData.h"
#include "FileFixtures.h"
#include "LinkManager.h"
#include "LocalHttpTestServer.h"
#include "MockConfiguration.h"
#include "MockLink.h"
#include "MockLinkFTP.h"
#include "MultiVehicleManager.h"
#include "ProcessFixtures.h"
#include "QGCLoggingCategoryManager.h"
#include "RequestMetaDataTypeStateMachine.h"
#include "UnitTest.h"
#include "Vehicle.h"

namespace {

bool writeRequestMetaDataTestFile(const QString& fileName, const QByteArray& contents)
{
    QFile file(fileName);
    return file.open(QIODevice::WriteOnly | QIODevice::Truncate) && (file.write(contents) == contents.size());
}

bool systemLocaleIsEnglish()
{
    return QLocale::system().name().startsWith(QLatin1String("en"));
}

}  // namespace

void RequestMetaDataTypeStateMachineTest::_typeToStringReflectsRequestedType()
{
    auto* manager = vehicle()->compInfoManager();
    QVERIFY(manager);

    RequestMetaDataTypeStateMachine requestMachine(manager, this);
    QSignalSpy completeSpy(&requestMachine, &RequestMetaDataTypeStateMachine::requestComplete);
    QVERIFY(completeSpy.isValid());

    auto* general = manager->compInfoGeneral(MAV_COMP_ID_AUTOPILOT1);
    QVERIFY(general);
    requestMachine.request(general);
    QCOMPARE(requestMachine.typeToString(), QStringLiteral("COMP_METADATA_TYPE_GENERAL"));
    if (completeSpy.count() == 0) {
        QVERIFY(completeSpy.wait(TestTimeout::longMs()));
    }

    auto* param = manager->compInfoParam(MAV_COMP_ID_AUTOPILOT1);
    QVERIFY(param);
    requestMachine.request(param);
    QCOMPARE(requestMachine.typeToString(), QStringLiteral("COMP_METADATA_TYPE_PARAMETER"));
}

void RequestMetaDataTypeStateMachineTest::_requestCompleteEmittedForGeneral()
{
    auto* manager = vehicle()->compInfoManager();
    QVERIFY(manager);

    RequestMetaDataTypeStateMachine requestMachine(manager, this);
    QSignalSpy completeSpy(&requestMachine, &RequestMetaDataTypeStateMachine::requestComplete);
    QVERIFY(completeSpy.isValid());

    auto* general = manager->compInfoGeneral(MAV_COMP_ID_AUTOPILOT1);
    QVERIFY(general);

    requestMachine.request(general);
    if (completeSpy.count() == 0) {
        QVERIFY(completeSpy.wait(TestTimeout::longMs()));
    }

    QCOMPARE(completeSpy.count(), 1);
    QVERIFY(!requestMachine.inProgress());
    QVERIFY2(general->available() || !general->uriMetaDataFallback().isEmpty(),
             "General metadata URI is empty after request");
}

void RequestMetaDataTypeStateMachineTest::_requestCompleteEmittedForParameter()
{
    auto* manager = vehicle()->compInfoManager();
    QVERIFY(manager);

    RequestMetaDataTypeStateMachine requestMachine(manager, this);
    QSignalSpy completeSpy(&requestMachine, &RequestMetaDataTypeStateMachine::requestComplete);
    QVERIFY(completeSpy.isValid());

    auto* param = manager->compInfoParam(MAV_COMP_ID_AUTOPILOT1);
    QVERIFY(param);

    requestMachine.request(param);
    if (completeSpy.count() == 0) {
        QVERIFY(completeSpy.wait(TestTimeout::longMs()));
    }

    QCOMPARE(completeSpy.count(), 1);
    QVERIFY(!requestMachine.inProgress());
}

void RequestMetaDataTypeStateMachineTest::_sequentialRequestsReuseMachine()
{
    auto* manager = vehicle()->compInfoManager();
    QVERIFY(manager);

    RequestMetaDataTypeStateMachine requestMachine(manager, this);
    QSignalSpy completeSpy(&requestMachine, &RequestMetaDataTypeStateMachine::requestComplete);
    QVERIFY(completeSpy.isValid());

    auto* general = manager->compInfoGeneral(MAV_COMP_ID_AUTOPILOT1);
    auto* param = manager->compInfoParam(MAV_COMP_ID_AUTOPILOT1);
    QVERIFY(general);
    QVERIFY(param);

    requestMachine.request(general);
    if (completeSpy.count() == 0) {
        QVERIFY(completeSpy.wait(TestTimeout::longMs()));
    }

    requestMachine.request(param);
    if (completeSpy.count() == 1) {
        QVERIFY(completeSpy.wait(TestTimeout::longMs()));
    }

    QCOMPARE(completeSpy.count(), 2);
    QVERIFY(!requestMachine.inProgress());
}

void RequestMetaDataTypeStateMachineTest::_requestCompletesForArduPilot()
{
    _disconnectMockLink();
    _connectMockLink(MAV_AUTOPILOT_ARDUPILOTMEGA);

    auto* manager = vehicle()->compInfoManager();
    QVERIFY(manager);

    RequestMetaDataTypeStateMachine requestMachine(manager, this);
    QSignalSpy completeSpy(&requestMachine, &RequestMetaDataTypeStateMachine::requestComplete);
    QVERIFY(completeSpy.isValid());

    auto* general = manager->compInfoGeneral(MAV_COMP_ID_AUTOPILOT1);
    QVERIFY(general);

    requestMachine.request(general);
    if (completeSpy.count() == 0) {
        QVERIFY(completeSpy.wait(TestTimeout::longMs()));
    }

    QCOMPARE(completeSpy.count(), 1);
    QVERIFY(vehicle()->isInitialConnectComplete());
}

void RequestMetaDataTypeStateMachineTest::_requestSkipsCompInfoOnHighLatencyLink()
{
    // High-latency link skips metadata requests, resulting in the expected failure warning.
    ignoreLogMessage("ComponentInformation.RequestMetaDataTypeStateMachine", QtWarningMsg,
                     QRegularExpression("failed to load metadata"));
    _disconnectMockLink();

    LinkManager::instance()->setConnectionsAllowed();
    auto* mvm = MultiVehicleManager::instance();
    QVERIFY(!mvm->activeVehicle());

    QSignalSpy activeVehicleSpy{mvm, &MultiVehicleManager::activeVehicleChanged};
    auto* mockConfig = new MockConfiguration(QStringLiteral("HighLatencyCompInfoMock"));
    mockConfig->setFirmwareType(MAV_AUTOPILOT_PX4);
    mockConfig->setVehicleType(MAV_TYPE_QUADROTOR);
    mockConfig->setHighLatency(true);
    mockConfig->setDynamic(true);

    SharedLinkConfigurationPtr linkConfig = LinkManager::instance()->addConfiguration(mockConfig);
    QVERIFY(LinkManager::instance()->createConnectedLink(linkConfig));

    _mockLink = qobject_cast<MockLink*>(linkConfig->link());
    QVERIFY(_mockLink);

    QVERIFY(activeVehicleSpy.wait(TestTimeout::longMs()));
    _vehicle = mvm->activeVehicle();
    QVERIFY(_vehicle);

    QSignalSpy initialConnectCompleteSpy{_vehicle, &Vehicle::initialConnectComplete};
    QVERIFY(_vehicle->isInitialConnectComplete() || initialConnectCompleteSpy.wait(TestTimeout::longMs()));

    auto* manager = vehicle()->compInfoManager();
    QVERIFY(manager);

    _mockLink->clearReceivedMavCommandCounts();

    RequestMetaDataTypeStateMachine requestMachine(manager, this);
    QSignalSpy completeSpy(&requestMachine, &RequestMetaDataTypeStateMachine::requestComplete);
    QVERIFY(completeSpy.isValid());

    auto* general = manager->compInfoGeneral(MAV_COMP_ID_AUTOPILOT1);
    QVERIFY(general);

    requestMachine.request(general);
    if (completeSpy.count() == 0) {
        QVERIFY(completeSpy.wait(TestTimeout::longMs()));
    }

    QCOMPARE(completeSpy.count(), 1);
    QCOMPARE(_mockLink->receivedMavCommandCount(MAV_CMD_REQUEST_MESSAGE), 0);
    QVERIFY(!requestMachine.inProgress());

    _disconnectMockLink();
}

void RequestMetaDataTypeStateMachineTest::_requestUsesCachedMetadataForParameter()
{
    auto* manager = vehicle()->compInfoManager();
    QVERIFY(manager);
    QVERIFY_TRUE_WAIT(!manager->inProgress(), TestTimeout::mediumMs());

    auto* param = manager->compInfoParam(MAV_COMP_ID_AUTOPILOT1);
    QVERIFY(param);

    static constexpr uint32_t crc = 0x1234ABCD;
    param->setUriMetaData(QStringLiteral("http://example.invalid/cache-hit-param.json"), crc);

    const QString fileTag = QString::asprintf("%08x_%02i_%i", crc, static_cast<int>(param->type), 0);
    const QString tempJsonFile =
        QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
            .filePath(QStringLiteral("qgc-compinfo-cache-hit-%1.json")
                          .arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));

    QFile file(tempJsonFile);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    const QByteArray jsonMetadata =
        R"({"version":1,"parameters":[{"name":"CACHE_HIT_PARAM","type":"Float","shortDesc":"Loaded from cache"}]})";
    QCOMPARE(file.write(jsonMetadata), jsonMetadata.size());
    file.close();

    const QString cachedPath = manager->fileCache().insert(fileTag, tempJsonFile);
    QVERIFY(!cachedPath.isEmpty());
    QVERIFY(QFile::exists(cachedPath));

    _mockLink->clearReceivedMavCommandCounts();
    const int initialCompMetadataRequests =
        _mockLink->receivedRequestMessageCount(MAV_COMP_ID_AUTOPILOT1, MAVLINK_MSG_ID_COMPONENT_METADATA);
    const int initialCompInformationRequests =
        _mockLink->receivedRequestMessageCount(MAV_COMP_ID_AUTOPILOT1, MAVLINK_MSG_ID_COMPONENT_INFORMATION);

    RequestMetaDataTypeStateMachine requestMachine(manager, this);
    QSignalSpy completeSpy(&requestMachine, &RequestMetaDataTypeStateMachine::requestComplete);
    QVERIFY(completeSpy.isValid());

    requestMachine.request(param);
    if (completeSpy.count() == 0) {
        QVERIFY(completeSpy.wait(TestTimeout::longMs()));
    }

    QCOMPARE(completeSpy.count(), 1);
    QCOMPARE(_mockLink->receivedRequestMessageCount(MAV_COMP_ID_AUTOPILOT1, MAVLINK_MSG_ID_COMPONENT_METADATA),
             initialCompMetadataRequests);
    QCOMPARE(_mockLink->receivedRequestMessageCount(MAV_COMP_ID_AUTOPILOT1, MAVLINK_MSG_ID_COMPONENT_INFORMATION),
             initialCompInformationRequests);
    QVERIFY(!requestMachine.inProgress());

    FactMetaData* metadata = param->factMetaDataForName(QStringLiteral("CACHE_HIT_PARAM"), FactMetaData::valueTypeFloat);
    QVERIFY(metadata);
    QCOMPARE(metadata->shortDescription(), QStringLiteral("Loaded from cache"));
}

// A metadata download that is clearly not going to finish in a sane time must be abandoned early: it sits in
// front of parameter load, and on a slow telemetry link the projection is obvious within a few seconds.
void RequestMetaDataTypeStateMachineTest::_slowFtpDownloadAbortsEarly()
{
    auto* manager = vehicle()->compInfoManager();
    QVERIFY(manager);
    QVERIFY_TRUE_WAIT(!manager->inProgress(), TestTimeout::mediumMs());

    auto* param = manager->compInfoParam(MAV_COMP_ID_AUTOPILOT1);
    QVERIFY(param);
    // Random CRC so the file cache cannot satisfy the request
    param->setUriMetaData(QStringLiteral("mftp://[;comp=1]parameter.json.xz"), QRandomGenerator::global()->generate());

    // ~2 KB/s against a 92 KB file: projected total is far past the abort limit after the first few bursts
    _mockLink->mockLinkFTP()->setBurstReadDelayMs(1000);

    // Debug output for the category is off by default; enable it so the abort message is captured
    const char* category = "ComponentInformation.RequestMetaDataTypeStateMachine";
    QGCLoggingCategoryManager::instance()->setCategoryEnabled(category, true);
    const auto restoreLogging = qScopeGuard([category]() {
        QGCLoggingCategoryManager::instance()->setCategoryEnabled(category, false);
    });
    ignoreLogMessage(category, QtDebugMsg, QRegularExpression(".*"));
    ignoreLogMessage(category, QtWarningMsg, QRegularExpression("failed to load metadata"));
    // The mock's blocking burst delay also starves unrelated commands sent to it during the download
    ignoreLogMessage("Vehicle.StandardModes", QtWarningMsg, QRegularExpression("Failed to retrieve available modes"));
    expectLogMessage(category, QtDebugMsg, QRegularExpression("Slow download, aborting"));

    RequestMetaDataTypeStateMachine requestMachine(manager, this);
    QSignalSpy completeSpy(&requestMachine, &RequestMetaDataTypeStateMachine::requestComplete);
    QVERIFY(completeSpy.isValid());

    QElapsedTimer timer;
    timer.start();
    requestMachine.request(param);
    QVERIFY(UnitTest::waitForSignal(completeSpy, TestTimeout::longMs(), QStringLiteral("requestComplete")));
    const qint64 elapsedMs = timer.elapsed();
    _mockLink->mockLinkFTP()->setBurstReadDelayMs(0);

    verifyExpectedLogMessage();
    // Abort fires on the first progress report 5s after the first data packet; well under the old fixed 10s,
    // with slack for CI load
    const qint64 maxAbortMs = TestTimeout::isCI() ? 15000 : 10000;
    QVERIFY2(elapsedMs < maxAbortMs, qPrintable(QStringLiteral("Slow download ran %1 ms before aborting").arg(elapsedMs)));
    QVERIFY(!requestMachine.inProgress());
}

void RequestMetaDataTypeStateMachineTest::_metadataUriFillsItsField_data()
{
    QTest::addColumn<bool>("deprecatedMessage");

    QTest::newRow("COMPONENT_METADATA") << false;
    QTest::newRow("COMPONENT_INFORMATION") << true;
}

void RequestMetaDataTypeStateMachineTest::_metadataUriFillsItsField()
{
    QFETCH(bool, deprecatedMessage);

    auto* manager = vehicle()->compInfoManager();
    QVERIFY(manager);
    QVERIFY_TRUE_WAIT(!manager->inProgress(), TestTimeout::mediumMs());

    // MockLink answers neither request, so the ack and the message below are the answer
    mockLink()->clearReceivedRequestMessageCounts();
    mockLink()->setRequestMessageNoResponse(MAVLINK_MSG_ID_COMPONENT_METADATA);
    mockLink()->setRequestMessageNoResponse(MAVLINK_MSG_ID_COMPONENT_INFORMATION);
    const auto requested = [this](int messageId) {
        return mockLink()->receivedRequestMessageCount(MAV_COMP_ID_AUTOPILOT1, messageId) > 0;
    };

    // A URI of 100 characters fills the field and so has no NUL. This one names nothing to download: it is refused,
    // with a warning that shows all of it, and only it.
    char uri[MAVLINK_MSG_COMPONENT_METADATA_FIELD_URI_LEN];
    for (char& character : uri) {
        character = 'u';
    }
    expectLogMessage("Vehicle.VehicleFileFetch", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Cannot fetch \"u{%1}\" - ").arg(sizeof(uri))));
    expectLogMessage("ComponentInformation.RequestMetaDataTypeStateMachine", QtWarningMsg,
                     QRegularExpression("the download could not be started"));
    ignoreLogMessage("ComponentInformation.RequestMetaDataTypeStateMachine", QtWarningMsg,
                     QRegularExpression("failed to load metadata"));

    CompInfoGeneral general(MAV_COMP_ID_AUTOPILOT1, vehicle());
    RequestMetaDataTypeStateMachine requestMachine(manager, this);
    QSignalSpy completeSpy(&requestMachine, &RequestMetaDataTypeStateMachine::requestComplete);
    QVERIFY(completeSpy.isValid());
    requestMachine.request(&general);
    QTRY_VERIFY_WITH_TIMEOUT(requested(MAVLINK_MSG_ID_COMPONENT_METADATA), TestTimeout::mediumMs());

    mavlink_message_t message{};
    if (deprecatedMessage) {
        // A vehicle without COMPONENT_METADATA is asked for the message it replaced. In that one
        // peripherals_metadata_uri follows general_metadata_uri.
        mockLink()->sendUnexpectedCommandAck(MAV_CMD_REQUEST_MESSAGE, MAV_RESULT_UNSUPPORTED);
        QTRY_VERIFY_WITH_TIMEOUT(requested(MAVLINK_MSG_ID_COMPONENT_INFORMATION), TestTimeout::mediumMs());
        static_assert(sizeof(uri) == MAVLINK_MSG_COMPONENT_INFORMATION_FIELD_GENERAL_METADATA_URI_LEN);
        const char peripheralsUri[MAVLINK_MSG_COMPONENT_INFORMATION_FIELD_PERIPHERALS_METADATA_URI_LEN] =
            "SHOULD_NOT_APPEAR";
        (void) mavlink_msg_component_information_pack_chan(mockLink()->vehicleId(), MAV_COMP_ID_AUTOPILOT1,
                                                           mockLink()->outgoingMavlinkChannel(), &message, 0, 0, uri, 0,
                                                           peripheralsUri);
    } else {
        (void) mavlink_msg_component_metadata_pack_chan(mockLink()->vehicleId(), MAV_COMP_ID_AUTOPILOT1,
                                                        mockLink()->outgoingMavlinkChannel(), &message, 0, 0, uri);
    }
    mockLink()->respondWithMavlinkMessage(message);
    mockLink()->sendUnexpectedCommandAck(MAV_CMD_REQUEST_MESSAGE, MAV_RESULT_ACCEPTED);

    QTRY_COMPARE_WITH_TIMEOUT(general.uriMetaData(), QString(sizeof(uri), QLatin1Char('u')), TestTimeout::mediumMs());

    if (completeSpy.isEmpty()) {
        QVERIFY(completeSpy.wait(TestTimeout::longMs()));
    }
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
}

// A state that times out is left while its download is still running. The download has to end with the state:
// one that ran on completed whichever state was current when it ended, and kept the vehicle's FTP session, and the
// transfers that wait for it.
void RequestMetaDataTypeStateMachineTest::_downloadEndsWithItsState()
{
    auto* manager = vehicle()->compInfoManager();
    QVERIFY(manager);
    QVERIFY_TRUE_WAIT(!manager->inProgress(), TestTimeout::mediumMs());

    auto* param = manager->compInfoParam(MAV_COMP_ID_AUTOPILOT1);
    QVERIFY(param);
    // Random CRC so the file cache cannot satisfy the request
    param->setUriMetaData(QStringLiteral("mftp://[;comp=1]parameter.json.xz"), QRandomGenerator::global()->generate());

    // One burst a second: the download is far from done when its state gives up, and the rule for slow downloads,
    // which needs five seconds of data, has not spoken yet
    _mockLink->mockLinkFTP()->setBurstReadDelayMs(1000);
    RequestMetaDataTypeStateMachine::Timeouts timeouts = RequestMetaDataTypeStateMachine::defaultTimeouts;
    timeouts.metaDataDownload = 500;

    const char* category = "ComponentInformation.RequestMetaDataTypeStateMachine";
    expectLogMessage("Utilities.StateMachine.QGCStateMachine", QtWarningMsg,
                     QRegularExpression("^Timeout \".*RequestMetaDataJson\" after 500 ms"));
    expectLogMessage(category, QtWarningMsg, QRegularExpression("failed to load metadata"));
    // The mock's blocking burst delay also starves unrelated commands sent to it during the download
    ignoreLogMessage("Vehicle.StandardModes", QtWarningMsg, QRegularExpression("Failed to retrieve available modes"));

    RequestMetaDataTypeStateMachine requestMachine(manager, this, timeouts);
    QSignalSpy completeSpy(&requestMachine, &RequestMetaDataTypeStateMachine::requestComplete);
    QVERIFY(completeSpy.isValid());

    requestMachine.request(param);
    QVERIFY(UnitTest::waitForSignal(completeSpy, TestTimeout::longMs(), QStringLiteral("requestComplete")));
    _mockLink->mockLinkFTP()->setBurstReadDelayMs(0);
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();

    // The download was given up when its state was left
    QCOMPARE(vehicle()->ftpManager()->requestCount(), 0);

    // So the vehicle's FTP session is free: the same file is downloaded now. A download that fails is reported with
    // a warning, which fails the test.
    requestMachine.request(param);
    QVERIFY(UnitTest::waitForSignal(completeSpy, TestTimeout::longMs(), QStringLiteral("requestComplete")));
}

// The same for a download over HTTP: it is aborted when its state is left, which closes its connection, and the next
// request downloads anew.
void RequestMetaDataTypeStateMachineTest::_httpDownloadEndsWithItsState()
{
    auto* manager = vehicle()->compInfoManager();
    QVERIFY(manager);
    QVERIFY_TRUE_WAIT(!manager->inProgress(), TestTimeout::mediumMs());

    // Accepts a connection and never answers it
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    QSignalSpy connectionSpy(&server, &QTcpServer::newConnection);

    auto* param = manager->compInfoParam(MAV_COMP_ID_AUTOPILOT1);
    QVERIFY(param);
    // Random CRC so the file cache cannot satisfy the request
    param->setUriMetaData(QStringLiteral("http://127.0.0.1:%1/parameter.json").arg(server.serverPort()),
                          QRandomGenerator::global()->generate());

    RequestMetaDataTypeStateMachine::Timeouts timeouts = RequestMetaDataTypeStateMachine::defaultTimeouts;
    timeouts.metaDataDownload = 500;
    RequestMetaDataTypeStateMachine requestMachine(manager, this, timeouts);
    QSignalSpy completeSpy(&requestMachine, &RequestMetaDataTypeStateMachine::requestComplete);
    QVERIFY(completeSpy.isValid());

    for (int request = 1; request <= 2; ++request) {
        expectLogMessage("Utilities.StateMachine.QGCStateMachine", QtWarningMsg,
                         QRegularExpression("^Timeout \".*RequestMetaDataJson\" after 500 ms"));
        expectLogMessage("ComponentInformation.RequestMetaDataTypeStateMachine", QtWarningMsg,
                         QRegularExpression("failed to load metadata"));
        requestMachine.request(param);
        QVERIFY(UnitTest::waitForSignal(completeSpy, TestTimeout::longMs(), QStringLiteral("requestComplete")));
        verifyExpectedLogMessage();
        verifyExpectedLogMessage();
        QTRY_COMPARE_WITH_TIMEOUT(connectionSpy.count(), request, TestTimeout::shortMs());

        QTcpSocket* const connection = server.nextPendingConnection();
        QVERIFY(connection);
        QTRY_COMPARE_WITH_TIMEOUT(connection->state(), QAbstractSocket::UnconnectedState, TestTimeout::shortMs());
    }
}

// A new build of a vehicle serves its metadata at the URL of the build before, with another CRC. The file of the new
// CRC is fetched from the server, not taken from the network disk cache, which keeps the file of that URL whatever
// its CRC.
void RequestMetaDataTypeStateMachineTest::_newCrcIsNotTakenFromTheHttpCache()
{
    auto* manager = vehicle()->compInfoManager();
    QVERIFY(manager);
    QVERIFY_TRUE_WAIT(!manager->inProgress(), TestTimeout::mediumMs());
    auto* param = manager->compInfoParam(MAV_COMP_ID_AUTOPILOT1);
    QVERIFY(param);

    TestFixtures::LocalHttpTestServer server;
    QVERIFY(server.listen());
    const QString uri = server.url(_uniqueServerPath(QStringLiteral("parameter.json")));
    RequestMetaDataTypeStateMachine requestMachine(manager, this);
    QSignalSpy completeSpy(&requestMachine, &RequestMetaDataTypeStateMachine::requestComplete);

    for (const QString& build : {QStringLiteral("First build"), QStringLiteral("Next build")}) {
        // The server lets the file be cached
        server.installHttpResponder(
            QStringLiteral(R"({"version":1,"parameters":[{"name":"BUILD_PARAM","type":"Float","shortDesc":"%1"}]})")
                .arg(build)
                .toUtf8(),
            200, "application/json", 3600);
        param->setUriMetaData(uri, QRandomGenerator::global()->generate());
        requestMachine.request(param);
        QVERIFY(UnitTest::waitForSignal(completeSpy, TestTimeout::longMs(), QStringLiteral("requestComplete")));

        FactMetaData* const metadata =
            param->factMetaDataForName(QStringLiteral("BUILD_PARAM"), FactMetaData::valueTypeFloat);
        QVERIFY(metadata);
        QCOMPARE(metadata->shortDescription(), build);
    }
    QCOMPARE(server.connectionCount(), 2);
}

void RequestMetaDataTypeStateMachineTest::_downloadsUseTheDirectoryOfTheirManager_data()
{
    QTest::addColumn<bool>("ftp");

    QTest::newRow("MAVLink FTP") << true;
    QTest::newRow("HTTP") << false;
}

// Vehicles download files of the same names, and may do so at the same time. Each manager writes them, and their
// as they were sent, into a directory of its own, which goes with the manager together with what is left in it.
void RequestMetaDataTypeStateMachineTest::_downloadsUseTheDirectoryOfTheirManager()
{
    QFETCH(bool, ftp);

    auto* manager = vehicle()->compInfoManager();
    QVERIFY(manager);
    QVERIFY_TRUE_WAIT(!manager->inProgress(), TestTimeout::mediumMs());

    QString uri = QStringLiteral("mftp://[;comp=1]parameter.json.xz");
    TestFixtures::LocalHttpTestServer server;
    if (!ftp) {
        QVERIFY(server.listen());
        server.installHttpResponder(
            TestFixtures::gzipMember(R"({"version":1,"parameters":[{"name":"DOWNLOADED_PARAM","type":"Float"}]})"));
        uri = server.url(_uniqueServerPath(QStringLiteral("parameter.json.gz")));
    }

    // A second manager of the vehicle stands for the manager of another vehicle. It makes its directory once it
    // downloads: one that downloads nothing, as that of an offline vehicle, leaves nothing behind.
    const QDir tempDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
    const QStringList directoriesBefore = tempDir.entryList({QStringLiteral("qgc_compinfo_*")}, QDir::Dirs);
    auto otherManager = std::make_unique<ComponentInformationManager>(vehicle());
    QCOMPARE(tempDir.entryList({QStringLiteral("qgc_compinfo_*")}, QDir::Dirs), directoriesBefore);
    const QString otherDirectory = otherManager->downloadDirectory();
    QVERIFY(QFileInfo(otherDirectory).isDir());
    QVERIFY(otherDirectory != manager->downloadDirectory());

    // The file that a download leaves is cached, and so moved, before the request completes: where it was
    // downloaded to is logged, the same way for both
    const char* const category = "ComponentInformation.RequestMetaDataTypeStateMachine";
    QGCLoggingCategoryManager::instance()->setCategoryEnabled(category, true);
    const auto restoreLogging =
        qScopeGuard([category]() { QGCLoggingCategoryManager::instance()->setCategoryEnabled(category, false); });
    ignoreLogMessage(category, QtDebugMsg, QRegularExpression(QStringLiteral(".*")));

    for (ComponentInformationManager* const requester : {manager, otherManager.get()}) {
        auto* const param = requester->compInfoParam(MAV_COMP_ID_AUTOPILOT1);
        QVERIFY(param);
        // Random CRC so the file cache cannot satisfy the request
        param->setUriMetaData(uri, QRandomGenerator::global()->generate());

        const QString downloaded =
            QDir(requester->downloadDirectory())
                .filePath(ftp ? QStringLiteral("parameter.json.xz") : QStringLiteral("parameter.json.gz"));
        expectLogMessage(category, QtDebugMsg,
                         QRegularExpression(QStringLiteral("^Download complete fileName:errorMsg \"%1\" \"\"$")
                                                .arg(QRegularExpression::escape(downloaded))));
        RequestMetaDataTypeStateMachine requestMachine(requester, this);
        QSignalSpy completeSpy(&requestMachine, &RequestMetaDataTypeStateMachine::requestComplete);
        requestMachine.request(param);
        QVERIFY(UnitTest::waitForSignal(completeSpy, TestTimeout::longMs(), QStringLiteral("requestComplete")));
        verifyExpectedLogMessage();
    }

    QVERIFY(TestFixtures::writeFile(QDir(otherDirectory).filePath(QStringLiteral("parameter.json.xz")), "left"));
    otherManager.reset();
    QVERIFY(!QFileInfo::exists(otherDirectory));
}

// A download of the state that waits behind another transfer of the vehicle neither disturbs that transfer nor is
// given up while it waits: the timeout of the state counts from when its download begins.
void RequestMetaDataTypeStateMachineTest::_downloadWaitsForTheOneInProgress()
{
    auto* manager = vehicle()->compInfoManager();
    QVERIFY(manager);
    QVERIFY_TRUE_WAIT(!manager->inProgress(), TestTimeout::mediumMs());

    const QString uri = QStringLiteral("mftp://[;comp=1]parameter.json.xz");
    auto* param = manager->compInfoParam(MAV_COMP_ID_AUTOPILOT1);
    QVERIFY(param);
    // Random CRC so the file cache cannot satisfy the request
    param->setUriMetaData(uri, QRandomGenerator::global()->generate());

    // One burst a second: the other download is far from done while the request waits
    _mockLink->mockLinkFTP()->setBurstReadDelayMs(1000);
    // The mock's blocking burst delay also starves unrelated commands sent to it during the download
    ignoreLogMessage("Vehicle.StandardModes", QtWarningMsg, QRegularExpression("Failed to retrieve available modes"));
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QObject owner;
    int otherDownloads = 0;
    QString otherError;
    QVERIFY(vehicle()->ftpManager()->download(&owner, MAV_COMP_ID_AUTOPILOT1, uri, directory.path(), {},
                                              [&](const QString&, const QString& errorMsg) {
                                                  otherDownloads++;
                                                  otherError = errorMsg;
                                              }));

    RequestMetaDataTypeStateMachine::Timeouts timeouts = RequestMetaDataTypeStateMachine::defaultTimeouts;
    timeouts.metaDataDownload = 2000;
    RequestMetaDataTypeStateMachine requestMachine(manager, this, timeouts);
    QSignalSpy completeSpy(&requestMachine, &RequestMetaDataTypeStateMachine::requestComplete);
    QVERIFY(completeSpy.isValid());

    // Waits longer than the timeout of its state: a download given up then is reported with warnings
    requestMachine.request(param);
    QVERIFY_NO_SIGNAL_WAIT(completeSpy, timeouts.metaDataDownload + 1000);
    QCOMPARE(otherDownloads, 0);

    _mockLink->mockLinkFTP()->setBurstReadDelayMs(0);
    QVERIFY(UnitTest::waitForSignal(completeSpy, TestTimeout::longMs(), QStringLiteral("requestComplete")));
    QCOMPARE(otherDownloads, 1);
    QVERIFY2(otherError.isEmpty(), qPrintable(otherError));
}

QString RequestMetaDataTypeStateMachineTest::_uniqueServerPath(const QString& fileName)
{
    return QStringLiteral("/%1/%2").arg(QUuid::createUuid().toString(QUuid::WithoutBraces), fileName);
}

void RequestMetaDataTypeStateMachineTest::_setUpTranslatedParameterMetadata(const QString& directory,
                                                                            TestFixtures::LocalHttpTestServer& server,
                                                                            const QString& translationUrl,
                                                                            bool compressed,
                                                                            QHash<QString, QByteArray> files)
{
    const QString generalFile = QDir(directory).filePath(QStringLiteral("general.json"));
    const QString metadataPath = _uniqueServerPath(QStringLiteral("parameter.json"));
    const QString summaryPath =
        _uniqueServerPath(compressed ? QStringLiteral("summary.json.gz") : QStringLiteral("summary.json"));

    files.insert(
        metadataPath,
        R"({"version":1,"translation":{"items":{"parameters":{"list":{"key":"name","translate":["shortDesc"]}}}},)"
        R"("parameters":[{"name":"TRANSLATED_PARAM","type":"Float","shortDesc":"In English"}]})");

    const QJsonObject summary{{QLocale::system().name(), QJsonObject{{QStringLiteral("url"), translationUrl}}}};
    const QByteArray summaryJson = QJsonDocument(summary).toJson();
    files.insert(summaryPath, compressed ? TestFixtures::gzipMember(summaryJson) : summaryJson);
    server.installFileResponder(files);

    // Random CRC so the file cache cannot satisfy the request
    const QString summaryUrl = server.url(summaryPath);
    const QJsonObject parameterType{
        {QStringLiteral("type"), COMP_METADATA_TYPE_PARAMETER},
        {QStringLiteral("uri"), server.url(metadataPath)},
        {QStringLiteral("fileCrc"), static_cast<qint64>(QRandomGenerator::global()->generate())},
        {QStringLiteral("translationUri"), summaryUrl},
    };
    const QJsonObject generalMetadata{
        {QStringLiteral("version"), 1},
        {QStringLiteral("metadataTypes"), QJsonArray{parameterType}},
    };
    QVERIFY(writeRequestMetaDataTestFile(generalFile, QJsonDocument(generalMetadata).toJson()));

    // A type has the URI of its translation from the general metadata only
    CompInfoGeneral general(MAV_COMP_ID_AUTOPILOT1, vehicle());
    general.setJson(generalFile, CompInfo::parseJsonFile(generalFile, general.jsonSchema()));
    auto* const param = vehicle()->compInfoManager()->compInfoParam(MAV_COMP_ID_AUTOPILOT1);
    QVERIFY(param);
    general.setUris(*param);
    QCOMPARE(param->uriTranslation(), summaryUrl);
}

void RequestMetaDataTypeStateMachineTest::_metadataIsTranslatedForSystemLocale_data()
{
    QTest::addColumn<bool>("translationNamed");
    QTest::addColumn<bool>("translationReachable");
    QTest::addColumn<bool>("compressed");

    QTest::newRow("the summary names a translation") << true << true << false;
    QTest::newRow("the summary and the translation are compressed") << true << true << true;
    QTest::newRow("the summary names none") << false << false << false;
    QTest::newRow("the translation cannot be downloaded") << true << false << false;
}

// Metadata is translated unless the system locale is English, with the file that the translation summary names for
// the locale. The metadata, the summary and that file come over HTTP, and are decompressed where they are read. Without
// a translation the metadata is used as it was downloaded.
void RequestMetaDataTypeStateMachineTest::_metadataIsTranslatedForSystemLocale()
{
    QFETCH(bool, translationNamed);
    QFETCH(bool, translationReachable);
    QFETCH(bool, compressed);

    auto* manager = vehicle()->compInfoManager();
    QVERIFY(manager);
    QVERIFY_TRUE_WAIT(!manager->inProgress(), TestTimeout::mediumMs());

    // What a missing translation logs depends on the locale: for an English one the summary is not read
    if (!translationNamed) {
        ignoreLogMessage("ComponentInformation.ComponentInformationTranslation", QtWarningMsg,
                         QRegularExpression("has no url in translation json"));
    } else if (!translationReachable) {
        ignoreLogMessage("Utilities.QGCFileDownload", QtWarningMsg, QRegularExpression("Download error"));
        ignoreLogMessage("ComponentInformation.RequestMetaDataTypeStateMachine", QtWarningMsg,
                         QRegularExpression("Metadata translation failed"));
    }

    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    TestFixtures::LocalHttpTestServer server;
    QVERIFY(server.listen());
    const QString translationPath =
        _uniqueServerPath(compressed ? QStringLiteral("translation.ts.gz") : QStringLiteral("translation.ts"));
    const QByteArray translation(
        R"(<?xml version="1.0" encoding="utf-8"?><!DOCTYPE TS><TS version="2.1"><context>)"
        R"(<name>/parameters/TRANSLATED_PARAM/shortDesc</name>)"
        R"(<message><source>In English</source><translation>Translated</translation></message>)"
        R"(</context></TS>)");
    QString translationUrl;
    if (translationNamed && translationReachable) {
        translationUrl = server.url(translationPath);
    } else if (translationNamed) {
        // A port that nothing listens on
        QTcpServer closedServer;
        QVERIFY(closedServer.listen(QHostAddress::LocalHost));
        translationUrl = QStringLiteral("http://127.0.0.1:%1/translation.ts").arg(closedServer.serverPort());
    }
    _setUpTranslatedParameterMetadata(
        directory.path(), server, translationUrl, compressed,
        {{translationPath, compressed ? TestFixtures::gzipMember(translation) : translation}});
    if (QTest::currentTestFailed()) {
        return;
    }

    RequestMetaDataTypeStateMachine requestMachine(manager, this);
    QSignalSpy completeSpy(&requestMachine, &RequestMetaDataTypeStateMachine::requestComplete);
    QVERIFY(completeSpy.isValid());

    // Well before the translation state would have timed out
    auto* param = manager->compInfoParam(MAV_COMP_ID_AUTOPILOT1);
    requestMachine.request(param);
    QVERIFY(UnitTest::waitForSignal(completeSpy, TestTimeout::mediumMs(), QStringLiteral("requestComplete")));
    QVERIFY(!requestMachine.inProgress());

    const bool translated = translationNamed && translationReachable && !systemLocaleIsEnglish();
    FactMetaData* metadata =
        param->factMetaDataForName(QStringLiteral("TRANSLATED_PARAM"), FactMetaData::valueTypeFloat);
    QVERIFY(metadata);
    QCOMPARE(metadata->shortDescription(), translated ? QStringLiteral("Translated") : QStringLiteral("In English"));
}

// The download of the translation ends with its state as well. It runs on the downloader that the metadata files
// use, so one that ran on refused the summary of the next request.
void RequestMetaDataTypeStateMachineTest::_translationDownloadEndsWithItsState()
{
    if (systemLocaleIsEnglish()) {
        QSKIP("No translation is downloaded for an English locale");
    }

    auto* manager = vehicle()->compInfoManager();
    QVERIFY(manager);
    QVERIFY_TRUE_WAIT(!manager->inProgress(), TestTimeout::mediumMs());

    // Accepts a connection and never answers it
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    QSignalSpy connectionSpy(&server, &QTcpServer::newConnection);

    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    TestFixtures::LocalHttpTestServer metadataServer;
    QVERIFY(metadataServer.listen());
    _setUpTranslatedParameterMetadata(directory.path(), metadataServer,
                                      QStringLiteral("http://127.0.0.1:%1/translation.ts").arg(server.serverPort()));
    if (QTest::currentTestFailed()) {
        return;
    }

    RequestMetaDataTypeStateMachine::Timeouts timeouts = RequestMetaDataTypeStateMachine::defaultTimeouts;
    timeouts.translation = 500;
    RequestMetaDataTypeStateMachine requestMachine(manager, this, timeouts);
    QSignalSpy completeSpy(&requestMachine, &RequestMetaDataTypeStateMachine::requestComplete);
    QVERIFY(completeSpy.isValid());

    // The second translation reaches the server only when the first download ended with its state. A summary that
    // found the downloader busy was refused with a warning, which fails the test.
    auto* param = manager->compInfoParam(MAV_COMP_ID_AUTOPILOT1);
    for (int request = 1; request <= 2; ++request) {
        expectLogMessage("Utilities.StateMachine.QGCStateMachine", QtWarningMsg,
                         QRegularExpression("^Timeout \".*RequestTranslate\" after 500 ms"));
        requestMachine.request(param);
        QVERIFY(UnitTest::waitForSignal(completeSpy, TestTimeout::longMs(), QStringLiteral("requestComplete")));
        verifyExpectedLogMessage();
        QTRY_COMPARE_WITH_TIMEOUT(connectionSpy.count(), request, TestTimeout::shortMs());
    }

    // Without its translation the metadata is used as it was downloaded
    FactMetaData* metadata =
        param->factMetaDataForName(QStringLiteral("TRANSLATED_PARAM"), FactMetaData::valueTypeFloat);
    QVERIFY(metadata);
    QCOMPARE(metadata->shortDescription(), QStringLiteral("In English"));
}

// A request whose state has timed out stays outstanding in Vehicle. Its answer has to be dropped: it set the URI
// of whatever the machine was requesting by the time it came.
void RequestMetaDataTypeStateMachineTest::_answerAfterRequestTimeoutIsNotUsed()
{
    auto* manager = vehicle()->compInfoManager();
    QVERIFY(manager);
    QVERIFY_TRUE_WAIT(!manager->inProgress(), TestTimeout::mediumMs());

    // MockLink answers neither request, so the ack and the message below are the answer
    mockLink()->clearReceivedRequestMessageCounts();
    mockLink()->setRequestMessageNoResponse(MAVLINK_MSG_ID_COMPONENT_METADATA);
    mockLink()->setRequestMessageNoResponse(MAVLINK_MSG_ID_COMPONENT_INFORMATION);

    // The two request states give up well before Vehicle stops waiting for the message, which it does half a
    // second after the command is acknowledged
    RequestMetaDataTypeStateMachine::Timeouts timeouts = RequestMetaDataTypeStateMachine::defaultTimeouts;
    timeouts.compInfoRequest = 100;
    ignoreLogMessage("Utilities.StateMachine.QGCStateMachine", QtWarningMsg,
                     QRegularExpression("^Timeout \".*RequestComp(Metadata|InfoDeprecated)\" after 100 ms"));
    ignoreLogMessage("ComponentInformation.RequestMetaDataTypeStateMachine", QtWarningMsg,
                     QRegularExpression("failed to load metadata"));

    bool answerReceived = false;
    connect(vehicle(), &Vehicle::mavlinkMessageReceived, this, [&answerReceived](const mavlink_message_t& message) {
        if (message.msgid == MAVLINK_MSG_ID_COMPONENT_METADATA) {
            answerReceived = true;
        }
    });

    CompInfoGeneral general(MAV_COMP_ID_AUTOPILOT1, vehicle());
    RequestMetaDataTypeStateMachine requestMachine(manager, this, timeouts);
    QSignalSpy completeSpy(&requestMachine, &RequestMetaDataTypeStateMachine::requestComplete);
    QVERIFY(completeSpy.isValid());
    requestMachine.request(&general);
    QTRY_VERIFY_WITH_TIMEOUT(
        mockLink()->receivedRequestMessageCount(MAV_COMP_ID_AUTOPILOT1, MAVLINK_MSG_ID_COMPONENT_METADATA) > 0,
        TestTimeout::mediumMs());
    mockLink()->sendUnexpectedCommandAck(MAV_CMD_REQUEST_MESSAGE, MAV_RESULT_ACCEPTED);

    // Without a URI there is nothing to download, so the request is over once both states have given up
    QVERIFY(UnitTest::waitForSignal(completeSpy, TestTimeout::longMs(), QStringLiteral("requestComplete")));
    QVERIFY(general.uriMetaData().isEmpty());

    mavlink_message_t message{};
    const char uri[MAVLINK_MSG_COMPONENT_METADATA_FIELD_URI_LEN] = "mftp://late.json";
    (void) mavlink_msg_component_metadata_pack_chan(mockLink()->vehicleId(), MAV_COMP_ID_AUTOPILOT1,
                                                    mockLink()->outgoingMavlinkChannel(), &message, 0, 0, uri);
    mockLink()->respondWithMavlinkMessage(message);
    // Vehicle hands a message to its requests before it announces it
    QTRY_VERIFY_WITH_TIMEOUT(answerReceived, TestTimeout::mediumMs());
    QVERIFY(general.uriMetaData().isEmpty());
}

void RequestMetaDataTypeStateMachineTest::_metadataIsParsedOnWorker_data()
{
    QTest::addColumn<int>("type");
    QTest::addColumn<QString>("end");
    QTest::addColumn<QString>("file");

    for (const int type : {COMP_METADATA_TYPE_PARAMETER, COMP_METADATA_TYPE_EVENTS}) {
        const char* const name = (type == COMP_METADATA_TYPE_EVENTS) ? "events" : "parameters";
        QTest::addRow("%s parsed", name) << type << QStringLiteral("none") << QStringLiteral("plain");
        QTest::addRow("%s, load state timed out", name) << type << QStringLiteral("timeout") << QStringLiteral("plain");
        QTest::addRow("%s, machine deleted", name) << type << QStringLiteral("delete") << QStringLiteral("plain");
        // The cache keeps a file as it was downloaded
        QTest::addRow("%s, compressed", name) << type << QStringLiteral("none") << QStringLiteral("compressed");
        QTest::addRow("%s, compressed and truncated", name)
            << type << QStringLiteral("none") << QStringLiteral("truncated");
        QTest::addRow("%s, larger than the limit once decompressed", name)
            << type << QStringLiteral("none") << QStringLiteral("over the limit");
    }
}

// The metadata file comes from the vehicle, or from where it points to, and may be large: it is read, decompressed
// if it is compressed, and parsed on a worker thread, while its state waits. A file that cannot be decompressed, or
// is larger than VehicleFileFetch::kMaxSmallFileBytes once it is, sets nothing, and its entry leaves the file cache, so
// that the next connection downloads it again. So does a parse that ends after its state was left, or its machine
// deleted, which leaves the entry. The events metadata is parsed by libevents, and once it is in place the vehicle is
// asked for its prearm checks.
void RequestMetaDataTypeStateMachineTest::_metadataIsParsedOnWorker()
{
    QFETCH(int, type);
    QFETCH(QString, end);
    QFETCH(QString, file);
    const bool events = (type == COMP_METADATA_TYPE_EVENTS);
    const bool readable = (file == QStringLiteral("plain")) || (file == QStringLiteral("compressed"));

    auto* manager = vehicle()->compInfoManager();
    QVERIFY(manager);
    QVERIFY_TRUE_WAIT(!manager->inProgress(), TestTimeout::mediumMs());

    // A component of its own, so that only this test sets its metadata
    constexpr uint8_t compId = 43;
    CompInfoParam* const param = manager->compInfoParam(compId);
    QVERIFY(param);
    CompInfoEvents eventsInfo(compId, vehicle());
    CompInfo* const compInfo = events ? static_cast<CompInfo*>(&eventsInfo) : param;
    const uint32_t crc = QRandomGenerator::global()->generate();
    compInfo->setUriMetaData(QStringLiteral("http://example.invalid/worker-metadata.json"), crc);

    // From the file cache, so that nothing but the parse uses a worker thread
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString metadataFile = directory.filePath(QStringLiteral("worker-metadata.json"));
    const QByteArray contents =
        events ? R"({"version":2,"components":{"43":{"namespace":"worker",)"
                 R"("supported_protocols":["health_and_arming_check"]}}})"
               : R"({"version":1,"parameters":[{"name":"WORKER_PARAM","type":"Float","shortDesc":"Parsed"}]})";
    if (file == QStringLiteral("plain")) {
        QVERIFY(writeRequestMetaDataTestFile(metadataFile, contents));
    } else if (file == QStringLiteral("compressed")) {
        QVERIFY(writeRequestMetaDataTestFile(metadataFile, TestFixtures::gzipMember(contents)));
    } else if (file == QStringLiteral("truncated")) {
        const QByteArray compressed = TestFixtures::gzipMember(contents);
        QVERIFY(writeRequestMetaDataTestFile(metadataFile, compressed.first(compressed.size() / 2)));
    } else {
        // 1025 members of 64 KiB: 64 KiB more than the limit
        QVERIFY(ArchiveFixtures::writeGzipOfZeros(metadataFile, 1025));
    }
    if (!readable) {
        const QString reason = (file == QStringLiteral("truncated")) ? "truncated" : "size limit exceeded";
        if (events) {
            expectLogMessage("MAVLink.LibEvents.EventHandler", QtWarningMsg,
                             QRegularExpression("Failed to load events JSON metadata file.*" + reason));
        } else {
            expectLogMessage("ComponentInformation.CompInfoParam", QtWarningMsg,
                             QRegularExpression("Metadata json file open failed.*" + reason));
        }
    }
    const QString fileTag = QString::asprintf("%08x_%02i_%i", crc, static_cast<int>(compInfo->type), 0);
    QVERIFY(!manager->fileCache().insert(fileTag, metadataFile).isEmpty());

    RequestMetaDataTypeStateMachine::Timeouts timeouts = RequestMetaDataTypeStateMachine::defaultTimeouts;
    if (end == QStringLiteral("timeout")) {
        timeouts.metaDataLoad = 100;
    }
    auto requestMachine = std::make_unique<RequestMetaDataTypeStateMachine>(manager, nullptr, timeouts);
    QSignalSpy completeSpy(requestMachine.get(), &RequestMetaDataTypeStateMachine::requestComplete);
    QVERIFY(completeSpy.isValid());
    _mockLink->clearReceivedMavCommandCounts();

    TestFixtures::ThreadPoolBlocker blocker;
    QVERIFY(blocker.isBlocking());
    requestMachine->request(compInfo);
    QTRY_VERIFY_WITH_TIMEOUT(requestMachine->findChild<QFutureWatcherBase*>(), TestTimeout::mediumMs());
    QCOMPARE(completeSpy.count(), 0);
    QVERIFY(!vehicle()->healthAndArmingChecksSupported(compId));

    if (end == QStringLiteral("timeout")) {
        expectLogMessage("Utilities.StateMachine.QGCStateMachine", QtWarningMsg,
                         QRegularExpression("^Timeout \".*LoadMetaData\" after 100 ms"));
        QVERIFY(UnitTest::waitForSignal(completeSpy, TestTimeout::longMs(), QStringLiteral("requestComplete")));
        verifyExpectedLogMessage();
    } else if (end == QStringLiteral("delete")) {
        requestMachine.reset();
    }

    blocker.release();
    const bool loaded = (end == QStringLiteral("none")) && readable;
    if (end == QStringLiteral("none")) {
        QVERIFY(UnitTest::waitForSignal(completeSpy, TestTimeout::longMs(), QStringLiteral("requestComplete")));
        if (!readable) {
            verifyExpectedLogMessage();
        }
    } else {
        QVERIFY(QThreadPool::globalInstance()->waitForDone(TestTimeout::mediumMs()));
        QCoreApplication::sendPostedEvents();
    }

    if (events) {
        QCOMPARE(vehicle()->healthAndArmingChecksSupported(compId), loaded);
        if (loaded) {
            QVERIFY_TRUE_WAIT(_mockLink->receivedMavCommandCount(MAV_CMD_RUN_PREARM_CHECKS) == 1,
                              TestTimeout::mediumMs());
        } else {
            QCOMPARE(_mockLink->receivedMavCommandCount(MAV_CMD_RUN_PREARM_CHECKS), 0);
        }
    } else {
        FactMetaData* const metadata =
            param->factMetaDataForName(QStringLiteral("WORKER_PARAM"), FactMetaData::valueTypeFloat);
        QVERIFY(metadata);
        QCOMPARE(metadata->shortDescription() == QStringLiteral("Parsed"), loaded);
    }
    QCOMPARE(manager->fileCache().access(fileTag).isEmpty(), (end == QStringLiteral("none")) && !readable);
}

void RequestMetaDataTypeStateMachineTest::_downloadIsCachedOnceRead_data()
{
    QTest::addColumn<int>("type");
    QTest::addColumn<bool>("readable");

    for (const int type : {COMP_METADATA_TYPE_PARAMETER, COMP_METADATA_TYPE_EVENTS}) {
        const char* const name = (type == COMP_METADATA_TYPE_EVENTS) ? "events" : "parameters";
        QTest::addRow("%s read", name) << type << true;
        QTest::addRow("%s, truncated", name) << type << false;
    }
}

// A downloaded metadata file is cached under its CRC once it has been read and parsed. One that cannot be, such as a
// transfer cut off, is not: it would be taken from the cache at every later connection. It is removed with the other
// downloads of the request.
void RequestMetaDataTypeStateMachineTest::_downloadIsCachedOnceRead()
{
    QFETCH(int, type);
    QFETCH(bool, readable);
    const bool events = (type == COMP_METADATA_TYPE_EVENTS);

    auto* manager = vehicle()->compInfoManager();
    QVERIFY(manager);
    QVERIFY_TRUE_WAIT(!manager->inProgress(), TestTimeout::mediumMs());

    // A component of its own, so that only this test sets its metadata
    constexpr uint8_t compId = 44;
    CompInfoParam* const param = manager->compInfoParam(compId);
    QVERIFY(param);
    CompInfoEvents eventsInfo(compId, vehicle());
    CompInfo* const compInfo = events ? static_cast<CompInfo*>(&eventsInfo) : param;

    const QByteArray contents =
        events ? R"({"version":2,"components":{"44":{"namespace":"cached",)"
                 R"("supported_protocols":["health_and_arming_check"]}}})"
               : R"({"version":1,"parameters":[{"name":"CACHED_PARAM","type":"Float","shortDesc":"Read"}]})";
    const QByteArray compressed = TestFixtures::gzipMember(contents);
    TestFixtures::LocalHttpTestServer server;
    QVERIFY(server.listen());
    server.installHttpResponder(readable ? compressed : compressed.first(compressed.size() / 2));
    const uint32_t crc = QRandomGenerator::global()->generate();
    compInfo->setUriMetaData(server.url(_uniqueServerPath(QStringLiteral("metadata.json.gz"))), crc);
    if (!readable) {
        if (events) {
            expectLogMessage("MAVLink.LibEvents.EventHandler", QtWarningMsg,
                             QRegularExpression("Failed to load events JSON metadata file.*truncated"));
        } else {
            expectLogMessage("ComponentInformation.CompInfoParam", QtWarningMsg,
                             QRegularExpression("Metadata json file open failed.*truncated"));
        }
    }

    RequestMetaDataTypeStateMachine requestMachine(manager, this);
    QSignalSpy completeSpy(&requestMachine, &RequestMetaDataTypeStateMachine::requestComplete);
    requestMachine.request(compInfo);
    QVERIFY(UnitTest::waitForSignal(completeSpy, TestTimeout::longMs(), QStringLiteral("requestComplete")));
    if (!readable) {
        verifyExpectedLogMessage();
    }

    const QString cachedFile =
        manager->fileCache().access(QString::asprintf("%08x_%02i_%i", crc, static_cast<int>(compInfo->type), 0));
    QCOMPARE(cachedFile.isEmpty(), !readable);
    if (readable) {
        QCOMPARE(TestFixtures::readFile(cachedFile), compressed);
    }
    QCOMPARE(QDir(manager->downloadDirectory()).entryList(QDir::Files), QStringList());
}

UT_REGISTER_TEST(RequestMetaDataTypeStateMachineTest, TestLabel::Integration, TestLabel::Vehicle)
