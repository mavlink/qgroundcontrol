#include <algorithm>
#include <memory>
#include <optional>

#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtGui/QPixmap>
#include <QtNetwork/QAbstractSocket>
#include <QtNetwork/QSslSocket>
#include <QtTest/QAbstractItemModelTester>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "NTRIPHttpSession.h"
#include "NTRIPSourceTable.h"
#include "NTRIPSourceTableController.h"
#include "NTRIPSourceTableControllerTest.h"
#include "NTRIPTestSupport.h"
#include "ScriptedNtripCaster.h"

using namespace NTRIPTestSupport;

void NTRIPSourceTableControllerTest::invalidFetchRetiresPendingFetch()
{
    NTRIPSourceTableController controller;
    controller.fetch(config());
    const auto previous = controller._activeSession();
    QVERIFY(previous);
    controller.fetch({});
    QCOMPARE(controller.fetchStatus(), NTRIPSourceTableController::FetchStatus::Error);
    QVERIFY(!controller._activeSession());
    QVERIFY(!previous->isConnected());
    controller._finishFetch();
    QCOMPARE(controller.fetchStatus(), NTRIPSourceTableController::FetchStatus::Error);
    QCOMPARE(controller.mountpointModel()->rowCount(), 0);
}

void NTRIPSourceTableControllerTest::sourceTableSuccessAndCache()
{
    const QByteArray table =
        "STR;MP1;Id1;RTCM 3.2;details;2;GPS;NET;USA;40.0;-74.0;0;1;gen;none;B;N;4800;misc\r\n"
        "STR;MP2;Id2;RTCM 3.2;details;2;GPS;NET;DEU;52.0;13.0;0;1;gen;none;B;N;4800;misc\r\n"
        "ENDSOURCETABLE\r\n";
    ScriptedNtripCaster caster;
    QVERIFY(caster.isListening());
    auto configuration = caster.connectionConfig();
    configuration.mountpoint.clear();
    NTRIPSourceTableController controller;
    controller.fetch(configuration);
    auto* connection = caster.waitForConnection();
    QVERIFY(connection && connection->peer);
    QVERIFY(connection->waitForRequest().startsWith("GET / HTTP/1.1"));
    const QByteArray response =
        "HTTP/1.1 200 OK\r\nContent-Length: " + QByteArray::number(table.size()) + "\r\n\r\n" + table;
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
    QVERIFY(!controller._activeSession());
    QVERIFY(controller._cacheStoredAtUs.has_value());
    QCOMPARE(distanceAt(0), 0.0);
    QVERIFY(distanceAt(1) > 1000);
    QCOMPARE(model->data(model->index(0, 0), NTRIPSourceTableModel::MountpointRole).toString(), QStringLiteral("MP1"));

    controller.fetch(configuration, QGeoCoordinate(52, 13));
    QVERIFY(!controller._activeSession());
    QCOMPARE(distanceAt(0), 0.0);
    QVERIFY(distanceAt(1) > 1000);
    QCOMPARE(model->data(model->index(0, 0), NTRIPSourceTableModel::MountpointRole).toString(), QStringLiteral("MP2"));

    controller.fetch(configuration);
    QVERIFY(!controller._activeSession());
    QCOMPARE(distanceAt(0), -1.0);
    QCOMPARE(distanceAt(1), -1.0);

    auto invalid = configuration;
    invalid.mountpoint = QStringLiteral("invalid\r\nmount");
    controller.fetch(invalid);
    QCOMPARE(controller.fetchStatus(), NTRIPSourceTableController::FetchStatus::Error);
    QVERIFY(!controller._cacheStoredAtUs.has_value());
    QCOMPARE(controller.mountpointModel()->rowCount(), 0);
}

void NTRIPSourceTableControllerTest::sourceTablePublishesSortedRowsOnce()
{
    NTRIPSourceTableController controller;
    auto* model = controller.mountpointModel();
    QAbstractItemModelTester tester(model, QAbstractItemModelTester::FailureReportingMode::QtTest);
    QSignalSpy resets(model, &QAbstractItemModel::modelReset);
    QSignalSpy counts(qobject_cast<NTRIPSourceTableModel*>(model), &NTRIPSourceTableModel::countChanged);
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
    controller.fetch(config(), QGeoCoordinate(40, -74));
    controller.injectSourceTableForTest(table);
    QCOMPARE(controller.fetchStatus(), NTRIPSourceTableController::FetchStatus::Success);
    QCOMPARE(resets.size(), 1);
    QCOMPARE(counts.size(), 1);
}

void NTRIPSourceTableControllerTest::v1SourceTable_data()
{
    QTest::addColumn<QByteArray>("separator");
    QTest::addColumn<bool>("cancel");
    QTest::newRow("without-headers") << QByteArray() << false;
    QTest::newRow("blank-separator") << QByteArray("\r\n") << false;
    QTest::newRow("optional-headers") << QByteArray("Server: loopback\r\n\r\n") << false;
    QTest::newRow("cancel") << QByteArray() << true;
}

void NTRIPSourceTableControllerTest::v1SourceTable()
{
    QFETCH(QByteArray, separator);
    QFETCH(bool, cancel);
    ScriptedNtripCaster caster;
    QVERIFY(caster.isListening());
    auto configuration = caster.connectionConfig();
    configuration.mountpoint.clear();
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
        QVERIFY(!controller._activeSession());
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
        QVERIFY(!controller._activeSession());
    }
}

void NTRIPSourceTableControllerTest::sourceTableIdentity_data()
{
    QTest::addColumn<NTRIPConnectionConfig>("initial");
    QTest::addColumn<NTRIPConnectionConfig>("replacement");
    QTest::addColumn<bool>("sameCaster");
    QTest::addColumn<bool>("cached");
    const auto add = [](const char* name, const NTRIPConnectionConfig& initial,
                        const NTRIPConnectionConfig& replacement, bool sameCaster = false) {
        for (const bool cached : {false, true}) {
            QTest::newRow((QByteArray(name) + (cached ? "-cached" : "-in-flight")).constData())
                << initial << replacement << sameCaster << cached;
        }
    };
    auto initial = config();
    initial.username = QStringLiteral("user");
    initial.password = QStringLiteral("pass");
    add("unchanged", initial, initial, true);
    auto replacement = initial;
    replacement.mountpoint = QStringLiteral("OTHER");
    add("mountpoint", initial, replacement, true);
    replacement = initial;
    replacement.host = QStringLiteral("localhost");
    add("host", initial, replacement);
    auto hostname = replacement;
    replacement.host = QStringLiteral("LOCALHOST");
    add("host-case", hostname, replacement);
    replacement = initial;
    ++replacement.port;
    add("port", initial, replacement);
    replacement = initial;
    replacement.username = QStringLiteral("another-user");
    add("username", initial, replacement);
    replacement = initial;
    replacement.password = QStringLiteral("another-password");
    add("password", initial, replacement);
    replacement = initial;
    replacement.useTls = true;
    add("tls", initial, replacement);
    replacement = initial;
    replacement.allowSelfSignedCerts = true;
    add("self-signed-policy", initial, replacement);

    initial.username = QStringLiteral("%4");
    replacement = initial;
    replacement.username = initial.password;
    add("username-password-placeholder", initial, replacement);
    initial.username = QStringLiteral("%5");
    replacement = initial;
    replacement.username = QStringLiteral("0");
    add("username-tls-placeholder", initial, replacement);
    initial.username = QStringLiteral("user");
    initial.password = QStringLiteral("%5");
    replacement = initial;
    replacement.password = QStringLiteral("0");
    add("password-tls-placeholder", initial, replacement);
    initial.username = QStringLiteral("%1%2%4");
    initial.password = QStringLiteral("%1%5%6");
    add("literal-placeholders", initial, initial, true);

    initial.username = QStringLiteral(
        "a\x1f"
        "b");
    initial.password = QStringLiteral("c");
    replacement = initial;
    replacement.username = QStringLiteral("a");
    replacement.password = QStringLiteral(
        "b\x1f"
        "c");
    add("credential-delimiters", initial, replacement);
}

void NTRIPSourceTableControllerTest::sourceTableIdentity()
{
    QFETCH(NTRIPConnectionConfig, initial);
    QFETCH(NTRIPConnectionConfig, replacement);
    QFETCH(bool, sameCaster);
    QFETCH(bool, cached);
    // The identity rows use credentials over plain HTTP, which the controller reports.
    ignoreLogMessage("GPS.NTRIP.NTRIPSourceTableController", QtWarningMsg,
                     QRegularExpression(QStringLiteral("credentials without TLS")));
    NTRIPSourceTableController controller;
    controller.fetch(initial);
    const auto previous = controller._activeSession();
    QVERIFY(previous);
    if (cached) {
        controller.injectSourceTableForTest(
            QStringLiteral("STR;MP;Id;RTCM 3.2;;2;GPS;NET;USA;40;-74;0;1;gen;none;B;N;4800\r\n"
                           "ENDSOURCETABLE\r\n"));
        QCOMPARE(controller.fetchStatus(), NTRIPSourceTableController::FetchStatus::Success);
        QVERIFY(!controller._activeSession());
    }
    const auto revision = controller._fetchRevision.value();
    controller.fetch(replacement);
    if (sameCaster && cached) {
        QCOMPARE(controller.fetchStatus(), NTRIPSourceTableController::FetchStatus::Success);
        QVERIFY(!controller._activeSession());
        QCOMPARE(controller.mountpointModel()->rowCount(), 1);
        return;
    }
    QCOMPARE(controller.fetchStatus(), NTRIPSourceTableController::FetchStatus::InProgress);
    const auto active = controller._activeSession();
    QVERIFY(active);
    if (sameCaster) {
        QCOMPARE(active, previous);
        QCOMPARE(controller._fetchRevision.value(), revision);
    } else {
        QVERIFY(active != previous);
        QVERIFY(controller._fetchRevision.value() > revision);
        QVERIFY(!previous || !previous->isConnected());
        QVERIFY(!controller._cacheStoredAtUs.has_value());
    }
    const QByteArray authorization =
        "Authorization: Basic " + (replacement.username + QLatin1Char(':') + replacement.password).toUtf8().toBase64() +
        "\r\n";
    QVERIFY(controller._activeRequest().contains(authorization));
    QCOMPARE(controller._lastFetchConfig.port, replacement.port);
}

void NTRIPSourceTableControllerTest::abortCallbackSupersedesReplacement()
{
    ScriptedNtripCaster caster;
    QVERIFY(caster.isListening());
    NTRIPSourceTableController controller;
    auto configuration = config();
    configuration.port = caster.port();
    controller.fetch(configuration);
    const auto previous = controller._activeSession();
    auto replacement = config();
    replacement.port = caster.port();
    auto* connection = caster.waitForConnection(TestTimeout::shortMs());
    QVERIFY(connection && connection->peer);
    connect(connection->peer, &QAbstractSocket::disconnected, this, [&]() { controller.fetch(replacement); });
    controller.fetch({});
    QTRY_COMPARE_WITH_TIMEOUT(controller.fetchStatus(), NTRIPSourceTableController::FetchStatus::InProgress,
                              TestTimeout::shortMs());
    QVERIFY(controller._activeSession() && controller._activeSession() != previous);
    QCOMPARE(controller._lastFetchConfig.port, replacement.port);
}

void NTRIPSourceTableControllerTest::socketAbortCallbackRetiresAttempt_data()
{
    QTest::addColumn<bool>("destroy");
    QTest::newRow("replace") << false;
    QTest::newRow("delete") << true;
}

void NTRIPSourceTableControllerTest::socketAbortCallbackRetiresAttempt()
{
    QFETCH(bool, destroy);
    ScriptedNtripCaster caster;
    QVERIFY(caster.isListening());
    auto configuration = caster.connectionConfig();
    auto controller = std::make_unique<NTRIPSourceTableController>();
    controller->fetch(configuration);
    const auto previous = controller->_activeSession();
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
        QVERIFY(controller->_activeSession() && controller->_activeSession() != previous);
        QVERIFY(controller->fetchError().isEmpty());
    }
}

void NTRIPSourceTableControllerTest::deletedSessionPublishesError()
{
    NTRIPSourceTableController controller;
    controller.fetch(config());
    delete controller._activeSession().data();
    QVERIFY(!controller._activeSession());
    QCOMPARE(controller.fetchStatus(), NTRIPSourceTableController::FetchStatus::Error);
    QVERIFY(!controller.fetchError().isEmpty());
}

void NTRIPSourceTableControllerTest::fetchNotificationReentry_data()
{
    QTest::addColumn<bool>("destroy");
    QTest::newRow("replace") << false;
    QTest::newRow("delete") << true;
}

void NTRIPSourceTableControllerTest::fetchNotificationReentry()
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
    controller->fetch(config());
    if (destroy) {
        QVERIFY(deleted);
        const QPointer<NTRIPHttpSession> session = deleted->_activeSession();
        QVERIFY(session);
        QTRY_VERIFY_WITH_TIMEOUT(!deleted && !session, TestTimeout::shortMs());
        return;
    }
    QCOMPARE(controller->fetchStatus(), NTRIPSourceTableController::FetchStatus::Error);
    QVERIFY(!controller->_activeSession());
}

void NTRIPSourceTableControllerTest::modelResetReentry_data()
{
    QTest::addColumn<bool>("aboutToReset");
    QTest::addColumn<int>("action");
    for (bool aboutToReset : {false, true}) {
        QTest::newRow(aboutToReset ? "about-to-reset-error" : "reset-error") << aboutToReset << 0;
        QTest::newRow(aboutToReset ? "about-to-reset-delete" : "reset-delete") << aboutToReset << 1;
        QTest::newRow(aboutToReset ? "about-to-reset-replace" : "reset-replace") << aboutToReset << 2;
    }
}

void NTRIPSourceTableControllerTest::modelResetReentry()
{
    QFETCH(bool, aboutToReset);
    QFETCH(int, action);
    auto controller = std::make_unique<NTRIPSourceTableController>();
    auto* model = controller->mountpointModel();
    qRegisterMetaType<QPixmap>();
    new QAbstractItemModelTester(model, QAbstractItemModelTester::FailureReportingMode::QtTest, model);
    const QString table = QStringLiteral(
        "STR;MP1;Id;RTCM 3.2;details;2;GPS;NET;USA;40;-74;0;1;gen;none;B;N;4800\r\n"
        "ENDSOURCETABLE\r\n");
    controller->injectSourceTableForTest(table);
    bool handled = false;
    int countChangesAfterDeletion = 0;
    connect(qobject_cast<NTRIPSourceTableModel*>(model), &NTRIPSourceTableModel::countChanged, this, [&]() {
        if (!controller) {
            ++countChangesAfterDeletion;
        }
    });
    const auto retire = [&]() {
        if (std::exchange(handled, true)) {
            return;
        }
        if (action == 1) {
            controller.reset();
        } else if (action == 0) {
            controller->injectFetchErrorForTest(QStringLiteral("replacement"));
        } else {
            controller->injectSourceTableForTest(
                QString(table).replace(QStringLiteral("MP1"), QStringLiteral("replacement")));
        }
    };
    int publications = 0;
    connect(controller.get(), &NTRIPSourceTableController::fetchStatusChanged, this, [&]() {
        ++publications;
        if (action == 0) {
            QCOMPARE(controller->mountpointModel()->rowCount(), 0);
        } else if (action == 2) {
            QCOMPARE(model->data(model->index(0, 0), NTRIPSourceTableModel::MountpointRole).toString(),
                     QStringLiteral("replacement"));
        }
    });
    if (aboutToReset) {
        connect(model, &QAbstractItemModel::modelAboutToBeReset, this, retire);
    } else {
        connect(model, &QAbstractItemModel::modelReset, this, retire);
    }
    controller->injectSourceTableForTest(table);
    QVERIFY(handled);
    QCOMPARE(countChangesAfterDeletion, 0);
    if (action != 1) {
        QTRY_COMPARE_WITH_TIMEOUT(publications, 1, TestTimeout::mediumMs());
        QCOMPARE(controller->fetchStatus(), action == 0 ? NTRIPSourceTableController::FetchStatus::Error
                                                        : NTRIPSourceTableController::FetchStatus::Success);
    }
}

void NTRIPSourceTableControllerTest::modelMutationReentry_data()
{
    QTest::addColumn<bool>("aboutToReset");
    QTest::addColumn<int>("action");
    for (bool aboutToReset : {false, true}) {
        for (int action : {0, 1, 2, 3}) {
            QTest::newRow(qPrintable(QStringLiteral("%1-%2").arg(aboutToReset).arg(action))) << aboutToReset << action;
        }
    }
}

void NTRIPSourceTableControllerTest::modelMutationReentry()
{
    QFETCH(bool, aboutToReset);
    QFETCH(int, action);
    NTRIPSourceTableModel model;
    qRegisterMetaType<QPixmap>();
    QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::QtTest);
    const QString first = QStringLiteral("STR;MP1;Id;RTCM 3.2;details;2;GPS;NET;USA;40;-74;0;1;gen;none;B;N;4800\n");
    const QString second = QStringLiteral("STR;MP2;Id;RTCM 3.2;details;2;GPS;NET;DEU;52;13;0;1;gen;none;B;N;4800\n");
    model.parseSourceTable(first);
    bool handled = false;
    const auto mutate = [&]() {
        if (std::exchange(handled, true)) {
            return;
        }
        if (action == 0) {
            model.clear();
        } else if (action == 1) {
            model.parseSourceTable(second);
        } else if (action == 2) {
            model.updateDistances(QGeoCoordinate(52, 13));
        } else {
            model.updateDistances({});
        }
    };
    if (aboutToReset) {
        connect(&model, &QAbstractItemModel::modelAboutToBeReset, this, mutate);
    } else {
        connect(&model, &QAbstractItemModel::modelReset, this, mutate);
    }
    model.parseSourceTable(first + second);
    QVERIFY(handled);
    QCOMPARE(model.rowCount(), action == 0 ? 0 : action == 1 ? 1 : 2);
    if (action == 1 || action == 2) {
        QCOMPARE(model.data(model.index(0, 0), NTRIPSourceTableModel::MountpointRole).toString(),
                 QStringLiteral("MP2"));
    }
    if (action == 2) {
        QCOMPARE(model.data(model.index(0, 0), NTRIPSourceTableModel::DistanceKmRole).toDouble(), 0.0);
    } else if (action == 3) {
        QCOMPARE(model.data(model.index(0, 0), NTRIPSourceTableModel::MountpointRole).toString(),
                 QStringLiteral("MP1"));
        for (int row = 0; row < model.rowCount(); ++row) {
            QCOMPARE(model.data(model.index(row, 0), NTRIPSourceTableModel::DistanceKmRole).toDouble(), -1.0);
        }
    }
}

void NTRIPSourceTableControllerTest::singleMountpointDistanceNotification()
{
    NTRIPSourceTableModel model;
    model.parseSourceTable(QStringLiteral("STR;MP1;Id;RTCM 3.2;details;2;GPS;NET;USA;40;-74;0;1;gen;none;B;N;4800\n"));
    QSignalSpy changes(&model, &QAbstractItemModel::dataChanged);
    QSignalSpy resets(&model, &QAbstractItemModel::modelReset);
    for (const auto& coordinate : {QGeoCoordinate(40, -74), QGeoCoordinate(52, 13), QGeoCoordinate()}) {
        model.updateDistances(coordinate);
        QCOMPARE(changes.size(), 1);
        const auto change = changes.takeFirst();
        QCOMPARE(qvariant_cast<QModelIndex>(change[0]), model.index(0, 0));
        QCOMPARE(qvariant_cast<QModelIndex>(change[1]), model.index(0, 0));
        QCOMPARE(qvariant_cast<QList<int>>(change[2]), QList<int>{NTRIPSourceTableModel::DistanceKmRole});
        const double distance = model.data(model.index(0, 0), NTRIPSourceTableModel::DistanceKmRole).toDouble();
        QCOMPARE(distance, coordinate.isValid() ? coordinate.distanceTo(QGeoCoordinate(40, -74)) / 1000 : -1.0);
    }
    QVERIFY(resets.isEmpty());
}
