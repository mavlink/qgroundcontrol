#include "RequestMetaDataTypeStateMachineTest.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
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

    RequestMetaDataTypeStateMachine requestMachine(manager, this);
    QSignalSpy completeSpy(&requestMachine, &RequestMetaDataTypeStateMachine::requestComplete);
    QVERIFY(completeSpy.isValid());

    requestMachine.request(param);
    if (completeSpy.count() == 0) {
        QVERIFY(completeSpy.wait(TestTimeout::longMs()));
    }

    QCOMPARE(completeSpy.count(), 1);
    QCOMPARE(_mockLink->receivedMavCommandCount(MAV_CMD_REQUEST_MESSAGE), 0);
    QVERIFY(!requestMachine.active());

    FactMetaData* metadata = param->factMetaDataForName(QStringLiteral("CACHE_HIT_PARAM"), FactMetaData::valueTypeFloat);
    QVERIFY(metadata);
    QCOMPARE(metadata->shortDescription(), QStringLiteral("Loaded from cache"));
}

UT_REGISTER_TEST(RequestMetaDataTypeStateMachineTest, TestLabel::Integration, TestLabel::Vehicle)
