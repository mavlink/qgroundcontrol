#include "RequestMetaDataTypeStateMachineTest.h"

#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtCore/QFile>
#include <QtCore/QRandomGenerator>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QStandardPaths>
#include <QtCore/QUuid>
#include <QtTest/QSignalSpy>

#include "CompInfoGeneral.h"
#include "CompInfoParam.h"
#include "ComponentInformationCache.h"
#include "ComponentInformationManager.h"
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

    auto* general = manager->compInfoGeneral(MAV_COMP_ID_AUTOPILOT1);
    QVERIFY(general);
    requestMachine.request(general);
    QCOMPARE(requestMachine.typeToString(), QStringLiteral("COMP_METADATA_TYPE_GENERAL"));

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
    QVERIFY(!requestMachine.active());
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
    QVERIFY(!requestMachine.active());
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
    QVERIFY(!requestMachine.active());
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
    QVERIFY(initialConnectCompleteSpy.wait(TestTimeout::longMs()) || _vehicle->isInitialConnectComplete());

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
    QVERIFY(!requestMachine.active());

    _disconnectMockLink();
}

void RequestMetaDataTypeStateMachineTest::_requestUsesCachedMetadataForParameter()
{
    auto* manager = vehicle()->compInfoManager();
    QVERIFY(manager);
    QVERIFY_TRUE_WAIT(!manager->isRunning(), TestTimeout::mediumMs());

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
    QVERIFY(!requestMachine.active());

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
    QVERIFY_TRUE_WAIT(!manager->isRunning(), TestTimeout::mediumMs());

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
    QVERIFY(!requestMachine.active());
}

UT_REGISTER_TEST(RequestMetaDataTypeStateMachineTest, TestLabel::Integration, TestLabel::Vehicle)
