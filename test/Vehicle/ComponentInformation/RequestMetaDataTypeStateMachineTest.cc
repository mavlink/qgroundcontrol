#include "RequestMetaDataTypeStateMachineTest.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QStandardPaths>
#include <QtCore/QTemporaryDir>
#include <QtCore/QUuid>
#include <QtTest/QSignalSpy>

#include "CompInfoGeneral.h"
#include "CompInfoParam.h"
#include "ComponentInformationCache.h"
#include "ComponentInformationManager.h"
#include "FTPManager.h"
#include "FTPManagerJob.h"
#include "FactMetaData.h"
#include "LinkManager.h"
#include "MockConfiguration.h"
#include "MockLinkFTP.h"
#include "MultiVehicleManager.h"
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
    // ArduPilot mock link has no metadata source; this warning is expected.
    ignoreLogMessage("ComponentInformation.RequestMetaDataTypeStateMachine", QtWarningMsg,
                     QRegularExpression("failed to load metadata"));
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

void RequestMetaDataTypeStateMachineTest::_timedOutFileDownloadIsCanceled()
{
    auto* manager = vehicle()->compInfoManager();
    QVERIFY(manager);
    QVERIFY_TRUE_WAIT(!manager->isRunning(), TestTimeout::mediumMs());

    auto* general = manager->compInfoGeneral(MAV_COMP_ID_AUTOPILOT1);
    QVERIFY(general);

    MockLinkFTP* const mockFtp = _mockLink->mockLinkFTP();
    QVERIFY(mockFtp);
    mockFtp->setErrorMode(MockLinkFTP::errModeNoResponse);
    const auto restoreFtpMode = qScopeGuard([mockFtp]() { mockFtp->setErrorMode(MockLinkFTP::errModeNone); });

    RequestMetaDataTypeStateMachine requestMachine(manager, this);
    requestMachine.setTimeoutOverride(QStringLiteral("RequestMetaDataJson"), 50);
    QSignalSpy completeSpy(&requestMachine, &RequestMetaDataTypeStateMachine::requestComplete);
    QVERIFY(completeSpy.isValid());

    requestMachine.request(general);
    if (completeSpy.isEmpty()) {
        QVERIFY(completeSpy.wait(TestTimeout::longMs()));
    }

    QCOMPARE(completeSpy.size(), 1);
    QVERIFY(!requestMachine.active());
    QVERIFY(!requestMachine._ftpDownloadJob);
    QCOMPARE(requestMachine._activeFileDownloadState, nullptr);

    mockFtp->setErrorMode(MockLinkFTP::errModeNone);
    const FTPManager::ListDirectoryStartResult probeResult =
        vehicle()->ftpManager()->startListDirectory(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("/"), 1);
    QCOMPARE(probeResult.error(), FTPManager::StartError::None);
    QVERIFY(probeResult.job());
    probeResult.job()->cancel();
}

void RequestMetaDataTypeStateMachineTest::_staleMessageCallbackIsIgnored()
{
    auto* manager = vehicle()->compInfoManager();
    QVERIFY(manager);

    auto* general = manager->compInfoGeneral(MAV_COMP_ID_AUTOPILOT1);
    auto* param = manager->compInfoParam(MAV_COMP_ID_AUTOPILOT1);
    QVERIFY(general);
    QVERIFY(param);

    auto* requestMachine = new RequestMetaDataTypeStateMachine(manager);
    requestMachine->_compInfo = param;
    requestMachine->_requestGeneration = 2;
    requestMachine->_messageRequestPhase = RequestMetaDataTypeStateMachine::MessageRequestPhase::ComponentMetadata;

    auto* staleContext = new RequestMetaDataTypeStateMachine::MessageRequestContext(
        vehicle(), requestMachine, general, 1, RequestMetaDataTypeStateMachine::MessageRequestPhase::ComponentMetadata);
    mavlink_message_t message = {};
    RequestMetaDataTypeStateMachine::_messageRequestResultHandler(staleContext, MAV_RESULT_ACCEPTED,
                                                                  Vehicle::RequestMessageNoFailure, message);

    QCOMPARE(requestMachine->_compInfo, param);
    QCOMPARE(requestMachine->_messageRequestPhase,
             RequestMetaDataTypeStateMachine::MessageRequestPhase::ComponentMetadata);

    auto* wrongPhaseContext = new RequestMetaDataTypeStateMachine::MessageRequestContext(
        vehicle(), requestMachine, param, requestMachine->_requestGeneration,
        RequestMetaDataTypeStateMachine::MessageRequestPhase::ComponentInformation);
    RequestMetaDataTypeStateMachine::_messageRequestResultHandler(wrongPhaseContext, MAV_RESULT_ACCEPTED,
                                                                  Vehicle::RequestMessageNoFailure, message);
    QCOMPARE(requestMachine->_messageRequestPhase,
             RequestMetaDataTypeStateMachine::MessageRequestPhase::ComponentMetadata);

    auto* destroyedContext = new RequestMetaDataTypeStateMachine::MessageRequestContext(
        vehicle(), requestMachine, param, requestMachine->_requestGeneration,
        RequestMetaDataTypeStateMachine::MessageRequestPhase::ComponentMetadata);
    delete requestMachine;
    RequestMetaDataTypeStateMachine::_messageRequestResultHandler(destroyedContext, MAV_RESULT_ACCEPTED,
                                                                  Vehicle::RequestMessageNoFailure, message);
}

void RequestMetaDataTypeStateMachineTest::_ftpFallbackWaitsForCancellation()
{
    auto* manager = vehicle()->compInfoManager();
    QVERIFY(manager);
    QVERIFY_TRUE_WAIT(!manager->isRunning(), TestTimeout::mediumMs());

    auto* general = manager->compInfoGeneral(MAV_COMP_ID_AUTOPILOT1);
    auto* param = manager->compInfoParam(MAV_COMP_ID_AUTOPILOT1);
    QVERIFY(general);
    QVERIFY(param);

    QTemporaryDir metadataDir;
    QVERIFY(metadataDir.isValid());
    const QString generalMetadataPath = metadataDir.filePath(QStringLiteral("general.json"));
    QFile generalMetadataFile(generalMetadataPath);
    QVERIFY(generalMetadataFile.open(QIODevice::WriteOnly | QIODevice::Truncate));
    const QByteArray generalMetadata = QByteArrayLiteral(
        R"({"version":1,"metadataTypes":[{"type":1,"uri":"mftp://[;comp=1]mocklink-size-32","fileCrc":1,"uriFallback":"mftp://[;comp=1]mocklink-size-64","fileCrcFallback":2}]})");
    QCOMPARE(generalMetadataFile.write(generalMetadata), generalMetadata.size());
    generalMetadataFile.close();
    general->setJson(generalMetadataPath);
    general->setUris(*param);
    QCOMPARE(param->uriMetaDataFallback(), QStringLiteral("mftp://[;comp=1]mocklink-size-64"));

    RequestMetaDataTypeStateMachine requestMachine(manager, this);
    requestMachine._compInfo = param;
    requestMachine._activeAsyncState = nullptr;
    requestMachine._activeSkippableState = requestMachine._stateRequestMetaDataJsonFallback;

    auto* const cancelingJob = new FTPDownloadJob(vehicle()->ftpManager());
    cancelingJob->_finish();
    QVERIFY(cancelingJob);
    requestMachine._trackCancelingFtpDownloadJob(cancelingJob);

    QString fallbackFile;
    requestMachine._requestFile(QString(), false, QStringLiteral("mftp://[;comp=1]mocklink-size-64"), fallbackFile,
                                false);
    QVERIFY(fallbackFile.isEmpty());
    QCOMPARE(requestMachine._cancelingFtpDownloadJob, cancelingJob);

    emit cancelingJob->finished(QString(), QStringLiteral("Aborted"), QString());
    QVERIFY(!requestMachine._cancelingFtpDownloadJob);
    QVERIFY(requestMachine._ftpDownloadJob);
    QVERIFY(fallbackFile.isEmpty());
    QVERIFY(requestMachine._cancelActiveFileDownload());
    delete cancelingJob;
}

UT_REGISTER_TEST(RequestMetaDataTypeStateMachineTest, TestLabel::Integration, TestLabel::Vehicle)
