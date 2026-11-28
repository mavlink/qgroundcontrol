#include "RequestMetaDataTypeStateMachineTest.h"

#include <cstring>
#include <memory>

#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtCore/QFileInfo>
#include <QtCore/QFutureWatcher>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
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
#include "ComponentInformationTranslation.h"
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
#include "VehicleFileFetch.h"

namespace {

/// The locale the metadata is translated for, whatever the locale of the machine is: for an English one nothing is
/// translated
const QString metadataTestLocale = QStringLiteral("de_DE");

}  // namespace

void RequestMetaDataTypeStateMachineTest::init()
{
    VehicleTest::init();
    if (QTest::currentTestFailed()) {
        return;
    }

    _manager = vehicle()->compInfoManager();
    QVERIFY(_manager);
    // The requests of the initial connect have ended
    QVERIFY_TRUE_WAIT(!_manager->inProgress(), TestTimeout::mediumMs());
    _manager->translation()->_localeName = metadataTestLocale;
}

bool RequestMetaDataTypeStateMachineTest::_requestAndWait(RequestMetaDataTypeStateMachine& machine, CompInfo* compInfo)
{
    QSignalSpy completeSpy(&machine, &RequestMetaDataTypeStateMachine::requestComplete);
    machine.request(compInfo);
    return !completeSpy.isEmpty() ||
           UnitTest::waitForSignal(completeSpy, TestTimeout::longMs(), QStringLiteral("requestComplete"));
}

QString RequestMetaDataTypeStateMachineTest::_cacheTag(const CompInfo* compInfo, uint32_t crc)
{
    // As ComponentInformationManager::_getFileCacheTag() makes it
    return QString::asprintf("%08x_%02i_%i", crc, static_cast<int>(compInfo->type), 0);
}

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

// A metadata download that is clearly not going to finish in a sane time must be abandoned early: it sits in
// front of parameter load, and on a slow telemetry link the projection is obvious within a few seconds.
void RequestMetaDataTypeStateMachineTest::_slowFtpDownloadAbortsEarly()
{
    auto* param = _manager->compInfoParam(MAV_COMP_ID_AUTOPILOT1);
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

    RequestMetaDataTypeStateMachine requestMachine(_manager, this);
    QElapsedTimer timer;
    timer.start();
    QVERIFY(_requestAndWait(requestMachine, param));
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
    RequestMetaDataTypeStateMachine requestMachine(_manager, this);
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
    auto* param = _manager->compInfoParam(MAV_COMP_ID_AUTOPILOT1);
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

    RequestMetaDataTypeStateMachine requestMachine(_manager, this, timeouts);
    QVERIFY(_requestAndWait(requestMachine, param));
    _mockLink->mockLinkFTP()->setBurstReadDelayMs(0);
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();

    // The download was given up when its state was left
    QCOMPARE(vehicle()->ftpManager()->requestCount(), 0);

    // So the vehicle's FTP session is free: the same file is downloaded now. A download that fails is reported with
    // a warning, which fails the test.
    QVERIFY(_requestAndWait(requestMachine, param));
}

// The same for a download over HTTP: it is aborted when its state is left, which closes its connection, and the next
// request downloads anew.
void RequestMetaDataTypeStateMachineTest::_httpDownloadEndsWithItsState()
{
    // Accepts a connection and never answers it
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    QSignalSpy connectionSpy(&server, &QTcpServer::newConnection);

    auto* param = _manager->compInfoParam(MAV_COMP_ID_AUTOPILOT1);
    QVERIFY(param);
    // Random CRC so the file cache cannot satisfy the request
    param->setUriMetaData(QStringLiteral("http://127.0.0.1:%1/parameter.json").arg(server.serverPort()),
                          QRandomGenerator::global()->generate());

    RequestMetaDataTypeStateMachine::Timeouts timeouts = RequestMetaDataTypeStateMachine::defaultTimeouts;
    timeouts.metaDataDownload = 500;
    RequestMetaDataTypeStateMachine requestMachine(_manager, this, timeouts);

    for (int request = 1; request <= 2; ++request) {
        expectLogMessage("Utilities.StateMachine.QGCStateMachine", QtWarningMsg,
                         QRegularExpression("^Timeout \".*RequestMetaDataJson\" after 500 ms"));
        expectLogMessage("ComponentInformation.RequestMetaDataTypeStateMachine", QtWarningMsg,
                         QRegularExpression("failed to load metadata"));
        QVERIFY(_requestAndWait(requestMachine, param));
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
    auto* param = _manager->compInfoParam(MAV_COMP_ID_AUTOPILOT1);
    QVERIFY(param);

    TestFixtures::LocalHttpTestServer server;
    QVERIFY(server.listen());
    const QString uri = server.url(_uniqueServerPath(QStringLiteral("parameter.json")));
    RequestMetaDataTypeStateMachine requestMachine(_manager, this);

    for (const QString& build : {QStringLiteral("First build"), QStringLiteral("Next build")}) {
        // The server lets the file be cached
        server.installHttpResponder(
            QStringLiteral(R"({"version":1,"parameters":[{"name":"BUILD_PARAM","type":"Float","shortDesc":"%1"}]})")
                .arg(build)
                .toUtf8(),
            200, "application/json", 3600);
        param->setUriMetaData(uri, QRandomGenerator::global()->generate());
        QVERIFY(_requestAndWait(requestMachine, param));

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
    QVERIFY(otherDirectory != _manager->downloadDirectory());

    // The file that a download leaves is cached, and so moved, before the request completes: where it was
    // downloaded to is logged, the same way for both
    const char* const category = "ComponentInformation.RequestMetaDataTypeStateMachine";
    QGCLoggingCategoryManager::instance()->setCategoryEnabled(category, true);
    const auto restoreLogging =
        qScopeGuard([category]() { QGCLoggingCategoryManager::instance()->setCategoryEnabled(category, false); });
    ignoreLogMessage(category, QtDebugMsg, QRegularExpression(QStringLiteral(".*")));

    for (ComponentInformationManager* const requester : {_manager, otherManager.get()}) {
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
        QVERIFY(_requestAndWait(requestMachine, param));
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
    const QString uri = QStringLiteral("mftp://[;comp=1]parameter.json.xz");
    auto* param = _manager->compInfoParam(MAV_COMP_ID_AUTOPILOT1);
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
    RequestMetaDataTypeStateMachine requestMachine(_manager, this, timeouts);
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

void RequestMetaDataTypeStateMachineTest::_transferIsHeldToTheSizeLimit_data()
{
    QTest::addColumn<QString>("transfer");

    QTest::newRow("metadata over MAVLink FTP") << QStringLiteral("ftp");
    QTest::newRow("metadata over HTTP") << QStringLiteral("http");
    QTest::newRow("translation") << QStringLiteral("translation");
}

// The files come from the vehicle, or from where it points to. Their transfer is held to the size their readers take,
// VehicleFileFetch::kMaxSmallFileBytes: a file that announces one byte more fails its transfer, and none of it is
// written. A reader refuses such a file too, but only once all of it is on the disk.
void RequestMetaDataTypeStateMachineTest::_transferIsHeldToTheSizeLimit()
{
    QFETCH(QString, transfer);

    constexpr qint64 announcedBytes = VehicleFileFetch::kMaxSmallFileBytes + 1;
    TestFixtures::LocalHttpTestServer oversizedServer;
    QVERIFY(oversizedServer.listen());
    oversizedServer.installRawResponder(
        "HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Length: " + QByteArray::number(announcedBytes) + "\r\n\r\nx");

    // The failed download of a metadata file is reported in the debug output only
    const char* const category = "ComponentInformation.RequestMetaDataTypeStateMachine";
    QGCLoggingCategoryManager::instance()->setCategoryEnabled(category, true);
    const auto restoreLogging =
        qScopeGuard([category]() { QGCLoggingCategoryManager::instance()->setCategoryEnabled(category, false); });
    ignoreLogMessage(category, QtDebugMsg, QRegularExpression(QStringLiteral(".*")));

    auto* const param = _manager->compInfoParam(MAV_COMP_ID_AUTOPILOT1);
    QVERIFY(param);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    TestFixtures::LocalHttpTestServer server;
    const bool translation = (transfer == QStringLiteral("translation"));
    if (translation) {
        QVERIFY(server.listen());
        _setUpTranslatedParameterMetadata(directory.path(), server,
                                          oversizedServer.url(_uniqueServerPath(QStringLiteral("translation.ts"))));
        if (QTest::currentTestFailed()) {
            return;
        }
        expectLogMessage(category, QtWarningMsg,
                         QRegularExpression(QStringLiteral("Metadata translation failed:.*larger than")));
    } else {
        // Random CRC so the file cache cannot satisfy the request
        const uint32_t crc = QRandomGenerator::global()->generate();
        if (transfer == QStringLiteral("ftp")) {
            _mockLink->mockLinkFTP()->setAnnouncedFileLength(static_cast<uint32_t>(announcedBytes));
            param->setUriMetaData(QStringLiteral("mftp://[;comp=1]parameter.json.xz"), crc);
        } else {
            param->setUriMetaData(oversizedServer.url(_uniqueServerPath(QStringLiteral("parameter.json"))), crc);
        }
        expectLogMessage(category, QtDebugMsg, QRegularExpression(QStringLiteral("download failed:.*larger than")));
        expectLogMessage(category, QtWarningMsg, QRegularExpression(QStringLiteral("failed to load metadata")));
    }
    if (transfer != QStringLiteral("ftp")) {
        ignoreLogMessage("Utilities.QGCFileDownload", QtWarningMsg,
                         QRegularExpression(QStringLiteral("The file is larger than")));
    }

    RequestMetaDataTypeStateMachine requestMachine(_manager, this);
    QVERIFY(_requestAndWait(requestMachine, param));
    verifyExpectedLogMessage();
    if (!translation) {
        verifyExpectedLogMessage();
        QCOMPARE(QDir(_manager->downloadDirectory()).entryList(QDir::Files), QStringList());
        return;
    }

    // Without its translation the metadata is used as it was downloaded
    QVERIFY(!QFileInfo::exists(QDir(_manager->downloadDirectory()).filePath(QStringLiteral("translation.ts"))));
    FactMetaData* const metadata =
        param->factMetaDataForName(QStringLiteral("TRANSLATED_PARAM"), FactMetaData::valueTypeFloat);
    QVERIFY(metadata);
    QCOMPARE(metadata->shortDescription(), QStringLiteral("In English"));
}

void RequestMetaDataTypeStateMachineTest::_fallbackUriIsUsedWhenPrimaryFails_data()
{
    QTest::addColumn<bool>("fallbackHasCrc");

    QTest::newRow("fallback with a CRC") << true;
    QTest::newRow("fallback without a CRC") << false;
}

// Metadata that cannot be downloaded from its URI is downloaded from its fallback URI, and cached under the CRC of
// that one. A fallback is the only metadata file that may have no CRC: it is then not cached, and removed once read.
void RequestMetaDataTypeStateMachineTest::_fallbackUriIsUsedWhenPrimaryFails()
{
    QFETCH(bool, fallbackHasCrc);

    const QByteArray contents(
        R"({"version":1,"parameters":[{"name":"FALLBACK_PARAM","type":"Float","shortDesc":"From the fallback"}]})");
    TestFixtures::LocalHttpTestServer server;
    QVERIFY(server.listen());
    const QString fallbackPath = _uniqueServerPath(QStringLiteral("parameter.json"));
    // The server has no other file
    server.installFileResponder({{fallbackPath, contents}});

    // Random CRCs so the file cache cannot satisfy the request
    const uint32_t crc = QRandomGenerator::global()->generate();
    const uint32_t fallbackCrc = QRandomGenerator::global()->generate();
    QJsonObject parameterType{
        {QStringLiteral("type"), COMP_METADATA_TYPE_PARAMETER},
        {QStringLiteral("uri"), server.url(_uniqueServerPath(QStringLiteral("missing.json")))},
        {QStringLiteral("fileCrc"), static_cast<qint64>(crc)},
        {QStringLiteral("uriFallback"), server.url(fallbackPath)},
    };
    if (fallbackHasCrc) {
        parameterType.insert(QStringLiteral("fileCrcFallback"), static_cast<qint64>(fallbackCrc));
    }
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    _setParameterUrisFromGeneralMetadata(directory.path(), parameterType);
    if (QTest::currentTestFailed()) {
        return;
    }
    auto* const param = _manager->compInfoParam(MAV_COMP_ID_AUTOPILOT1);
    QCOMPARE(param->uriMetaDataFallback(), server.url(fallbackPath));
    const QDir cacheDirectory(ComponentInformationCache::defaultDirectory());
    const QStringList cachedBefore = cacheDirectory.entryList(QDir::Files);

    expectLogMessage("Utilities.QGCFileDownload", QtWarningMsg, QRegularExpression(QStringLiteral("Download error")));
    RequestMetaDataTypeStateMachine requestMachine(_manager, this);
    QVERIFY(_requestAndWait(requestMachine, param));
    verifyExpectedLogMessage();

    FactMetaData* const metadata =
        param->factMetaDataForName(QStringLiteral("FALLBACK_PARAM"), FactMetaData::valueTypeFloat);
    QVERIFY(metadata);
    QCOMPARE(metadata->shortDescription(), QStringLiteral("From the fallback"));

    QVERIFY(_manager->fileCache().access(_cacheTag(param, crc)).isEmpty());
    if (fallbackHasCrc) {
        QCOMPARE(TestFixtures::readFile(_manager->fileCache().access(_cacheTag(param, fallbackCrc))), contents);
    } else {
        QCOMPARE(cacheDirectory.entryList(QDir::Files), cachedBefore);
    }
    QCOMPARE(QDir(_manager->downloadDirectory()).entryList(QDir::Files), QStringList());
}

void RequestMetaDataTypeStateMachineTest::_managerGoneEndsItsRequest_data()
{
    QTest::addColumn<QString>("phase");

    QTest::newRow("downloading over MAVLink FTP") << QStringLiteral("ftp");
    QTest::newRow("downloading over HTTP") << QStringLiteral("http");
    QTest::newRow("reading on a worker") << QStringLiteral("read");
}

// A vehicle that goes away takes its manager with it, whatever the request of the manager is doing then. The request
// ends with the manager: its download is given up, which gives the FTP session of the vehicle back and closes the
// connection, what a worker still reads is dropped, nothing is reported, and the download directory goes.
void RequestMetaDataTypeStateMachineTest::_managerGoneEndsItsRequest()
{
    QFETCH(QString, phase);

    // MockLink answers neither request, so the message below is the answer: it says where the general metadata is
    mockLink()->clearReceivedRequestMessageCounts();
    mockLink()->setRequestMessageNoResponse(MAVLINK_MSG_ID_COMPONENT_METADATA);
    mockLink()->setRequestMessageNoResponse(MAVLINK_MSG_ID_COMPONENT_INFORMATION);

    // A second manager of the vehicle stands for the manager of a vehicle that goes away
    auto otherManager = std::make_unique<ComponentInformationManager>(vehicle());
    const QString otherDirectory = otherManager->downloadDirectory();
    QVERIFY(QFileInfo(otherDirectory).isDir());
    CompInfoGeneral* const general = otherManager->compInfoGeneral(MAV_COMP_ID_AUTOPILOT1);
    QVERIFY(general);

    // Accepts a connection and never answers it
    QTcpServer silentServer;
    QVERIFY(silentServer.listen(QHostAddress::LocalHost));
    QSignalSpy connectionSpy(&silentServer, &QTcpServer::newConnection);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    std::unique_ptr<TestFixtures::ThreadPoolBlocker> blocker;
    // Random CRC so that the file cache has the file only if the test puts it there
    const uint32_t crc = QRandomGenerator::global()->generate();
    const QString cacheTag = _cacheTag(general, crc);
    QString uri = QStringLiteral("http://127.0.0.1:%1/general.json").arg(silentServer.serverPort());
    if (phase == QStringLiteral("ftp")) {
        uri = QStringLiteral("mftp://[;comp=1]parameter.json.xz");
        // One burst a second: the download is far from done when the manager goes
        _mockLink->mockLinkFTP()->setBurstReadDelayMs(1000);
        // The mock's blocking burst delay also starves unrelated commands sent to it during the download
        ignoreLogMessage("Vehicle.StandardModes", QtWarningMsg,
                         QRegularExpression("Failed to retrieve available modes"));
    } else if (phase == QStringLiteral("read")) {
        // From the file cache, and unreadable: a result that reached the request after all would be warned about,
        // and its entry removed
        const QByteArray compressed = TestFixtures::gzipMember(R"({"version":1,"metadataTypes":[]})");
        const QString cachedFile = directory.filePath(QStringLiteral("general.json.gz"));
        QVERIFY(TestFixtures::writeFile(cachedFile, compressed.first(compressed.size() / 2)));
        QVERIFY(!otherManager->fileCache().insert(cacheTag, cachedFile).isEmpty());
        blocker = std::make_unique<TestFixtures::ThreadPoolBlocker>();
        QVERIFY(blocker->isBlocking());
    }

    bool completed = false;
    otherManager->requestAllComponentInformation([](void* called) { *static_cast<bool*>(called) = true; }, &completed);
    QTRY_VERIFY_WITH_TIMEOUT(
        mockLink()->receivedRequestMessageCount(MAV_COMP_ID_AUTOPILOT1, MAVLINK_MSG_ID_COMPONENT_METADATA) > 0,
        TestTimeout::mediumMs());
    char uriField[MAVLINK_MSG_COMPONENT_METADATA_FIELD_URI_LEN] = {};
    const QByteArray uriBytes = uri.toUtf8();
    QVERIFY(uriBytes.size() < static_cast<qsizetype>(sizeof(uriField)));
    memcpy(uriField, uriBytes.constData(), static_cast<size_t>(uriBytes.size()));
    mavlink_message_t message{};
    (void) mavlink_msg_component_metadata_pack_chan(mockLink()->vehicleId(), MAV_COMP_ID_AUTOPILOT1,
                                                    mockLink()->outgoingMavlinkChannel(), &message, 0, crc, uriField);
    mockLink()->respondWithMavlinkMessage(message);
    mockLink()->sendUnexpectedCommandAck(MAV_CMD_REQUEST_MESSAGE, MAV_RESULT_ACCEPTED);

    // The request is under way. The states between the answer and the read of a cached file end as they are entered,
    // so the read has begun once the URI is set
    QTcpSocket* connection = nullptr;
    if (phase == QStringLiteral("ftp")) {
        // The transfer has begun, not only been queued: its file is open in the directory of the manager
        QTRY_VERIFY_WITH_TIMEOUT(!QDir(otherDirectory).entryList(QDir::Files).isEmpty(), TestTimeout::mediumMs());
        QCOMPARE(vehicle()->ftpManager()->requestCount(), 1);
    } else if (phase == QStringLiteral("http")) {
        QTRY_COMPARE_WITH_TIMEOUT(connectionSpy.count(), 1, TestTimeout::mediumMs());
        connection = silentServer.nextPendingConnection();
        QVERIFY(connection);
    } else {
        QTRY_COMPARE_WITH_TIMEOUT(general->uriMetaData(), uri, TestTimeout::mediumMs());
    }
    QVERIFY(!completed);

    otherManager.reset();

    QVERIFY(!QFileInfo::exists(otherDirectory));
    if (phase == QStringLiteral("ftp")) {
        QCOMPARE(vehicle()->ftpManager()->requestCount(), 0);
        // So the FTP session of the vehicle is free: its own manager downloads now. A download that fails is reported
        // with a warning, which fails the test.
        _mockLink->mockLinkFTP()->setBurstReadDelayMs(0);
        auto* const param = _manager->compInfoParam(MAV_COMP_ID_AUTOPILOT1);
        QVERIFY(param);
        param->setUriMetaData(uri, QRandomGenerator::global()->generate());
        RequestMetaDataTypeStateMachine requestMachine(_manager, this);
        QVERIFY(_requestAndWait(requestMachine, param));
    } else if (phase == QStringLiteral("http")) {
        QTRY_COMPARE_WITH_TIMEOUT(connection->state(), QAbstractSocket::UnconnectedState, TestTimeout::shortMs());
    } else {
        blocker->release();
        QVERIFY(QThreadPool::globalInstance()->waitForDone(TestTimeout::mediumMs()));
        QCoreApplication::sendPostedEvents();
        QVERIFY(!_manager->fileCache().access(cacheTag).isEmpty());
    }
    QVERIFY(!completed);
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
    const QString metadataPath = _uniqueServerPath(QStringLiteral("parameter.json"));
    const QString summaryPath =
        _uniqueServerPath(compressed ? QStringLiteral("summary.json.gz") : QStringLiteral("summary.json"));

    files.insert(
        metadataPath,
        R"({"version":1,"translation":{"items":{"parameters":{"list":{"key":"name","translate":["shortDesc"]}}}},)"
        R"("parameters":[{"name":"TRANSLATED_PARAM","type":"Float","shortDesc":"In English"}]})");

    const QJsonObject summary{
        {_manager->translation()->_localeName, QJsonObject{{QStringLiteral("url"), translationUrl}}}};
    const QByteArray summaryJson = QJsonDocument(summary).toJson();
    files.insert(summaryPath, compressed ? TestFixtures::gzipMember(summaryJson) : summaryJson);
    server.installFileResponder(files);

    // Random CRC so the file cache cannot satisfy the request
    const QString summaryUrl = server.url(summaryPath);
    _setParameterUrisFromGeneralMetadata(
        directory, {
                       {QStringLiteral("type"), COMP_METADATA_TYPE_PARAMETER},
                       {QStringLiteral("uri"), server.url(metadataPath)},
                       {QStringLiteral("fileCrc"), static_cast<qint64>(QRandomGenerator::global()->generate())},
                       {QStringLiteral("translationUri"), summaryUrl},
                   });
    if (QTest::currentTestFailed()) {
        return;
    }
    QCOMPARE(_manager->compInfoParam(MAV_COMP_ID_AUTOPILOT1)->uriTranslation(), summaryUrl);
}

void RequestMetaDataTypeStateMachineTest::_setParameterUrisFromGeneralMetadata(const QString& directory,
                                                                               const QJsonObject& parameterType)
{
    const QString generalFile = QDir(directory).filePath(QStringLiteral("general.json"));
    const QJsonObject generalMetadata{
        {QStringLiteral("version"), 1},
        {QStringLiteral("metadataTypes"), QJsonArray{parameterType}},
    };
    QVERIFY(TestFixtures::writeFile(generalFile, QJsonDocument(generalMetadata).toJson()));

    CompInfoGeneral general(MAV_COMP_ID_AUTOPILOT1, vehicle());
    general.setJson(generalFile, CompInfo::parseJsonFile(generalFile, general.jsonSchema()));
    auto* const param = _manager->compInfoParam(MAV_COMP_ID_AUTOPILOT1);
    QVERIFY(param);
    general.setUris(*param);
}

void RequestMetaDataTypeStateMachineTest::_metadataIsTranslatedForLocale_data()
{
    QTest::addColumn<QString>("locale");
    QTest::addColumn<bool>("translationNamed");
    QTest::addColumn<bool>("translationReachable");
    QTest::addColumn<bool>("compressed");
    QTest::addColumn<bool>("translated");

    QTest::newRow("the summary names a translation") << metadataTestLocale << true << true << false << true;
    QTest::newRow("the summary and the translation are compressed")
        << metadataTestLocale << true << true << true << true;
    QTest::newRow("the summary names none") << metadataTestLocale << false << false << false << false;
    QTest::newRow("the translation cannot be downloaded") << metadataTestLocale << true << false << false << false;
    // Metadata is written in English
    QTest::newRow("an English locale is not translated for")
        << QStringLiteral("en_US") << true << true << false << false;
}

// Metadata is translated unless the locale is English, with the file that the translation summary names for the
// locale. The metadata, the summary and that file come over HTTP, and are decompressed where they are read. Without
// a translation the metadata is used as it was downloaded.
void RequestMetaDataTypeStateMachineTest::_metadataIsTranslatedForLocale()
{
    QFETCH(QString, locale);
    QFETCH(bool, translationNamed);
    QFETCH(bool, translationReachable);
    QFETCH(bool, compressed);
    QFETCH(bool, translated);

    _manager->translation()->_localeName = locale;
    if (!translationNamed) {
        expectLogMessage("ComponentInformation.ComponentInformationTranslation", QtWarningMsg,
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

    RequestMetaDataTypeStateMachine requestMachine(_manager, this);
    QSignalSpy completeSpy(&requestMachine, &RequestMetaDataTypeStateMachine::requestComplete);
    QVERIFY(completeSpy.isValid());

    // Well before the translation state would have timed out
    auto* param = _manager->compInfoParam(MAV_COMP_ID_AUTOPILOT1);
    requestMachine.request(param);
    QVERIFY(UnitTest::waitForSignal(completeSpy, TestTimeout::mediumMs(), QStringLiteral("requestComplete")));
    QVERIFY(!requestMachine.inProgress());
    if (!translationNamed) {
        verifyExpectedLogMessage();
    }

    FactMetaData* metadata =
        param->factMetaDataForName(QStringLiteral("TRANSLATED_PARAM"), FactMetaData::valueTypeFloat);
    QVERIFY(metadata);
    QCOMPARE(metadata->shortDescription(), translated ? QStringLiteral("Translated") : QStringLiteral("In English"));
}

// The download of the translation ends with its state as well. It runs on the downloader that the metadata files
// use, so one that ran on refused the summary of the next request.
void RequestMetaDataTypeStateMachineTest::_translationDownloadEndsWithItsState()
{
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
    RequestMetaDataTypeStateMachine requestMachine(_manager, this, timeouts);

    // The second translation reaches the server only when the first download ended with its state. A summary that
    // found the downloader busy was refused with a warning, which fails the test.
    auto* param = _manager->compInfoParam(MAV_COMP_ID_AUTOPILOT1);
    for (int request = 1; request <= 2; ++request) {
        expectLogMessage("Utilities.StateMachine.QGCStateMachine", QtWarningMsg,
                         QRegularExpression("^Timeout \".*RequestTranslate\" after 500 ms"));
        QVERIFY(_requestAndWait(requestMachine, param));
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
    RequestMetaDataTypeStateMachine requestMachine(_manager, this, timeouts);
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
// deleted, which leaves the entry. The events metadata is parsed by libevents. Vehicle asks for the prearm checks
// whenever events metadata is handed to it, read or not, as it always has: only the request after metadata that was
// read is checked here.
void RequestMetaDataTypeStateMachineTest::_metadataIsParsedOnWorker()
{
    QFETCH(int, type);
    QFETCH(QString, end);
    QFETCH(QString, file);
    const bool events = (type == COMP_METADATA_TYPE_EVENTS);
    const bool readable = (file == QStringLiteral("plain")) || (file == QStringLiteral("compressed"));

    // A component of its own, so that only this test sets its metadata
    constexpr uint8_t compId = 43;
    CompInfoParam* const param = _manager->compInfoParam(compId);
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
        QVERIFY(TestFixtures::writeFile(metadataFile, contents));
    } else if (file == QStringLiteral("compressed")) {
        QVERIFY(TestFixtures::writeFile(metadataFile, TestFixtures::gzipMember(contents)));
    } else if (file == QStringLiteral("truncated")) {
        const QByteArray compressed = TestFixtures::gzipMember(contents);
        QVERIFY(TestFixtures::writeFile(metadataFile, compressed.first(compressed.size() / 2)));
    } else {
        // 1025 members of 64 KiB: 64 KiB more than the limit
        QVERIFY(ArchiveFixtures::writeGzipOfZeros(metadataFile, 1025));
    }
    if (!readable) {
        const QString reason = (file == QStringLiteral("truncated")) ? "truncated" : "larger than the size limit";
        if (events) {
            expectLogMessage("MAVLink.LibEvents.EventHandler", QtWarningMsg,
                             QRegularExpression("Failed to load events JSON metadata file.*" + reason));
        } else {
            expectLogMessage("ComponentInformation.CompInfoParam", QtWarningMsg,
                             QRegularExpression("Metadata json file open failed.*" + reason));
        }
    }
    const QString fileTag = _cacheTag(compInfo, crc);
    QVERIFY(!_manager->fileCache().insert(fileTag, metadataFile).isEmpty());

    RequestMetaDataTypeStateMachine::Timeouts timeouts = RequestMetaDataTypeStateMachine::defaultTimeouts;
    if (end == QStringLiteral("timeout")) {
        timeouts.metaDataLoad = 100;
    }
    auto requestMachine = std::make_unique<RequestMetaDataTypeStateMachine>(_manager, nullptr, timeouts);
    QSignalSpy completeSpy(requestMachine.get(), &RequestMetaDataTypeStateMachine::requestComplete);
    QVERIFY(completeSpy.isValid());
    _mockLink->clearReceivedMavCommandCounts();
    const auto requestsForUri = [this]() {
        return _mockLink->receivedRequestMessageCount(MAV_COMP_ID_AUTOPILOT1, MAVLINK_MSG_ID_COMPONENT_METADATA) +
               _mockLink->receivedRequestMessageCount(MAV_COMP_ID_AUTOPILOT1, MAVLINK_MSG_ID_COMPONENT_INFORMATION);
    };
    const int requestsForUriBefore = requestsForUri();

    TestFixtures::ThreadPoolBlocker blocker;
    QVERIFY(blocker.isBlocking());
    requestMachine->request(compInfo);
    QTRY_VERIFY_WITH_TIMEOUT(requestMachine->findChild<QFutureWatcherBase*>(), TestTimeout::mediumMs());
    QCOMPARE(completeSpy.count(), 0);
    // Only the general metadata asks the vehicle where it is: every other type has its URI from there
    QCOMPARE(requestsForUri(), requestsForUriBefore);
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
        QCOMPARE(completeSpy.count(), 1);
        QVERIFY(!requestMachine->inProgress());
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
        }
    } else {
        FactMetaData* const metadata =
            param->factMetaDataForName(QStringLiteral("WORKER_PARAM"), FactMetaData::valueTypeFloat);
        QVERIFY(metadata);
        QCOMPARE(metadata->shortDescription() == QStringLiteral("Parsed"), loaded);
    }
    QCOMPARE(_manager->fileCache().access(fileTag).isEmpty(), (end == QStringLiteral("none")) && !readable);
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

    // A component of its own, so that only this test sets its metadata
    constexpr uint8_t compId = 44;
    CompInfoParam* const param = _manager->compInfoParam(compId);
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

    RequestMetaDataTypeStateMachine requestMachine(_manager, this);
    QVERIFY(_requestAndWait(requestMachine, compInfo));
    if (!readable) {
        verifyExpectedLogMessage();
    }

    const QString cachedFile = _manager->fileCache().access(_cacheTag(compInfo, crc));
    QCOMPARE(cachedFile.isEmpty(), !readable);
    if (readable) {
        QCOMPARE(TestFixtures::readFile(cachedFile), compressed);
    }
    QCOMPARE(QDir(_manager->downloadDirectory()).entryList(QDir::Files), QStringList());
}

UT_REGISTER_TEST(RequestMetaDataTypeStateMachineTest, TestLabel::Integration, TestLabel::Vehicle)
