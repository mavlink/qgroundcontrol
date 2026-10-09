#include "NTRIPSourceTableControllerTest.h"

#include <chrono>
#include <memory>

#include <QtCore/QAbstractItemModel>
#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtNetwork/QAbstractSocket>
#include <QtTest/QAbstractItemModelTester>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "ManualScheduler.h"
#include "NTRIP/Support/NTRIPTestHelpers.h"
#include "NTRIP/Support/ScriptedNTRIPCaster.h"
#include "NTRIPConfiguration.h"
#include "NTRIPHttpCodec.h"
#include "NTRIPHttpSession.h"
#include "NTRIPSourceTable.h"
#include "NTRIPSourceTableController.h"

using namespace GPSTest;

static const QString kValidTable = QStringLiteral(
    "SOURCETABLE 200 OK\r\n"
    "STR;MP1;Id1;RTCM 3.2;details;2;GPS;NET;USA;40.0;-74.0;0;1;gen;none;B;N;4800;misc\r\n"
    "ENDSOURCETABLE\r\n");

namespace {
/// The session of the fetch in progress; a finished fetch releases its session.
QPointer<NTRIPHttpSession> activeSession(const NTRIPSourceTableController& controller)
{
    return controller.findChild<NTRIPHttpSession*>(Qt::FindDirectChildrenOnly);
}
}  // namespace

void NTRIPSourceTableControllerTest::_invalidConfigReportsError_data()
{
    QTest::addColumn<NTRIPConnectionConfig>("config");
    QTest::newRow("empty-host") << NTRIPConnectionConfig{};
    auto config = connectionConfig();
    config.username = QStringLiteral("bad:user");
    QTest::newRow("colon-in-username") << config;
}

void NTRIPSourceTableControllerTest::_invalidConfigReportsError()
{
    QFETCH(NTRIPConnectionConfig, config);
    NTRIPSourceTableController ctrl;
    QCOMPARE(ctrl.fetchStatus(), NTRIPSourceTableController::FetchStatus::Idle);
    QVERIFY(ctrl.fetchError().isEmpty());
    QVERIFY(ctrl.mountpointModel() != nullptr);
    QCOMPARE(ctrl.mountpointModel()->rowCount(), 0);
    QSignalSpy statusSpy(&ctrl, &NTRIPSourceTableController::fetchStatusChanged);
    QSignalSpy errorSpy(&ctrl, &NTRIPSourceTableController::fetchErrorChanged);

    ctrl.fetch(config);

    QCOMPARE(ctrl.fetchStatus(), NTRIPSourceTableController::FetchStatus::Error);
    QVERIFY(!ctrl.fetchError().isEmpty());
    QCOMPARE(statusSpy.count(), 1);
    QCOMPARE(errorSpy.count(), 1);
}

void NTRIPSourceTableControllerTest::_fetchWarnsForPlaintextCredentials()
{
    NTRIPSourceTableController ctrl;
    QSignalSpy warnings(&ctrl, &NTRIPSourceTableController::securityWarningChanged);
    auto config = connectionConfig();
    ctrl.fetch(config);
    QVERIFY(ctrl.securityWarning().isEmpty());
    QCOMPARE(warnings.count(), 0);

    config.username = QStringLiteral("user");
    config.password = QStringLiteral("secret");
    expectLogMessage("GPS.NTRIPSourceTableController", QtWarningMsg, QRegularExpression(QStringLiteral("without TLS")));
    ctrl.fetch(config);
    verifyExpectedLogMessage();
    QVERIFY(!ctrl.securityWarning().isEmpty());
    QCOMPARE(warnings.count(), 1);

    config.username.clear();
    config.password.clear();
    ctrl.fetch(config);
    QVERIFY(ctrl.securityWarning().isEmpty());
    QCOMPARE(warnings.count(), 2);
}

void NTRIPSourceTableControllerTest::_fetchAbortsOversizedSourceTable()
{
    ScriptedNTRIPCaster caster;
    QVERIFY(caster.isListening());
    NTRIPSourceTableController ctrl;
    ctrl.fetch(caster.connectionConfig());
    QVERIFY(caster.serveSourceTable(QString(9 * 1024 * 1024, QLatin1Char('X'))));

    QCOMPARE_TRUE_WAIT(ctrl.fetchStatus(), NTRIPSourceTableController::FetchStatus::Error, TestTimeout::mediumMs());
    QVERIFY(ctrl.fetchError().contains(QStringLiteral("too large")));
}

void NTRIPSourceTableControllerTest::_fetchErrorInvalidatesCache()
{
    ScriptedNTRIPCaster caster;
    QVERIFY(caster.isListening());
    ManualScheduler scheduler;
    NTRIPSourceTableController ctrl(nullptr, &scheduler);
    const NTRIPConnectionConfig config = caster.connectionConfig(QString());

    ctrl.fetch(config);
    QVERIFY(caster.serveSourceTable(kValidTable));
    QTRY_COMPARE_WITH_TIMEOUT(ctrl.fetchStatus(), NTRIPSourceTableController::FetchStatus::Success,
                              TestTimeout::mediumMs());
    QCOMPARE(ctrl.mountpointModel()->rowCount(), 1);

    // A failed refresh discards the cached table, so the next fetch asks the caster again.
    QVERIFY(scheduler.advanceBy(NTRIPSourceTableController::CACHE_TTL + std::chrono::milliseconds(1)));
    ctrl.fetch(config);
    QVERIFY(caster.dropNextRequest());
    QTRY_COMPARE_WITH_TIMEOUT(ctrl.fetchStatus(), NTRIPSourceTableController::FetchStatus::Error,
                              TestTimeout::mediumMs());
    QCOMPARE(ctrl.mountpointModel()->rowCount(), 0);

    ctrl.fetch(config);
    QCOMPARE(ctrl.fetchStatus(), NTRIPSourceTableController::FetchStatus::InProgress);
}

void NTRIPSourceTableControllerTest::_fetchReportsHttpError_data()
{
    QTest::addColumn<QByteArray>("response");
    QTest::addColumn<bool>("success");
    QTest::addColumn<QString>("detail");
    QTest::newRow("close-delimited-error") << QByteArray("HTTP/1.0 403 Forbidden\r\n\r\nBanned for rapid reconnects")
                                           << false << QStringLiteral("HTTP 403: Forbidden \u2014 Banned");
    QTest::newRow("table-before-framing-error") << okResponse(kValidTable.toUtf8()) + "extra" << true << QString();
}

void NTRIPSourceTableControllerTest::_fetchReportsHttpError()
{
    QFETCH(QByteArray, response);
    QFETCH(bool, success);
    QFETCH(QString, detail);
    ScriptedNTRIPCaster caster;
    QVERIFY(caster.isListening());
    NTRIPSourceTableController ctrl;
    ctrl.fetch(caster.connectionConfig(QString()));
    auto* connection = caster.waitForConnection();
    QVERIFY(connection && connection->peer);
    QVERIFY(connection->waitForRequest().startsWith("GET / HTTP/1.1"));
    QCOMPARE(connection->write(response), response.size());
    connection->disconnectFromHost();
    QTRY_VERIFY_WITH_TIMEOUT(ctrl.fetchStatus() != NTRIPSourceTableController::FetchStatus::InProgress,
                             TestTimeout::mediumMs());
    QCOMPARE(ctrl.fetchStatus(), success ? NTRIPSourceTableController::FetchStatus::Success
                                         : NTRIPSourceTableController::FetchStatus::Error);
    QVERIFY2(ctrl.fetchError().startsWith(detail), qPrintable(ctrl.fetchError()));
    QCOMPARE(ctrl.mountpointModel()->rowCount(), success ? 1 : 0);
}

void NTRIPSourceTableControllerTest::_fetchTimeoutRestartsOnData()
{
    ScriptedNTRIPCaster caster;
    QVERIFY(caster.isListening());
    ManualScheduler scheduler;
    NTRIPSourceTableController ctrl(nullptr, &scheduler);
    ctrl.fetch(caster.connectionConfig(QString()));
    auto* connection = caster.waitForConnection();
    QVERIFY(connection && connection->peer);
    QVERIFY(connection->waitForRequest().startsWith("GET / HTTP/1.1"));
    const auto almostTimeout = NTRIPSourceTableController::FETCH_TIMEOUT - std::chrono::milliseconds(1);
    QVERIFY(scheduler.advanceBy(almostTimeout));

    // A slow caster still delivering the table keeps the fetch alive past the first deadline.
    QSignalSpy received(activeSession(ctrl).data(), &NTRIPHttpSession::responseStarted);
    const QByteArray partial = "SOURCETABLE 200 OK\r\nSTR;MP1;Id1;RTCM 3.2;details;2;GPS;NET;USA;40.0;-74.0;0;1";
    QCOMPARE(connection->write(partial), partial.size());
    QTRY_VERIFY_WITH_TIMEOUT(!received.isEmpty(), TestTimeout::mediumMs());
    QVERIFY(scheduler.advanceBy(almostTimeout));
    QCOMPARE(ctrl.fetchStatus(), NTRIPSourceTableController::FetchStatus::InProgress);

    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
    QCOMPARE(ctrl.fetchStatus(), NTRIPSourceTableController::FetchStatus::Error);
    QVERIFY(ctrl.fetchError().contains(QStringLiteral("timed out")));
}

void NTRIPSourceTableControllerTest::_cacheTtlPreventsFetch()
{
    ScriptedNTRIPCaster caster;
    QVERIFY(caster.isListening());
    ManualScheduler scheduler;
    NTRIPSourceTableController ctrl(nullptr, &scheduler);
    const NTRIPConnectionConfig config = caster.connectionConfig(QString());

    ctrl.fetch(config);
    QCOMPARE(ctrl.fetchStatus(), NTRIPSourceTableController::FetchStatus::InProgress);
    QVERIFY(caster.serveSourceTable(kValidTable));
    QTRY_COMPARE_WITH_TIMEOUT(ctrl.fetchStatus(), NTRIPSourceTableController::FetchStatus::Success,
                              TestTimeout::mediumMs());
    QVERIFY(ctrl.mountpointModel() != nullptr);
    QVERIFY(ctrl.mountpointModel()->rowCount() > 0);

    QSignalSpy statusSpy(&ctrl, &NTRIPSourceTableController::fetchStatusChanged);
    ctrl.fetch(config);

    QCOMPARE(ctrl.fetchStatus(), NTRIPSourceTableController::FetchStatus::Success);
    QVERIFY(statusSpy.isEmpty());

    QVERIFY(scheduler.advanceBy(NTRIPSourceTableController::CACHE_TTL + std::chrono::milliseconds(1)));
    ctrl.fetch(config);
    QCOMPARE(ctrl.fetchStatus(), NTRIPSourceTableController::FetchStatus::InProgress);
}

void NTRIPSourceTableControllerTest::_invalidFetchRetiresPendingFetch()
{
    NTRIPSourceTableController controller;
    controller.fetch(connectionConfig());
    const auto previous = activeSession(controller);
    QVERIFY(previous);
    controller.fetch({});
    QCOMPARE(controller.fetchStatus(), NTRIPSourceTableController::FetchStatus::Error);
    QVERIFY(!activeSession(controller));
    QVERIFY(!previous->isConnected());
    controller._finishFetch();
    QCOMPARE(controller.fetchStatus(), NTRIPSourceTableController::FetchStatus::Error);
    QCOMPARE(controller.mountpointModel()->rowCount(), 0);
}

void NTRIPSourceTableControllerTest::_sourceTableSuccessAndCache()
{
    const QByteArray table =
        "STR;MP1;Id1;RTCM 3.2;details;2;GPS;NET;USA;40.0;-74.0;0;1;gen;none;B;N;4800;misc\r\n"
        "STR;MP2;Id2;RTCM 3.2;details;2;GPS;NET;DEU;52.0;13.0;0;1;gen;none;B;N;4800;misc\r\n"
        "ENDSOURCETABLE\r\n";
    ScriptedNTRIPCaster caster;
    QVERIFY(caster.isListening());
    const auto configuration = caster.connectionConfig(QString());
    NTRIPSourceTableController controller;
    controller.fetch(configuration);
    auto* connection = caster.waitForConnection();
    QVERIFY(connection && connection->peer);
    QVERIFY(connection->waitForRequest().startsWith("GET / HTTP/1.1"));
    const QByteArray response = okResponse(table);
    QCOMPARE(connection->write(response), response.size());
    QTRY_COMPARE_WITH_TIMEOUT(controller.fetchStatus(), NTRIPSourceTableController::FetchStatus::Success,
                              TestTimeout::mediumMs());
    auto* model = controller.mountpointModel();
    QCOMPARE(model->rowCount(), 2);
    const auto distanceAt = [model](int row) {
        return model->data(model->index(row, 0), NTRIPSourceTableModel::DistanceKmRole).toDouble();
    };
    QCOMPARE(distanceAt(0), -1.0);
    controller.fetch(configuration, QGeoCoordinate(40, -74));
    QCOMPARE(controller.fetchStatus(), NTRIPSourceTableController::FetchStatus::Success);
    QVERIFY(!activeSession(controller));
    QVERIFY(controller._cacheStoredAtUs.has_value());
    QCOMPARE(distanceAt(0), 0.0);
    QVERIFY(distanceAt(1) > 1000);
    QCOMPARE(model->data(model->index(0, 0), NTRIPSourceTableModel::MountpointRole).toString(), QStringLiteral("MP1"));

    controller.fetch(configuration, QGeoCoordinate(52, 13));
    QVERIFY(!activeSession(controller));
    QCOMPARE(distanceAt(0), 0.0);
    QVERIFY(distanceAt(1) > 1000);
    QCOMPARE(model->data(model->index(0, 0), NTRIPSourceTableModel::MountpointRole).toString(), QStringLiteral("MP2"));

    controller.fetch(configuration);
    QVERIFY(!activeSession(controller));
    QCOMPARE(distanceAt(0), -1.0);
    QCOMPARE(distanceAt(1), -1.0);

    auto invalid = configuration;
    invalid.mountpoint = QStringLiteral("invalid\r\nmount");
    controller.fetch(invalid);
    QCOMPARE(controller.fetchStatus(), NTRIPSourceTableController::FetchStatus::Error);
    QVERIFY(!controller._cacheStoredAtUs.has_value());
    QCOMPARE(controller.mountpointModel()->rowCount(), 0);
}

void NTRIPSourceTableControllerTest::_sourceTablePublishesSortedRowsOnce()
{
    NTRIPSourceTableController controller;
    auto* model = controller.mountpointModel();
    QAbstractItemModelTester tester(model, QAbstractItemModelTester::FailureReportingMode::QtTest);
    QSignalSpy resets(model, &QAbstractItemModel::modelReset);
    QSignalSpy counts(qobject_cast<NTRIPSourceTableSortModel*>(model), &NTRIPSourceTableSortModel::countChanged);
    const auto makeRow = [](const QString& name, const QString& latitude, const QString& longitude) {
        return QStringLiteral("STR;%1;Id;RTCM 3.2;;2;GPS;NET;USA;%2;%3;0;1;gen;none;B;N;4800\r\n")
            .arg(name, latitude, longitude);
    };
    const QString table = makeRow(QStringLiteral("unknown-a"), QStringLiteral("0"), QStringLiteral("0")) +
                          makeRow(QStringLiteral("far"), QStringLiteral("52"), QStringLiteral("13")) +
                          makeRow(QStringLiteral("near-a"), QStringLiteral("40"), QStringLiteral("-74")) +
                          makeRow(QStringLiteral("unknown-b"), QStringLiteral("999"), QStringLiteral("13")) +
                          makeRow(QStringLiteral("near-b"), QStringLiteral("40"), QStringLiteral("-74")) +
                          QStringLiteral("ENDSOURCETABLE\r\n");
    connect(model, &QAbstractItemModel::modelReset, this, [&]() {
        QStringList names;
        for (int row = 0; row < model->rowCount(); ++row) {
            names.append(model->data(model->index(row, 0), NTRIPSourceTableModel::MountpointRole).toString());
        }
        QCOMPARE(names, (QStringList{QStringLiteral("near-a"), QStringLiteral("near-b"), QStringLiteral("far"),
                                     QStringLiteral("unknown-a"), QStringLiteral("unknown-b")}));
        for (int row : {0, 1, 3, 4}) {
            QCOMPARE(model->data(model->index(row, 0), NTRIPSourceTableModel::DistanceKmRole).toDouble(),
                     row < 2 ? 0.0 : -1.0);
        }
        QVERIFY(model->data(model->index(2, 0), NTRIPSourceTableModel::DistanceKmRole).toDouble() > 1000);
    });
    ScriptedNTRIPCaster caster;
    QVERIFY(caster.isListening());
    controller.fetch(caster.connectionConfig(), QGeoCoordinate(40, -74));
    QVERIFY(caster.serveSourceTable(table));
    QTRY_COMPARE_WITH_TIMEOUT(controller.fetchStatus(), NTRIPSourceTableController::FetchStatus::Success,
                              TestTimeout::mediumMs());
    QCOMPARE(resets.size(), 1);
    QCOMPARE(counts.size(), 1);
    // QML reads the row count through this property.
    QCOMPARE(model->property("count").toInt(), 5);

    // A cached table reorders for a new position without a reset; equal distances keep caster order.
    controller.fetch(caster.connectionConfig(), QGeoCoordinate(52, 13));
    QStringList names;
    for (int row = 0; row < model->rowCount(); ++row) {
        names.append(model->data(model->index(row, 0), NTRIPSourceTableModel::MountpointRole).toString());
    }
    QCOMPARE(names, (QStringList{QStringLiteral("far"), QStringLiteral("near-a"), QStringLiteral("near-b"),
                                 QStringLiteral("unknown-a"), QStringLiteral("unknown-b")}));
    QCOMPARE(resets.size(), 1);
    QCOMPARE(counts.size(), 1);
}

void NTRIPSourceTableControllerTest::_v1SourceTable_data()
{
    QTest::addColumn<QByteArray>("separator");
    QTest::addColumn<bool>("cancel");
    QTest::newRow("without-headers") << QByteArray() << false;
    QTest::newRow("blank-separator") << QByteArray("\r\n") << false;
    QTest::newRow("optional-headers") << QByteArray("Server: loopback\r\n\r\n") << false;
    QTest::newRow("cancel") << QByteArray() << true;
}

void NTRIPSourceTableControllerTest::_v1SourceTable()
{
    QFETCH(QByteArray, separator);
    QFETCH(bool, cancel);
    ScriptedNTRIPCaster caster;
    QVERIFY(caster.isListening());
    const auto configuration = caster.connectionConfig(QString());
    NTRIPSourceTableController controller;
    const QByteArray table =
        "STR;MP;Id;RTCM 3.2;;2;GPS;NET;USA;40;-74;0;1;gen;none;B;N;4800\r\n"
        "ENDSOURCETABLE\r\n";
    int requests = 0;
    controller.fetch(configuration);
    auto* connection = caster.waitForConnection();
    QVERIFY(connection && connection->peer);
    QVERIFY(connection->waitForRequest().startsWith("GET / HTTP/1.1\r\n"));
    ++requests;
    if (cancel) {
        controller.cancel();
    }
    connection->write("SOURCETABLE 200 OK\r\n" + separator + table);
    connection->disconnectFromHost();
    if (cancel) {
        QTRY_COMPARE_WITH_TIMEOUT(requests, 1, TestTimeout::mediumMs());
        QVERIFY(!activeSession(controller));
        QCOMPARE(controller.fetchStatus(), NTRIPSourceTableController::FetchStatus::Idle);
        QCOMPARE(controller.mountpointModel()->rowCount(), 0);
    } else {
        QTRY_COMPARE_WITH_TIMEOUT(controller.fetchStatus(), NTRIPSourceTableController::FetchStatus::Success,
                                  TestTimeout::mediumMs());
        // NTRIP v1 responses need no second request.
        QCOMPARE(requests, 1);
        QCOMPARE(controller.mountpointModel()->rowCount(), 1);
        controller.fetch(configuration);
        QCOMPARE(requests, 1);
        QVERIFY(!activeSession(controller));
    }
}

void NTRIPSourceTableControllerTest::_sourceTableIdentity_data()
{
    QTest::addColumn<NTRIPConnectionConfig>("initial");
    QTest::addColumn<NTRIPConnectionConfig>("replacement");
    QTest::addColumn<bool>("sameCaster");
    QTest::addColumn<bool>("cached");
    const auto add = [](const char* name, const NTRIPConnectionConfig& initial,
                        const NTRIPConnectionConfig& replacement, bool sameCaster = false) {
        for (const bool cached : {false, true}) {
            QTest::addRow("%s-%s", name, cached ? "cached" : "in-flight")
                << initial << replacement << sameCaster << cached;
        }
    };
    // The cache key is the connection without the mountpoint; one changed field stands for any.
    auto initial = connectionConfig();
    initial.username = QStringLiteral("user");
    initial.password = QStringLiteral("pass");
    add("unchanged", initial, initial, true);
    auto replacement = initial;
    replacement.mountpoint = QStringLiteral("OTHER");
    add("mountpoint", initial, replacement, true);
    replacement = initial;
    ++replacement.port;
    add("port", initial, replacement);
}

void NTRIPSourceTableControllerTest::_sourceTableIdentity()
{
    QFETCH(NTRIPConnectionConfig, initial);
    QFETCH(NTRIPConnectionConfig, replacement);
    QFETCH(bool, sameCaster);
    QFETCH(bool, cached);
    // The identity rows use credentials over plain HTTP, which the controller reports.
    ignoreLogMessage("GPS.NTRIPSourceTableController", QtWarningMsg,
                     QRegularExpression(QStringLiteral("credentials without TLS")));
    // The rows vary the request; a scripted caster answers the initial one on its own port.
    ScriptedNTRIPCaster caster;
    QVERIFY(caster.isListening());
    const int portOffset = replacement.port - initial.port;
    initial.port = caster.port();
    replacement.port = caster.port() + portOffset;
    NTRIPSourceTableController controller;
    controller.fetch(initial);
    const auto previous = activeSession(controller);
    QVERIFY(previous);
    if (cached) {
        QVERIFY(
            caster.serveSourceTable(QStringLiteral("STR;MP;Id;RTCM 3.2;;2;GPS;NET;USA;40;-74;0;1;gen;none;B;N;4800\r\n"
                                                   "ENDSOURCETABLE\r\n")));
        QTRY_COMPARE_WITH_TIMEOUT(controller.fetchStatus(), NTRIPSourceTableController::FetchStatus::Success,
                                  TestTimeout::mediumMs());
        QVERIFY(!activeSession(controller));
    }
    controller.fetch(replacement);
    if (sameCaster && cached) {
        QCOMPARE(controller.fetchStatus(), NTRIPSourceTableController::FetchStatus::Success);
        QVERIFY(!activeSession(controller));
        QCOMPARE(controller.mountpointModel()->rowCount(), 1);
        return;
    }
    QCOMPARE(controller.fetchStatus(), NTRIPSourceTableController::FetchStatus::InProgress);
    const auto active = activeSession(controller);
    QVERIFY(active);
    if (sameCaster) {
        QCOMPARE(active, previous);
    } else {
        QVERIFY(active != previous);
        QVERIFY(!previous || !previous->isConnected());
        QVERIFY(!controller._cacheStoredAtUs.has_value());
    }
    const QByteArray authorization =
        "Authorization: Basic " + (replacement.username + QLatin1Char(':') + replacement.password).toUtf8().toBase64() +
        "\r\n";
    QVERIFY(NTRIPHttpRequest::build(controller._lastFetchConfig, NTRIPHttpPurpose::SourceTable)
                .bytes.contains(authorization));
    QCOMPARE(controller._lastFetchConfig.port, replacement.port);
}

void NTRIPSourceTableControllerTest::_abortCallbackSupersedesReplacement()
{
    ScriptedNTRIPCaster caster;
    QVERIFY(caster.isListening());
    NTRIPSourceTableController controller;
    controller.fetch(caster.connectionConfig());
    const auto previous = activeSession(controller);
    const auto replacement = caster.connectionConfig();
    auto* connection = caster.waitForConnection(TestTimeout::shortMs());
    QVERIFY(connection && connection->peer);
    connect(connection->peer, &QAbstractSocket::disconnected, this, [&]() { controller.fetch(replacement); });
    controller.fetch({});
    QTRY_COMPARE_WITH_TIMEOUT(controller.fetchStatus(), NTRIPSourceTableController::FetchStatus::InProgress,
                              TestTimeout::shortMs());
    QVERIFY(activeSession(controller) && activeSession(controller) != previous);
    QCOMPARE(controller._lastFetchConfig.port, replacement.port);
}

void NTRIPSourceTableControllerTest::_socketAbortCallbackRetiresAttempt_data()
{
    QTest::addColumn<bool>("destroy");
    QTest::newRow("replace") << false;
    QTest::newRow("delete") << true;
}

void NTRIPSourceTableControllerTest::_socketAbortCallbackRetiresAttempt()
{
    QFETCH(bool, destroy);
    ScriptedNTRIPCaster caster;
    QVERIFY(caster.isListening());
    const auto configuration = caster.connectionConfig();
    auto controller = std::make_unique<NTRIPSourceTableController>();
    controller->fetch(configuration);
    const auto previous = activeSession(*controller);
    QVERIFY(previous);
    auto* connection = caster.waitForConnection(TestTimeout::shortMs());
    QVERIFY(connection && connection->peer);
    const QPointer<QTcpSocket> peer = connection->peer;
    int callbacks = 0;
    connect(peer, &QTcpSocket::disconnected, this, [&]() {
        ++callbacks;
        if (destroy) {
            controller.reset();
        } else {
            controller->fetch(configuration);
        }
    });
    controller->cancel();
    QTRY_COMPARE_WITH_TIMEOUT(callbacks, 1, TestTimeout::shortMs());
    if (destroy) {
        QVERIFY(!controller);
        QTRY_VERIFY_WITH_TIMEOUT(!peer || peer->state() == QAbstractSocket::UnconnectedState, TestTimeout::shortMs());
    } else {
        QCOMPARE(controller->fetchStatus(), NTRIPSourceTableController::FetchStatus::InProgress);
        QVERIFY(activeSession(*controller) && activeSession(*controller) != previous);
        QVERIFY(controller->fetchError().isEmpty());
    }
}

void NTRIPSourceTableControllerTest::_fetchNotificationReentry_data()
{
    QTest::addColumn<bool>("destroy");
    QTest::newRow("replace") << false;
    QTest::newRow("delete") << true;
}

void NTRIPSourceTableControllerTest::_fetchNotificationReentry()
{
    QFETCH(bool, destroy);
    auto controller = std::make_unique<NTRIPSourceTableController>();
    QPointer<NTRIPSourceTableController> deleted;
    connect(controller.get(), &NTRIPSourceTableController::fetchStatusChanged, this, [&]() {
        if (!controller || controller->fetchStatus() != NTRIPSourceTableController::FetchStatus::InProgress) {
            return;
        }
        if (destroy) {
            deleted = controller.release();
            deleted->deleteLater();
        } else {
            controller->fetch({});
        }
    });
    controller->fetch(connectionConfig());
    if (destroy) {
        QVERIFY(deleted);
        const QPointer<NTRIPHttpSession> session = activeSession(*deleted);
        QVERIFY(session);
        QTRY_VERIFY_WITH_TIMEOUT(!deleted && !session, TestTimeout::shortMs());
        return;
    }
    QCOMPARE(controller->fetchStatus(), NTRIPSourceTableController::FetchStatus::Error);
    QVERIFY(!activeSession(*controller));
}

UT_REGISTER_TEST_LIGHTWEIGHT(NTRIPSourceTableControllerTest, TestLabel::Unit)
