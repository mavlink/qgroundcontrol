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
#include "FTPManager.h"
#include "Fact.h"
#include "FirmwarePlugin.h"
#include "FtpDownloadSession.h"
#include "FtpListingParser.h"
#include "FtpListingSession.h"
#include "FtpTransport.h"
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
    QCOMPARE(ftpTransport->_processFileEntries({QStringLiteral("Ffirst.ulg\t100"), QStringLiteral("Fsecond.ulg\t100")},
                                               QString()),
             2U);
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
