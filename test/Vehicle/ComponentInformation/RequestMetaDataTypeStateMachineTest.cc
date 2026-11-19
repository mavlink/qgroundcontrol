#include "RequestMetaDataTypeStateMachineTest.h"

#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtCore/QFile>
#include <QtCore/QRandomGenerator>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QStandardPaths>
#include <QtCore/QUuid>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QTcpServer>
#include <QtTest/QSignalSpy>

#include "CompInfoGeneral.h"
#include "CompInfoParam.h"
#include "ComponentInformationCache.h"
#include "ComponentInformationManager.h"
#include "FTPManager.h"
#include "FactMetaData.h"
#include "LinkManager.h"
#include "MockConfiguration.h"
#include "MockLink.h"
#include "MockLinkFTP.h"
#include "MultiVehicleManager.h"
#include "QGCLoggingCategoryManager.h"
#include "RequestMetaDataTypeStateMachine.h"
#include "UnitTest.h"
#include "Vehicle.h"

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

    // A URI of 100 characters fills the field and so has no NUL. This one names nothing to download.
    char uri[MAVLINK_MSG_COMPONENT_METADATA_FIELD_URI_LEN];
    for (char& character : uri) {
        character = 'u';
    }
    ignoreLogMessage("Utilities.QGCFileDownload", QtWarningMsg, QRegularExpression("Download error"));
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
}

// A state that times out is left while its download is still running. The download has to end with the state:
// one that ran on completed whichever state was current when it ended, and kept the vehicle's FTP session.
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

    QSignalSpy downloadCompleteSpy(vehicle()->ftpManager(), &FTPManager::downloadComplete);
    RequestMetaDataTypeStateMachine requestMachine(manager, this, timeouts);
    QSignalSpy completeSpy(&requestMachine, &RequestMetaDataTypeStateMachine::requestComplete);
    QVERIFY(completeSpy.isValid());

    requestMachine.request(param);
    QVERIFY(UnitTest::waitForSignal(completeSpy, TestTimeout::longMs(), QStringLiteral("requestComplete")));
    _mockLink->mockLinkFTP()->setBurstReadDelayMs(0);
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();

    // The download was given up when its state was left, with an error
    QCOMPARE(downloadCompleteSpy.count(), 1);
    QVERIFY(!downloadCompleteSpy.first().at(1).toString().isEmpty());

    // So the vehicle's FTP session is free: the same file is downloaded now, where a request that found the
    // session taken failed with a warning
    requestMachine.request(param);
    QVERIFY(UnitTest::waitForSignal(completeSpy, TestTimeout::longMs(), QStringLiteral("requestComplete")));
    QCOMPARE(downloadCompleteSpy.count(), 2);
    QVERIFY(downloadCompleteSpy.last().at(1).toString().isEmpty());
}

// The same for a download over HTTP, which the machine starts on a downloader that all of its requests share: one
// that is still running refuses the next.
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

    // The second request reaches the server only when the first download ended with its state. One that found
    // the downloader busy was refused with a warning, which fails the test.
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
    }
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

UT_REGISTER_TEST(RequestMetaDataTypeStateMachineTest, TestLabel::Integration, TestLabel::Vehicle)
