#include "LogDownloadTest.h"
#include "LogDownloadController.h"
#include "MultiVehicleManager.h"
#include "QmlObjectListModel.h"

#include <QtTest/QTest>
#include <QtTest/QSignalSpy>

// ============================================================================
// Controller Creation Tests
// ============================================================================

void LogDownloadTest::_testControllerCreation()
{
    LogDownloadController* controller = new LogDownloadController(this);
    VERIFY_NOT_NULL(controller);
    delete controller;
}

void LogDownloadTest::_testControllerParent()
{
    LogDownloadController* controller = new LogDownloadController(this);
    QCOMPARE(controller->parent(), this);
    delete controller;
}

// ============================================================================
// Model Tests
// ============================================================================

void LogDownloadTest::_testModelExists()
{
    LogDownloadController* controller = new LogDownloadController(this);

    // Access model via QML property
    QmlObjectListModel* model = controller->property("model").value<QmlObjectListModel*>();
    VERIFY_NOT_NULL(model);

    delete controller;
}

void LogDownloadTest::_testModelInitiallyEmpty()
{
    LogDownloadController* controller = new LogDownloadController(this);

    QmlObjectListModel* model = controller->property("model").value<QmlObjectListModel*>();
    VERIFY_NOT_NULL(model);
    QCOMPARE_EQ(model->count(), 0);

    delete controller;
}

// ============================================================================
// Initial State Tests
// ============================================================================

void LogDownloadTest::_testInitialRequestingListFalse()
{
    LogDownloadController* controller = new LogDownloadController(this);

    bool requestingList = controller->property("requestingList").toBool();
    QVERIFY(!requestingList);

    delete controller;
}

void LogDownloadTest::_testInitialDownloadingLogsFalse()
{
    LogDownloadController* controller = new LogDownloadController(this);

    bool downloadingLogs = controller->property("downloadingLogs").toBool();
    QVERIFY(!downloadingLogs);

    delete controller;
}

void LogDownloadTest::_testInitialCompressLogsFalse()
{
    LogDownloadController* controller = new LogDownloadController(this);

    QVERIFY(!controller->compressLogs());

    delete controller;
}

void LogDownloadTest::_testInitialCompressingFalse()
{
    LogDownloadController* controller = new LogDownloadController(this);

    QVERIFY(!controller->compressing());

    delete controller;
}

void LogDownloadTest::_testInitialCompressionProgressZero()
{
    LogDownloadController* controller = new LogDownloadController(this);

    QCOMPARE_EQ(controller->compressionProgress(), 0.0f);

    delete controller;
}

// ============================================================================
// CompressLogs Property Tests
// ============================================================================

void LogDownloadTest::_testSetCompressLogsTrue()
{
    LogDownloadController* controller = new LogDownloadController(this);

    controller->setCompressLogs(true);
    QVERIFY(controller->compressLogs());

    delete controller;
}

void LogDownloadTest::_testSetCompressLogsFalse()
{
    LogDownloadController* controller = new LogDownloadController(this);

    controller->setCompressLogs(true);
    QVERIFY(controller->compressLogs());

    controller->setCompressLogs(false);
    QVERIFY(!controller->compressLogs());

    delete controller;
}

void LogDownloadTest::_testCompressLogsSignal()
{
    LogDownloadController* controller = new LogDownloadController(this);

    QSignalSpy spy(controller, &LogDownloadController::compressLogsChanged);
    QVERIFY(spy.isValid());

    controller->setCompressLogs(true);
    QCOMPARE_EQ(spy.count(), 1);

    delete controller;
}

// ============================================================================
// Signal Existence Tests
// ============================================================================

void LogDownloadTest::_testRequestingListChangedSignal()
{
    LogDownloadController* controller = new LogDownloadController(this);

    QSignalSpy spy(controller, &LogDownloadController::requestingListChanged);
    QVERIFY(spy.isValid());

    delete controller;
}

void LogDownloadTest::_testDownloadingLogsChangedSignal()
{
    LogDownloadController* controller = new LogDownloadController(this);

    QSignalSpy spy(controller, &LogDownloadController::downloadingLogsChanged);
    QVERIFY(spy.isValid());

    delete controller;
}

void LogDownloadTest::_testSelectionChangedSignal()
{
    LogDownloadController* controller = new LogDownloadController(this);

    QSignalSpy spy(controller, &LogDownloadController::selectionChanged);
    QVERIFY(spy.isValid());

    delete controller;
}

void LogDownloadTest::_testCompressingChangedSignal()
{
    LogDownloadController* controller = new LogDownloadController(this);

    QSignalSpy spy(controller, &LogDownloadController::compressingChanged);
    QVERIFY(spy.isValid());

    delete controller;
}

void LogDownloadTest::_testCompressionProgressChangedSignal()
{
    LogDownloadController* controller = new LogDownloadController(this);

    QSignalSpy spy(controller, &LogDownloadController::compressionProgressChanged);
    QVERIFY(spy.isValid());

    delete controller;
}

void LogDownloadTest::_testCompressionCompleteSignal()
{
    LogDownloadController* controller = new LogDownloadController(this);

    QSignalSpy spy(controller, &LogDownloadController::compressionComplete);
    QVERIFY(spy.isValid());

    delete controller;
}

// ============================================================================
// Q_INVOKABLE Method Tests
// ============================================================================

void LogDownloadTest::_testRefreshMethod()
{
    LogDownloadController* controller = new LogDownloadController(this);

    // Verify refresh() can be called without crashing
    // The actual refresh behavior depends on vehicle connection
    controller->refresh();

    delete controller;
}

void LogDownloadTest::_testCancelMethod()
{
    LogDownloadController* controller = new LogDownloadController(this);

    // Verify cancel() can be called without crashing
    controller->cancel();

    delete controller;
}

void LogDownloadTest::_testCancelCompressionMethod()
{
    LogDownloadController* controller = new LogDownloadController(this);

    // Verify cancelCompression() can be called without crashing
    controller->cancelCompression();

    delete controller;
}
