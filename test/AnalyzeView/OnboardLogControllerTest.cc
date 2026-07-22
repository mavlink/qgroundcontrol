#include <QtCore/QAbstractItemModel>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QPersistentModelIndex>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTimer>
#include <QtTest/QAbstractItemModelTester>
#include <QtTest/QSignalSpy>
#include <array>
#include <chrono>
#include <limits>

#include "ArduCopterFirmwarePlugin.h"
#include "FTP/FtpDownloadSession.h"
#include "FTP/FtpListingParser.h"
#include "FTP/FtpListingSession.h"
#include "FTP/FtpTransport.h"
#include "FTPManager.h"
#include "Fact.h"
#include "FirmwarePlugin.h"
#include "LogProtocolDownloadBatchSession.h"
#include "LogProtocolDownloadSession.h"
#include "LogProtocolTransport.h"
#include "MAVLinkProtocol.h"
#include "MavlinkSettings.h"
#include "MockLinkFTP.h"
#include "MultiSignalSpy.h"
#include "MultiVehicleManager.h"
#include "OnboardLogController.h"
#include "OnboardLogDownloadTest.h"
#include "OnboardLogEntry.h"
#include "OnboardLogFileName.h"
#include "OnboardLogModel.h"
#include "SettingsManager.h"
#include "Vehicle.h"

void OnboardLogDownloadTest::_selectionAndSortTest()
{
    Fact* const transportFact = SettingsManager::instance()->mavlinkSettings()->onboardLogTransport();
    const QVariant originalOverride = transportFact->rawValue();
    const auto restoreOverride =
        qScopeGuard([transportFact, originalOverride]() { transportFact->setRawValue(originalOverride); });
    transportFact->setRawValue(1U);

    OnboardLogController controller;
    QCOMPARE(controller.metaObject()->indexOfSignal("requestingListChanged()"), -1);
    QCOMPARE(controller.transportKind(), OnboardLogController::TransportKind::LogProtocol);

    OnboardLogModel* const model = controller.sourceModel();
    QVERIFY(model);
    QAbstractItemModelTester sourceModelTester(model, QAbstractItemModelTester::FailureReportingMode::Fatal);
    QAbstractItemModelTester proxyModelTester(controller.model(),
                                              QAbstractItemModelTester::FailureReportingMode::Fatal);
    model->clearAndDeleteContents();

    OnboardLogEntry* const newest = new OnboardLogEntry(2, QDateTime::fromSecsSinceEpoch(300), 100, true, &controller);
    OnboardLogEntry* const unavailable = new OnboardLogEntry(1, QDateTime(), 0, false, &controller);
    OnboardLogEntry* const oldest = new OnboardLogEntry(0, QDateTime::fromSecsSinceEpoch(100), 100, true, &controller);
    model->append({newest, unavailable, oldest});

    QSignalSpy newestSelectionSpy(newest, &OnboardLogEntry::selectedChanged);
    QSignalSpy unavailableSelectionSpy(unavailable, &OnboardLogEntry::selectedChanged);
    QSignalSpy oldestSelectionSpy(oldest, &OnboardLogEntry::selectedChanged);
    QSignalSpy aggregateSelectionSpy(&controller, &OnboardLogController::selectionChanged);

    controller.selectAll(true);
    QCOMPARE(controller.selectedCount(), 2);
    QVERIFY(controller.allLogsSelected());
    QVERIFY(!unavailable->selected());
    QCOMPARE(newestSelectionSpy.size(), 1);
    QCOMPARE(oldestSelectionSpy.size(), 1);
    QCOMPARE(unavailableSelectionSpy.size(), 0);
    QCOMPARE(aggregateSelectionSpy.size(), 1);

    controller.setSortAscending(true);
    const QAbstractItemModel* const sortedModel = controller.model();
    QCOMPARE(sortedModel->index(0, 0).data(Qt::UserRole).value<QObject*>(), oldest);
    QCOMPARE(sortedModel->index(1, 0).data(Qt::UserRole).value<QObject*>(), newest);
    QCOMPARE(sortedModel->index(2, 0).data(Qt::UserRole).value<QObject*>(), unavailable);
    QCOMPARE(model->value<OnboardLogEntry*>(0), newest);
    QCOMPARE(model->value<OnboardLogEntry*>(1), unavailable);
    QCOMPARE(model->value<OnboardLogEntry*>(2), oldest);

    controller.selectAll(false);
    QCOMPARE(controller.selectedCount(), 0);
    QVERIFY(!controller.allLogsSelected());
    QCOMPARE(newestSelectionSpy.size(), 2);
    QCOMPARE(oldestSelectionSpy.size(), 2);
    QCOMPARE(unavailableSelectionSpy.size(), 0);
    QCOMPARE(aggregateSelectionSpy.size(), 2);

    model->clearAndDeleteContents();
    controller._logTransport->_setListing(true);
    OnboardLogEntry* const firstReceived = new OnboardLogEntry(0, QDateTime(), 100, false, &controller);
    OnboardLogEntry* const secondReceived = new OnboardLogEntry(1, QDateTime(), 100, false, &controller);
    model->append({firstReceived, secondReceived});

    firstReceived->setTime(QDateTime::fromSecsSinceEpoch(300));
    firstReceived->setReceived(true);
    secondReceived->setTime(QDateTime::fromSecsSinceEpoch(100));
    secondReceived->setReceived(true);
    controller._logTransport->_setListing(false);

    QCOMPARE(sortedModel->index(0, 0).data(Qt::UserRole).value<QObject*>(), secondReceived);
    QCOMPARE(sortedModel->index(1, 0).data(Qt::UserRole).value<QObject*>(), firstReceived);
}

void OnboardLogDownloadTest::_typedModelRoleTest()
{
    OnboardLogModel model;
    QAbstractItemModelTester modelTester(&model, QAbstractItemModelTester::FailureReportingMode::Fatal);
    QSignalSpy countSpy(&model, &OnboardLogModel::countChanged);
    QSignalSpy dataChangedSpy(&model, &QAbstractItemModel::dataChanged);

    const QDateTime timestamp = QDateTime::fromSecsSinceEpoch(1234);
    auto* const entry = new OnboardLogEntry(42, timestamp, 1024, true, &model);
    model.append({nullptr, entry, entry});

    QCOMPARE(model.count(), 1);
    QCOMPARE(countSpy.size(), 1);
    QCOMPARE(model.roleNames().value(OnboardLogModel::ObjectRole), QByteArrayLiteral("object"));
    QCOMPARE(model.roleNames().value(OnboardLogModel::IdRole), QByteArrayLiteral("logId"));
    const QModelIndex modelIndex = model.index(0, 0);
    QCOMPARE(modelIndex.data(OnboardLogModel::ObjectRole).value<QObject*>(), entry);
    QCOMPARE(modelIndex.data(OnboardLogModel::IdRole).toUInt(), 42U);
    QCOMPARE(modelIndex.data(OnboardLogModel::TimeRole).toDateTime(), timestamp);
    QCOMPARE(modelIndex.data(OnboardLogModel::SizeRole).toUInt(), 1024U);
    QVERIFY(modelIndex.data(OnboardLogModel::ReceivedRole).toBool());
    QCOMPARE(modelIndex.data(OnboardLogModel::StateRole).value<OnboardLogEntry::State>(),
             OnboardLogEntry::State::Available);

    entry->setSize(2048);
    QCOMPARE(dataChangedSpy.size(), 1);
    QCOMPARE(modelIndex.data(OnboardLogModel::SizeRole).toUInt(), 2048U);
    entry->setSelected(true);
    QCOMPARE(dataChangedSpy.size(), 2);
    QVERIFY(modelIndex.data(OnboardLogModel::SelectedRole).toBool());
    entry->setErrorMessage(QStringLiteral("transfer failed"));
    QCOMPARE(dataChangedSpy.size(), 3);
    QCOMPARE(modelIndex.data(OnboardLogModel::ErrorMessageRole).toString(), QStringLiteral("transfer failed"));

    QCOMPARE(model.removeOne(entry), entry);
    QCOMPARE(model.count(), 0);
    QCOMPARE(countSpy.size(), 2);
    delete entry;

    constexpr int kLargeModelSize = 10000;
    QList<OnboardLogEntry*> largeBatch;
    largeBatch.reserve(kLargeModelSize);
    for (int i = 0; i < kLargeModelSize; ++i) {
        largeBatch.append(new OnboardLogEntry(static_cast<uint>(i), QDateTime(), 0, false, &model));
    }
    model.append(largeBatch);
    QCOMPARE(model.count(), kLargeModelSize);
    OnboardLogEntry* const lastEntry = model.at(kLargeModelSize - 1);
    QVERIFY(lastEntry);
    dataChangedSpy.clear();
    lastEntry->setSize(64);
    QCOMPARE(dataChangedSpy.size(), 1);
    QCOMPARE(model.index(kLargeModelSize - 1, 0).data(OnboardLogModel::SizeRole).toUInt(), 64U);
    model.clearAndDeleteContents();

    auto* const survivingInsert = new OnboardLogEntry(10001, QDateTime(), 0, false, &model);
    auto* const vanishingInsert = new OnboardLogEntry(10002, QDateTime(), 0, false, &model);
    const QMetaObject::Connection insertConnection = connect(&model, &QAbstractItemModel::rowsAboutToBeInserted, &model,
                                                             [vanishingInsert]() { delete vanishingInsert; });
    model.append({survivingInsert, vanishingInsert});
    (void) disconnect(insertConnection);
    QCOMPARE(model.count(), 1);
    QCOMPARE(model.at(0), survivingInsert);

    const QMetaObject::Connection resetConnection = connect(&model, &QAbstractItemModel::modelAboutToBeReset, &model,
                                                            [survivingInsert]() { delete survivingInsert; });
    model.clear();
    (void) disconnect(resetConnection);
    QCOMPARE(model.count(), 0);

    auto* const vanishingRemoval = new OnboardLogEntry(10003, QDateTime(), 0, false, &model);
    model.append(vanishingRemoval);
    const QMetaObject::Connection removeConnection = connect(&model, &QAbstractItemModel::rowsAboutToBeRemoved, &model,
                                                             [vanishingRemoval]() { delete vanishingRemoval; });
    QCOMPARE(model.removeOne(vanishingRemoval), nullptr);
    (void) disconnect(removeConnection);
    QCOMPARE(model.count(), 0);

    QPointer<OnboardLogEntry> clearedOuter = new OnboardLogEntry(10004, QDateTime(), 0, false, &model);
    OnboardLogEntry* const deferredInsert = new OnboardLogEntry(10005, QDateTime(), 0, false, &model);
    bool handledDeferredChange = false;
    const QMetaObject::Connection deferredConnection =
        connect(&model, &QAbstractItemModel::rowsAboutToBeInserted, &model, [&]() {
            if (handledDeferredChange) {
                return;
            }
            handledDeferredChange = true;
            model.clearAndDeleteContents();
            model.append(deferredInsert);
        });
    model.append(clearedOuter);
    (void) disconnect(deferredConnection);

    QVERIFY(handledDeferredChange);
    QCOMPARE(model.count(), 1);
    QCOMPARE(model.at(0), deferredInsert);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(!clearedOuter);
    model.clearAndDeleteContents();
}

void OnboardLogDownloadTest::_modelReentrancyCleanupTest()
{
    OnboardLogModel model;
    QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::Fatal);

    auto* const roleChangedDuringRemoval = new OnboardLogEntry(0, QDateTime(), 0, false, &model);
    model.append(roleChangedDuringRemoval);
    bool structuralChangeActive = false;
    bool nestedDataChange = false;
    const QMetaObject::Connection roleRemoveConnection =
        connect(&model, &QAbstractItemModel::rowsAboutToBeRemoved, &model, [&]() {
            structuralChangeActive = true;
            roleChangedDuringRemoval->setStatus(QStringLiteral("changed during removal"));
        });
    const QMetaObject::Connection roleDataConnection = connect(&model, &QAbstractItemModel::dataChanged, &model, [&]() {
        nestedDataChange = nestedDataChange || structuralChangeActive;
    });
    const QMetaObject::Connection roleRemovedConnection =
        connect(&model, &QAbstractItemModel::rowsRemoved, &model, [&]() { structuralChangeActive = false; });
    QCOMPARE(model.removeOne(roleChangedDuringRemoval), roleChangedDuringRemoval);
    (void) disconnect(roleRemoveConnection);
    (void) disconnect(roleDataConnection);
    (void) disconnect(roleRemovedConnection);
    QVERIFY(!nestedDataChange);
    delete roleChangedDuringRemoval;

    QPointer<OnboardLogEntry> deleteRequestedDuringReset = new OnboardLogEntry(1);
    model.append(deleteRequestedDuringReset);
    bool deleteRequested = false;
    const QMetaObject::Connection resetConnection =
        connect(&model, &QAbstractItemModel::modelAboutToBeReset, &model, [&]() {
            if (!deleteRequested) {
                deleteRequested = true;
                model.clearAndDeleteContents();
            }
        });
    model.clear();
    (void) disconnect(resetConnection);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(deleteRequested);
    QVERIFY(!deleteRequestedDuringReset);

    OnboardLogEntry* const survivor = new OnboardLogEntry(2, QDateTime(), 0, false, &model);
    OnboardLogEntry* const vanishingEntry = new OnboardLogEntry(3, QDateTime(), 0, false, &model);
    const QMetaObject::Connection insertionConnection = connect(&model, &QAbstractItemModel::rowsAboutToBeInserted,
                                                                &model, [vanishingEntry]() { delete vanishingEntry; });
    bool clearedDuringCleanup = false;
    const QMetaObject::Connection countConnection = connect(&model, &OnboardLogModel::countChanged, &model, [&]() {
        if (!clearedDuringCleanup && (model.count() == 1)) {
            clearedDuringCleanup = true;
            model.clear();
        }
    });
    model.append({survivor, vanishingEntry});
    (void) disconnect(insertionConnection);
    (void) disconnect(countConnection);
    QVERIFY(clearedDuringCleanup);
    QCOMPARE(model.count(), 0);

    QList<OnboardLogEntry*> sparseRanges;
    QList<OnboardLogEntry*> sparseEntriesToDelete;
    for (int index = 0; index < 6; ++index) {
        OnboardLogEntry* const entry = new OnboardLogEntry(static_cast<uint>(index), QDateTime(), 0, false, &model);
        sparseRanges.append(entry);
        if ((index == 1) || (index == 4)) {
            sparseEntriesToDelete.append(entry);
        }
    }
    QSignalSpy sparseResetSpy(&model, &QAbstractItemModel::modelReset);
    QSignalSpy sparseRemovalSpy(&model, &QAbstractItemModel::rowsRemoved);
    const QMetaObject::Connection sparseRangesConnection =
        connect(&model, &QAbstractItemModel::rowsAboutToBeInserted, &model, [&sparseEntriesToDelete]() {
            for (OnboardLogEntry* const entry : sparseEntriesToDelete) {
                delete entry;
            }
        });
    model.append(sparseRanges);
    (void) disconnect(sparseRangesConnection);
    QCOMPARE(model.count(), 4);
    QCOMPARE(sparseResetSpy.size(), 0);
    QCOMPARE(sparseRemovalSpy.size(), 2);
    model.clearAndDeleteContents();

    constexpr int kSparseBatchSize = 200;
    QList<OnboardLogEntry*> sparseBatch;
    QList<OnboardLogEntry*> entriesToDelete;
    sparseBatch.reserve(kSparseBatchSize);
    entriesToDelete.reserve(kSparseBatchSize / 2);
    for (int index = 0; index < kSparseBatchSize; ++index) {
        OnboardLogEntry* const entry = new OnboardLogEntry(static_cast<uint>(index), QDateTime(), 0, false, &model);
        sparseBatch.append(entry);
        if ((index % 2) != 0) {
            entriesToDelete.append(entry);
        }
    }
    QSignalSpy modelResetSpy(&model, &QAbstractItemModel::modelReset);
    const QMetaObject::Connection sparseConnection =
        connect(&model, &QAbstractItemModel::rowsAboutToBeInserted, &model, [&entriesToDelete]() {
            for (OnboardLogEntry* const entry : entriesToDelete) {
                delete entry;
            }
        });
    model.append(sparseBatch);
    (void) disconnect(sparseConnection);
    QCOMPARE(model.count(), kSparseBatchSize / 2);
    QCOMPARE(modelResetSpy.size(), 1);
    for (int row = 0; row < model.count(); ++row) {
        QVERIFY(model.at(row));
        QCOMPARE(model.at(row)->id() % 2U, 0U);
    }
    model.clearAndDeleteContents();

    auto* const firstDeferredEntry = new OnboardLogEntry(2000, QDateTime(), 0, false, &model);
    auto* const secondDeferredEntry = new OnboardLogEntry(2001, QDateTime(), 0, false, &model);
    QPointer<OnboardLogEntry> deletedDuringDeferredDataChange =
        new OnboardLogEntry(2002, QDateTime(), 0, false, &model);
    model.append({firstDeferredEntry, secondDeferredEntry, deletedDuringDeferredDataChange});

    bool queuedInitialRoleChange = false;
    const QMetaObject::Connection queueRoleConnection =
        connect(&model, &QAbstractItemModel::rowsAboutToBeInserted, &model, [&]() {
            if (!queuedInitialRoleChange) {
                queuedInitialRoleChange = true;
                firstDeferredEntry->setStatus(QStringLiteral("first deferred change"));
            }
        });
    bool handledDeferredDataChange = false;
    const QMetaObject::Connection deferredDataConnection =
        connect(&model, &QAbstractItemModel::dataChanged, &model, [&](const QModelIndex& topLeft) {
            if (!handledDeferredDataChange && (model.at(topLeft.row()) == firstDeferredEntry)) {
                handledDeferredDataChange = true;
                secondDeferredEntry->setStatus(QStringLiteral("second deferred change"));
                delete deletedDuringDeferredDataChange.data();
            }
        });
    QSignalSpy deferredDataSpy(&model, &QAbstractItemModel::dataChanged);
    auto* const triggerEntry = new OnboardLogEntry(2003, QDateTime(), 0, false, &model);
    model.append(triggerEntry);
    (void) disconnect(queueRoleConnection);
    (void) disconnect(deferredDataConnection);

    QVERIFY(queuedInitialRoleChange);
    QVERIFY(handledDeferredDataChange);
    QVERIFY(!deletedDuringDeferredDataChange);
    QCOMPARE(model.count(), 3);
    QCOMPARE(deferredDataSpy.size(), 2);
    QCOMPARE(secondDeferredEntry->status(), QStringLiteral("second deferred change"));
    model.clearAndDeleteContents();

    {
        QPointer<OnboardLogModel> deletedFromCount = new OnboardLogModel;
        auto* const entry = new OnboardLogEntry(1000, QDateTime(), 0, false, deletedFromCount);
        connect(
            deletedFromCount, &OnboardLogModel::countChanged, this,
            [deletedFromCount]() { delete deletedFromCount.data(); }, Qt::DirectConnection);
        deletedFromCount->append(entry);
        QVERIFY(!deletedFromCount);
    }
}

void OnboardLogDownloadTest::_selectionInvalidationTest()
{
    Fact* const transportFact = SettingsManager::instance()->mavlinkSettings()->onboardLogTransport();
    const QVariant originalOverride = transportFact->rawValue();
    const auto restoreOverride =
        qScopeGuard([transportFact, originalOverride]() { transportFact->setRawValue(originalOverride); });
    transportFact->setRawValue(1U);

    OnboardLogController controller;
    LogProtocolTransport* const logTransport = controller._logTransport;
    QVERIFY(logTransport);
    OnboardLogModel* const logModel = logTransport->model();
    logModel->clearAndDeleteContents();

    (void) logTransport->_listingSession.begin();
    logTransport->_setListing(true);
    QSignalSpy batchedLogInsertSpy(logTransport, &LogProtocolTransport::selectionChanged);
    logTransport->_logEntry(100, 100, 0, 2, 1);
    QCOMPARE(batchedLogInsertSpy.size(), 1);
    QCOMPARE(logModel->count(), 2);
    OnboardLogEntry* const firstLog = logModel->value<OnboardLogEntry*>(0);
    OnboardLogEntry* const secondLog = logModel->value<OnboardLogEntry*>(1);
    QCOMPARE(firstLog->parent(), logTransport);
    QCOMPARE(secondLog->parent(), logTransport);
    QVERIFY(firstLog->received());
    QVERIFY(!secondLog->received());

    controller.selectAll(true);
    QVERIFY(controller.allLogsSelected());

    QSignalSpy eligibilitySpy(&controller, &OnboardLogController::selectionChanged);
    logTransport->_logEntry(200, 100, 1, 2, 1);
    QCOMPARE(eligibilitySpy.size(), 1);
    QCOMPARE(batchedLogInsertSpy.size(), 2);
    QVERIFY(!controller.allLogsSelected());

    transportFact->setRawValue(2U);
    FtpTransport* const ftpTransport = controller._ftpTransport;
    QVERIFY(ftpTransport);
    OnboardLogModel* const ftpModel = ftpTransport->model();
    ftpModel->clearAndDeleteContents();
    (void) ftpTransport->_listingSession.begin(QStringLiteral("/logs"));
    QSignalSpy batchedFtpInsertSpy(ftpTransport, &FtpTransport::selectionChanged);
    const std::optional<uint> processedFtpEntries = ftpTransport->_processFileEntries(
        {QStringLiteral("Ffirst.ulg\t100"), QStringLiteral("Fsecond.ulg\t100")}, QString());
    QVERIFY(processedFtpEntries.has_value());
    QCOMPARE(*processedFtpEntries, 2U);
    ftpTransport->_appendNextLogEntryBatch();
    QCOMPARE(batchedFtpInsertSpy.size(), 1);

    OnboardLogEntry* const selectedEntry = ftpModel->value<OnboardLogEntry*>(0);
    OnboardLogEntry* const removedEntry = ftpModel->value<OnboardLogEntry*>(1);
    selectedEntry->setSelected(true);
    QVERIFY(!controller.allLogsSelected());

    QSignalSpy membershipSpy(&controller, &OnboardLogController::selectionChanged);
    QCOMPARE(ftpModel->removeOne(removedEntry), removedEntry);
    removedEntry->deleteLater();
    QCOMPARE(membershipSpy.size(), 1);
    QVERIFY(controller.allLogsSelected());

    QSignalSpy resetSpy(&controller, &OnboardLogController::selectionChanged);
    ftpModel->clearAndDeleteContents();
    QCOMPARE(resetSpy.size(), 1);
    QVERIFY(!controller.allLogsSelected());

    transportFact->setRawValue(1U);
    logModel->clearAndDeleteContents();
    OnboardLogEntry* const clearingEntry = new OnboardLogEntry(10, QDateTime(), 100, true, logTransport);
    OnboardLogEntry* const untouchedEntry = new OnboardLogEntry(11, QDateTime(), 100, true, logTransport);
    logModel->append({clearingEntry, untouchedEntry});
    const QMetaObject::Connection clearConnection = connect(clearingEntry, &OnboardLogEntry::selectedChanged, logModel,
                                                            &OnboardLogModel::clear, Qt::DirectConnection);
    QSignalSpy reentrantSelectionSpy(&controller, &OnboardLogController::selectionChanged);
    controller.selectAll(true);
    QCOMPARE(logModel->count(), 0);
    QVERIFY(clearingEntry->selected());
    QVERIFY(!untouchedEntry->selected());
    QCOMPARE(reentrantSelectionSpy.size(), 1);
    (void) disconnect(clearConnection);

    OnboardLogEntry* const switchingEntry = new OnboardLogEntry(12, QDateTime(), 100, true, logTransport);
    OnboardLogEntry* const unvisitedEntry = new OnboardLogEntry(13, QDateTime(), 100, true, logTransport);
    logModel->append({switchingEntry, unvisitedEntry});
    const QMetaObject::Connection switchConnection = connect(
        switchingEntry, &OnboardLogEntry::selectedChanged, &controller,
        [transportFact]() { transportFact->setRawValue(2U); }, Qt::DirectConnection);
    controller.selectAll(true);
    QCOMPARE(controller.transportKind(), OnboardLogController::TransportKind::MavlinkFtp);
    QVERIFY(!unvisitedEntry->selected());
    (void) disconnect(switchConnection);
}

void OnboardLogDownloadTest::_vehicleReentrancyTest()
{
    OnboardLogController controller;
    controller._setActiveVehicle(vehicle());

    OnboardLogModel* const logModel = controller._logTransport->model();
    QVERIFY(logModel);
    logModel->clearAndDeleteContents();
    logModel->append(new OnboardLogEntry(0, QDateTime(), 100, true, controller._logTransport));

    bool nestedVehicleChange = false;
    const QMetaObject::Connection reentrantConnection =
        connect(logModel, &QAbstractItemModel::modelReset, &controller, [&controller, this, &nestedVehicleChange]() {
            if (!nestedVehicleChange) {
                nestedVehicleChange = true;
                controller._setActiveVehicle(vehicle());
            }
        });

    controller._setActiveVehicle(nullptr);

    QVERIFY(nestedVehicleChange);
    QCOMPARE(controller._vehicle, vehicle());
    QCOMPARE(controller._logTransport->_vehicle, vehicle());
    QCOMPARE(controller._ftpTransport->_vehicle, vehicle());
    (void) disconnect(reentrantConnection);
}

void OnboardLogDownloadTest::_firmwarePolicyTest()
{
    FirmwarePlugin genericPlugin;
    const FirmwarePlugin::OnboardLogPolicy genericPolicy = genericPlugin.onboardLogPolicy(nullptr);
    QVERIFY(genericPolicy.ftpFallbackDirectory.isEmpty());
    QCOMPARE(genericPolicy.logFileExtension, QStringLiteral("bin"));
    QVERIFY(!genericPolicy.logProtocolIdsAreOneBased);
    QVERIFY(genericPolicy.autoSelectFtp);

    ArduCopterFirmwarePlugin ardupilotPlugin;
    const FirmwarePlugin::OnboardLogPolicy ardupilotPolicy = ardupilotPlugin.onboardLogPolicy(nullptr);
    QCOMPARE(ardupilotPolicy.ftpFallbackDirectory, QStringLiteral("/APM/LOGS"));
    QCOMPARE(ardupilotPolicy.logFileExtension, QStringLiteral("bin"));
    QVERIFY(ardupilotPolicy.logProtocolIdsAreOneBased);
    QVERIFY(!ardupilotPolicy.autoSelectFtp);

    QVERIFY(vehicle()->px4Firmware());
    const FirmwarePlugin::OnboardLogPolicy px4Policy = vehicle()->firmwarePlugin()->onboardLogPolicy(vehicle());
    QCOMPARE(px4Policy.ftpFallbackDirectory, QStringLiteral("/fs/microsd/log"));
    QCOMPARE(px4Policy.logFileExtension, QStringLiteral("ulg"));
    QVERIFY(!px4Policy.logProtocolIdsAreOneBased);
    QVERIFY(px4Policy.autoSelectFtp);
}

void OnboardLogDownloadTest::_transportOverrideTest()
{
    Fact* const transportFact = SettingsManager::instance()->mavlinkSettings()->onboardLogTransport();
    const QVariant originalOverride = transportFact->rawValue();
    const auto restoreOverride =
        qScopeGuard([transportFact, originalOverride]() { transportFact->setRawValue(originalOverride); });

    OnboardLogController controller;

    transportFact->setRawValue(1U);
    QCOMPARE(controller.transportKind(), OnboardLogController::TransportKind::LogProtocol);

    transportFact->setRawValue(2U);
    QCOMPARE(controller.transportKind(), OnboardLogController::TransportKind::MavlinkFtp);

    transportFact->setRawValue(1U);
    QCOMPARE(controller.transportKind(), OnboardLogController::TransportKind::LogProtocol);

    // A forced FTP override must work when the capability bit is absent; that is the override's purpose.
    QVERIFY(!(vehicle()->capabilityBits() & MAV_PROTOCOL_CAPABILITY_FTP));
    transportFact->setRawValue(2U);
    bool forcedFtpListingStarted = false;
    const QMetaObject::Connection listingConnection =
        connect(&controller, &OnboardLogController::busyChanged, this, [&controller, &forcedFtpListingStarted]() {
            forcedFtpListingStarted =
                forcedFtpListingStarted || (controller._active && controller._active->requestingList());
        });
    controller.refresh();
    QVERIFY(forcedFtpListingStarted);
    QVERIFY(controller._loadState == OnboardLogController::LoadState::Loading);
    controller.cancel();
    QVERIFY(controller._loadState == OnboardLogController::LoadState::NotLoaded);

    forcedFtpListingStarted = false;
    controller._loadVehicle = controller._vehicle;
    controller._loadTransport = controller._active;
    controller._loadState = OnboardLogController::LoadState::Failed;
    controller.ensureLoaded();
    QVERIFY(forcedFtpListingStarted);
    controller.cancel();
    (void) disconnect(listingConnection);

    controller._loadVehicle = controller._vehicle;
    controller._loadTransport = controller._active;
    controller._loadState = OnboardLogController::LoadState::Loading;
    controller._activeListingFinished(OnboardLogTransport::ListingResult::Partial);
    QCOMPARE(controller._loadState, OnboardLogController::LoadState::Loaded);

    controller._loadState = OnboardLogController::LoadState::Loading;
    controller._activeListingFinished(OnboardLogTransport::ListingResult::Failed);
    QCOMPARE(controller._loadState, OnboardLogController::LoadState::Failed);

    controller._loadState = OnboardLogController::LoadState::Loading;
    controller._activeListingFinished(OnboardLogTransport::ListingResult::Canceled);
    QCOMPARE(controller._loadState, OnboardLogController::LoadState::NotLoaded);

    constexpr uint64_t ftpCapability = MAV_PROTOCOL_CAPABILITY_FTP;
    QVERIFY(OnboardLogController::_shouldAutoSelectFtp(true, true, ftpCapability));
    QVERIFY(!OnboardLogController::_shouldAutoSelectFtp(false, true, ftpCapability));
    QVERIFY(!OnboardLogController::_shouldAutoSelectFtp(true, false, ftpCapability));
    QVERIFY(!OnboardLogController::_shouldAutoSelectFtp(true, true, 0));

    transportFact->setRawValue(0U);
    const FirmwarePlugin::OnboardLogPolicy policy = vehicle()->firmwarePlugin()->onboardLogPolicy(vehicle());
    const OnboardLogController::TransportKind expectedAutoTransport =
        OnboardLogController::_shouldAutoSelectFtp(policy.autoSelectFtp, vehicle()->capabilitiesKnown(),
                                                   vehicle()->capabilityBits())
            ? OnboardLogController::TransportKind::MavlinkFtp
            : OnboardLogController::TransportKind::LogProtocol;
    QCOMPARE(controller.transportKind(), expectedAutoTransport);
}
