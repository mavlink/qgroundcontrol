#include "LogDownloadStateMachineTest.h"
#include "LogDownloadController.h"
#include "LogDownloadStateMachine.h"
#include "LogEntry.h"
#include "MockLink.h"
#include "MultiVehicleManager.h"
#include "MAVLinkProtocol.h"
#include "QmlObjectListModel.h"

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

void LogDownloadStateMachineTest::_testStateCreation()
{
    LogDownloadController controller;
    LogDownloadStateMachine stateMachine(&controller, this);

    QCOMPARE(stateMachine.machineName(), QStringLiteral("LogDownload"));
    QVERIFY(!stateMachine.isRequestingList());
    QVERIFY(!stateMachine.isDownloading());
}

void LogDownloadStateMachineTest::_testStartListRequest()
{
    MultiVehicleManager::instance()->init();
    MAVLinkProtocol::instance()->init();

    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    LogDownloadController controller;
    LogDownloadStateMachine* stateMachine = controller._stateMachine;
    QVERIFY(stateMachine);

    QSignalSpy requestingListSpy(stateMachine, &LogDownloadStateMachine::requestingListChanged);

    stateMachine->startListRequest();

    QVERIFY(stateMachine->isRequestingList());
    QCOMPARE(requestingListSpy.count(), 1);

    _disconnectMockLink();
}

void LogDownloadStateMachineTest::_testHandleLogEntry()
{
    MultiVehicleManager::instance()->init();
    MAVLinkProtocol::instance()->init();

    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    LogDownloadController controller;
    LogDownloadStateMachine* stateMachine = controller._stateMachine;
    QVERIFY(stateMachine);

    stateMachine->startListRequest();
    QVERIFY(stateMachine->isRequestingList());

    // Simulate receiving 3 log entries
    stateMachine->handleLogEntry(1640000000, 1024, 0, 3);
    QTest::qWait(50);

    QmlObjectListModel* model = controller._logEntriesModel;
    QVERIFY(model);
    QCOMPARE(model->count(), 3);

    // First entry should be received
    QGCLogEntry* entry = model->value<QGCLogEntry*>(0);
    QVERIFY(entry);
    QVERIFY(entry->received());
    QCOMPARE(entry->size(), 1024u);

    stateMachine->cancel();
    _disconnectMockLink();
}

void LogDownloadStateMachineTest::_testStartDownload()
{
    MultiVehicleManager::instance()->init();
    MAVLinkProtocol::instance()->init();

    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    LogDownloadController controller;
    LogDownloadStateMachine* stateMachine = controller._stateMachine;
    QVERIFY(stateMachine);

    QSignalSpy downloadingSpy(stateMachine, &LogDownloadStateMachine::downloadingChanged);

    // First get some log entries
    stateMachine->startListRequest();
    stateMachine->handleLogEntry(1640000000, 100, 0, 1);
    QTest::qWait(50);

    // Select the first entry
    QmlObjectListModel* model = controller._logEntriesModel;
    QVERIFY(model);
    QCOMPARE(model->count(), 1);
    model->value<QGCLogEntry*>(0)->setSelected(true);

    // Start download
    const QString downloadPath = QDir::tempPath();
    stateMachine->startDownload(downloadPath);

    QVERIFY(stateMachine->isDownloading());
    QVERIFY(downloadingSpy.count() >= 1);

    stateMachine->cancel();
    _disconnectMockLink();
}

void LogDownloadStateMachineTest::_testCancel()
{
    MultiVehicleManager::instance()->init();
    MAVLinkProtocol::instance()->init();

    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    LogDownloadController controller;
    LogDownloadStateMachine* stateMachine = controller._stateMachine;
    QVERIFY(stateMachine);

    stateMachine->startListRequest();
    QVERIFY(stateMachine->isRequestingList());

    stateMachine->cancel();
    QTest::qWait(50);

    // After cancel, should no longer be requesting
    QVERIFY(!stateMachine->isRequestingList());

    _disconnectMockLink();
}

void LogDownloadStateMachineTest::_testListRequestWorkflow()
{
    MultiVehicleManager::instance()->init();
    MAVLinkProtocol::instance()->init();

    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    LogDownloadController controller;
    LogDownloadStateMachine* stateMachine = controller._stateMachine;
    QVERIFY(stateMachine);

    QSignalSpy listCompleteSpy(stateMachine, &LogDownloadStateMachine::listComplete);

    // Start list request
    stateMachine->startListRequest();
    QVERIFY(stateMachine->isRequestingList());

    // Simulate receiving all entries (num_logs = 2)
    stateMachine->handleLogEntry(1640000000, 1024, 0, 2);
    QTest::qWait(50);

    stateMachine->handleLogEntry(1640001000, 2048, 1, 2);
    QTest::qWait(100);

    // Should complete when all entries received
    if (listCompleteSpy.count() == 0) {
        QVERIFY(listCompleteSpy.wait(5000));
    }
    QVERIFY(!stateMachine->isRequestingList());

    // Verify all entries
    QmlObjectListModel* model = controller._logEntriesModel;
    QCOMPARE(model->count(), 2);

    QGCLogEntry* entry0 = model->value<QGCLogEntry*>(0);
    QVERIFY(entry0->received());
    QCOMPARE(entry0->size(), 1024u);

    QGCLogEntry* entry1 = model->value<QGCLogEntry*>(1);
    QVERIFY(entry1->received());
    QCOMPARE(entry1->size(), 2048u);

    _disconnectMockLink();
}
