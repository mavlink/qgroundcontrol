#include "NTRIPSourceTableControllerTest.h"
#include "NTRIPSourceTableController.h"
#include "NTRIPSourceTable.h"
#include "NTRIPSourceTableFetcher.h"
#include "NTRIPSettings.h"
#include "SettingsManager.h"
#include "QmlObjectListModel.h"

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

static const QString kValidTable =
    QStringLiteral(
        "SOURCETABLE 200 OK\r\n"
        "STR;MP1;Id1;RTCM 3.2;details;2;GPS;NET;USA;40.0;-74.0;0;1;gen;none;B;N;4800;misc\r\n"
        "ENDSOURCETABLE\r\n");

static NTRIPSourceTableFetcher *findFetcher(NTRIPSourceTableController *ctrl)
{
    return ctrl->findChild<NTRIPSourceTableFetcher *>();
}

// ---------------------------------------------------------------------------
// Initial state
// ---------------------------------------------------------------------------

void NTRIPSourceTableControllerTest::testInitialState()
{
    NTRIPSourceTableController ctrl;

    QCOMPARE(ctrl.fetchStatus(), NTRIPSourceTableController::FetchStatus::Idle);
    QVERIFY(ctrl.fetchError().isEmpty());
    QVERIFY(ctrl.mountpointModel() == nullptr);
}

// ---------------------------------------------------------------------------
// Fetch with empty host → Error
// ---------------------------------------------------------------------------

void NTRIPSourceTableControllerTest::testFetchEmptyHostTriggersError()
{
    NTRIPSourceTableController ctrl;
    QSignalSpy statusSpy(&ctrl, &NTRIPSourceTableController::fetchStatusChanged);
    QSignalSpy errorSpy(&ctrl, &NTRIPSourceTableController::fetchErrorChanged);

    ctrl.fetch(QString(), 2101, QString(), QString(), false);

    QCOMPARE(ctrl.fetchStatus(), NTRIPSourceTableController::FetchStatus::Error);
    QVERIFY(!ctrl.fetchError().isEmpty());
    QVERIFY(statusSpy.count() >= 1);
    QVERIFY(errorSpy.count() >= 1);
}

// ---------------------------------------------------------------------------
// Fetch with valid host transitions to InProgress
// ---------------------------------------------------------------------------

void NTRIPSourceTableControllerTest::testFetchValidHostGoesInProgress()
{
    NTRIPSourceTableController ctrl;
    QSignalSpy statusSpy(&ctrl, &NTRIPSourceTableController::fetchStatusChanged);

    ctrl.fetch(QStringLiteral("caster.example.com"), 2101, QString(), QString(), false);

    QCOMPARE(ctrl.fetchStatus(), NTRIPSourceTableController::FetchStatus::InProgress);
    QVERIFY(statusSpy.count() >= 1);
    QVERIFY(findFetcher(&ctrl) != nullptr);
}

// ---------------------------------------------------------------------------
// Cache TTL prevents re-fetch when config unchanged
// ---------------------------------------------------------------------------

void NTRIPSourceTableControllerTest::testCacheTtlPreventsFetch()
{
    NTRIPSourceTableController ctrl;

    ctrl.fetch(QStringLiteral("caster.example.com"), 2101, QString(), QString(), false);
    QCOMPARE(ctrl.fetchStatus(), NTRIPSourceTableController::FetchStatus::InProgress);

    auto *fetcher = findFetcher(&ctrl);
    QVERIFY(fetcher);
    emit fetcher->sourceTableReceived(kValidTable);
    emit fetcher->finished();

    QCOMPARE(ctrl.fetchStatus(), NTRIPSourceTableController::FetchStatus::Success);
    QVERIFY(ctrl.mountpointModel() != nullptr);
    QVERIFY(ctrl.mountpointModel()->count() > 0);

    // Wait for deleteLater to process the old fetcher
    QCoreApplication::processEvents();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

    const int fetcherCountBefore = ctrl.findChildren<NTRIPSourceTableFetcher*>().count();

    QSignalSpy statusSpy(&ctrl, &NTRIPSourceTableController::fetchStatusChanged);
    ctrl.fetch(QStringLiteral("caster.example.com"), 2101, QString(), QString(), false);

    QCOMPARE(ctrl.fetchStatus(), NTRIPSourceTableController::FetchStatus::Success);
    QVERIFY(statusSpy.count() >= 1);

    // Cache hit — no new fetcher should be created
    const int fetcherCountAfter = ctrl.findChildren<NTRIPSourceTableFetcher*>().count();
    QCOMPARE(fetcherCountAfter, fetcherCountBefore);
}

// ---------------------------------------------------------------------------
// Config change invalidates cache → InProgress
// ---------------------------------------------------------------------------

void NTRIPSourceTableControllerTest::testConfigChangeInvalidatesCache()
{
    NTRIPSourceTableController ctrl;

    ctrl.fetch(QStringLiteral("caster.example.com"), 2101, QString(), QString(), false);
    auto *fetcher = findFetcher(&ctrl);
    QVERIFY(fetcher);
    emit fetcher->sourceTableReceived(kValidTable);
    emit fetcher->finished();
    QCoreApplication::processEvents();

    QCOMPARE(ctrl.fetchStatus(), NTRIPSourceTableController::FetchStatus::Success);

    ctrl.fetch(QStringLiteral("other.caster.com"), 2101, QString(), QString(), false);
    QCOMPARE(ctrl.fetchStatus(), NTRIPSourceTableController::FetchStatus::InProgress);

    auto *fetcher2 = findFetcher(&ctrl);
    QVERIFY(fetcher2);
    emit fetcher2->sourceTableReceived(kValidTable);
    emit fetcher2->finished();
    QCoreApplication::processEvents();

    QCOMPARE(ctrl.fetchStatus(), NTRIPSourceTableController::FetchStatus::Success);

    ctrl.fetch(QStringLiteral("other.caster.com"), 9999, QString(), QString(), false);
    QCOMPARE(ctrl.fetchStatus(), NTRIPSourceTableController::FetchStatus::InProgress);
}

// ---------------------------------------------------------------------------
// selectMountpoint writes to NTRIPSettings
// ---------------------------------------------------------------------------

void NTRIPSourceTableControllerTest::testSelectMountpointWritesToSettings()
{
    NTRIPSettings *settings = SettingsManager::instance()->ntripSettings();
    QVERIFY(settings);
    QVERIFY(settings->ntripMountpoint());

    const QString original = settings->ntripMountpoint()->rawValue().toString();

    NTRIPSourceTableController ctrl;
    ctrl.selectMountpoint(QStringLiteral("TEST_MP"));

    QCOMPARE(settings->ntripMountpoint()->rawValue().toString(), QStringLiteral("TEST_MP"));

    settings->ntripMountpoint()->setRawValue(original);
}

UT_REGISTER_TEST(NTRIPSourceTableControllerTest, TestLabel::Unit)
