#include "UomRegistrationCoreTest.h"

#include <cstring>

#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QScopeGuard>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTimer>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QSslConfiguration>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "UnitTest.h"
#include "UomRegistrationClient.h"
#include "UomRegistrationCredentials.h"
#include "UomRegistrationCrypto.h"
#include "UomRegistrationStore.h"

using UomRegistration::Operation;
using UomRegistration::Result;

namespace {
const QString KEY = QStringLiteral("0123456789abcdeffedcba9876543210");
const QString SERIAL = QStringLiteral("TEST0000000000000001");

class TestReply final : public QNetworkReply
{
public:
    TestReply(const QNetworkRequest& request, QByteArray data, int status, bool delayed, QObject* parent)
        : QNetworkReply(parent)
        , _data(std::move(data))
    {
        setRequest(request);
        setUrl(request.url());
        setAttribute(QNetworkRequest::HttpStatusCodeAttribute, status);
        open(QIODevice::ReadOnly | QIODevice::Unbuffered);
        if (!delayed) {
            QTimer::singleShot(0, this, [this]() {
                setFinished(true);
                emit readyRead();
                emit finished();
            });
        }
    }

    void abort() override
    {
        if (!isFinished()) {
            setError(QNetworkReply::OperationCanceledError, QStringLiteral("Cancelled"));
            setFinished(true);
            emit finished();
        }
    }

    qint64 bytesAvailable() const override { return _data.size() - _offset + QNetworkReply::bytesAvailable(); }

protected:
    qint64 readData(char* output, qint64 maximum) override
    {
        const qint64 count = qMin(maximum, _data.size() - _offset);
        if (count <= 0) {
            return -1;
        }
        std::memcpy(output, _data.constData() + _offset, size_t(count));
        _offset += count;
        return count;
    }

private:
    QByteArray _data;
    qint64 _offset = 0;
};

class TestNetwork final : public QNetworkAccessManager
{
public:
    QByteArray response = R"({"code":200,"body":{"value":"0"}})";
    QByteArray sent;
    QNetworkRequest lastRequest;
    int status = 200;
    int count = 0;
    bool delayed = false;

protected:
    QNetworkReply* createRequest(QNetworkAccessManager::Operation, const QNetworkRequest& request,
                                 QIODevice* outgoing) override
    {
        ++count;
        lastRequest = request;
        sent = outgoing ? outgoing->readAll() : QByteArray();
        return new TestReply(request, response, status, delayed, this);
    }
};

UomRegistration::Config config()
{
    UomRegistration::Config cfg;
    cfg.producerId = QStringLiteral("12345678-1234-1234-1234-123456789012");
    cfg.expectedSerial = SERIAL;
    return cfg;
}

QString profileJson(const UomRegistration::Config& cfg)
{
    return QString::fromUtf8(QJsonDocument(QJsonObject{{QStringLiteral("environment"), cfg.environment},
                                                       {QStringLiteral("producerId"), cfg.producerId},
                                                       {QStringLiteral("expectedSerial"), cfg.expectedSerial},
                                                       {QStringLiteral("verifyUrl"), cfg.verifyUrl.toString()},
                                                       {QStringLiteral("reportUrl"), cfg.reportUrl.toString()},
                                                       {QStringLiteral("cancelUrl"), cfg.cancelUrl.toString()}})
                                 .toJson(QJsonDocument::Compact));
}

UomRegistration::Record record()
{
    return {.id = QStringLiteral("operation-1"),
            .scope = config().scope(),
            .serial = SERIAL,
            .uid = QStringLiteral("001122AABBCC"),
            .kind = QStringLiteral("deactivate"),
            .stage = QStringLiteral("awaiting-confirmation"),
            .type = QStringLiteral("3"),
            .reason = QStringLiteral("Ownership change"),
            .tokenReference = QStringLiteral("token-ref-1"),
            .configJson = profileJson(config()),
            .deviceState = 2,
            .updatedMs = 100000};
}
}  // namespace

void UomRegistrationCoreTest::_cryptoContract()
{
    const QByteArray plain = R"({"UPIC_MSN":"TEST0000000000000001"})";
    const auto cipher = UomRegistration::encrypt(plain, KEY);
    QVERIFY(cipher);
    QCOMPARE(
        *cipher,
        QByteArray("ed8a9bb659a3d5e91c45da97c9e5cb53afca255bca74027e544bdbc5434d61f4fa8fd72358440d007e082c4bb25a434c"));
    QCOMPARE(UomRegistration::decrypt(*cipher, KEY).value_or(QByteArray()), plain);
    const QByteArray unicode = QStringLiteral("{\"REASON\":\"所有权变更\"}").toUtf8();
    QCOMPARE(UomRegistration::decrypt(*UomRegistration::encrypt(unicode, KEY), KEY).value_or(QByteArray()), unicode);
    QVERIFY(!UomRegistration::encrypt(plain, QStringLiteral("not-a-key")));
    QVERIFY(!UomRegistration::decrypt(QByteArray("xx"), KEY));
    QByteArray damaged = *cipher;
    damaged[damaged.size() - 1] = damaged.back() == '0' ? '1' : '0';
    QVERIFY(!UomRegistration::decrypt(damaged, KEY));
}

void UomRegistrationCoreTest::_officialProfilesOnly()
{
    auto cfg = config();
    QVERIFY(cfg.valid());
    const auto original = cfg.scope();
    cfg.environment = QStringLiteral("official");
    QVERIFY(cfg.valid());
    QVERIFY(cfg.scope() != original);
    for (const QString& url :
         {QStringLiteral("http://uom.caac.gov.cn/a"), QStringLiteral("https://example.com/a"),
          QStringLiteral("https://uom.caac.gov.cn:444/a"), QStringLiteral("https://user@uom.caac.gov.cn/a"),
          QStringLiteral("https://uom.caac.gov.cn/a?secret=test")}) {
        cfg.verifyUrl = QUrl(url);
        QVERIFY(!cfg.valid());
    }
    QVERIFY(!UomRegistration::validSerial(QStringLiteral("too-short")));
    QVERIFY(!UomRegistration::validSerial(QString(20, u' ')));
}

void UomRegistrationCoreTest::_parser_data()
{
    QTest::addColumn<int>("operation");
    QTest::addColumn<int>("status");
    QTest::addColumn<QByteArray>("body");
    QTest::addColumn<bool>("success");
    QTest::addColumn<bool>("uncertain");
    QTest::newRow("verify-json-string") << int(Operation::Verify) << 200
                                        << QByteArray(R"({"code":200,"body":"{\"value\":\"99\"}"})") << true << false;
    QTest::newRow("activation") << int(Operation::ReportActive) << 200
                                << QByteArray(R"({"code":200,"body":{"value":"1"}})") << true << false;
    QTest::newRow("token-issued") << int(Operation::ReportInactive) << 200
                                  << QByteArray(R"({"code":200,"body":{"value":"2","token":"sensitive"}})") << true
                                  << false;
    QTest::newRow("missing-token") << int(Operation::ReportInactive) << 200
                                   << QByteArray(R"({"code":200,"body":{"value":"2"}})") << false << true;
    QTest::newRow("wrong-report-state") << int(Operation::ReportActive) << 200
                                        << QByteArray(R"({"code":200,"body":{"value":"2"}})") << false << true;
    QTest::newRow("cancelled") << int(Operation::Cancel) << 200 << QByteArray(R"({"code":200,"body":{"value":"3"}})")
                               << true << false;
    QTest::newRow("cancel-wrong-state") << int(Operation::Cancel) << 200
                                        << QByteArray(R"({"code":200,"body":{"value":"0"}})") << false << true;
    QTest::newRow("rejected") << int(Operation::Cancel) << 200 << QByteArray(R"({"code":401,"msg":"TOKEN invalid"})")
                              << false << false;
    QTest::newRow("malformed") << int(Operation::Cancel) << 200 << QByteArray("broken") << false << true;
    QTest::newRow("http-error") << int(Operation::Cancel) << 503 << QByteArray("unavailable") << false << true;
    QTest::newRow("verify-error") << int(Operation::Verify) << 503 << QByteArray("unavailable") << false << false;
}

void UomRegistrationCoreTest::_parser()
{
    QFETCH(int, operation);
    QFETCH(int, status);
    QFETCH(QByteArray, body);
    QFETCH(bool, success);
    QFETCH(bool, uncertain);
    const auto result = UomRegistrationClient::parseResponse(Operation(operation), status, body);
    QCOMPARE(result.success, success);
    QCOMPARE(result.uncertain, uncertain);
}

void UomRegistrationCoreTest::_durableRecoveryAndScopeIsolation()
{
    QTemporaryDir directory;
    const QString path = directory.filePath(QStringLiteral("uom.sqlite"));
    const QString fingerprint(64, u'a');
    {
        UomRegistrationStore store(path);
        QVERIFY2(store.ready(), qPrintable(store.error()));
        QVERIFY(store.save(record()));
        QVERIFY(store.audit(record(), QStringLiteral("device-inactive")));
        QVERIFY(store.recordAttempt(config().scope(), fingerprint, 100000));
        QCOMPARE(store.retryAfter(config().scope(), fingerprint, 159000), 1);
        QCOMPARE(store.retryAfter(config().scope(), fingerprint, 99000), 60);
        QCOMPARE(store.retryAfter(QStringLiteral("other-scope"), fingerprint, 100000), 0);
        QVERIFY(!store.loadPending(QStringLiteral("other-scope"), SERIAL, record().uid));
        QVERIFY(store.loadPendingForDevice(SERIAL, record().uid));
        auto collision = record();
        collision.scope = QStringLiteral("another-scope");
        QVERIFY(!store.save(collision));
        collision.id = QStringLiteral("second-operation");
        QVERIFY(!store.save(collision));
    }
    {
        UomRegistrationStore store(path);
        QVERIFY(store.ready());
        auto pending = store.loadPending(config().scope(), SERIAL, record().uid);
        QVERIFY(pending);
        QCOMPARE(pending->tokenReference, record().tokenReference);
        QCOMPARE(pending->configJson, record().configJson);
        QCOMPARE(store.retryAfter(config().scope(), fingerprint, 159000), 1);
        QCOMPARE(store.retryAfter(config().scope(), fingerprint, 160000), 0);
        QVERIFY(store.complete(record().id));
        QVERIFY(!store.loadPendingForDevice(SERIAL, record().uid));
        QVERIFY(store.loadLatest(config().scope(), SERIAL, record().uid));
    }
}

void UomRegistrationCoreTest::_schemaMigrationDoesNotInventAcknowledgements()
{
    QTemporaryDir directory;
    const QString path = directory.filePath(QStringLiteral("legacy.sqlite"));
    const QString connection = QStringLiteral("uom-legacy-fixture");
    {
        auto database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection);
        database.setDatabaseName(path);
        QVERIFY(database.open());
        QSqlQuery query(database);
        QVERIFY(query.exec(
            QStringLiteral("CREATE TABLE operations (id TEXT PRIMARY KEY, scope TEXT NOT NULL, serial TEXT NOT NULL, "
                           "uid TEXT NOT NULL, "
                           "kind TEXT NOT NULL, stage TEXT NOT NULL, type TEXT NOT NULL, reason TEXT NOT NULL, "
                           "token_reference TEXT NOT NULL, config_json TEXT NOT NULL, device_state INTEGER NOT NULL, "
                           "updated_ms INTEGER NOT NULL)")));
        QVERIFY(query.exec(QStringLiteral("PRAGMA user_version=1")));
        query.prepare(QStringLiteral(
            "INSERT INTO operations VALUES('legacy',?,?,?,'deactivate','complete','3','reason','','{}',0,123456)"));
        query.addBindValue(config().scope());
        query.addBindValue(SERIAL);
        query.addBindValue(record().uid);
        QVERIFY(query.exec());
        database.close();
    }
    QSqlDatabase::removeDatabase(connection);
    UomRegistrationStore store(path);
    QVERIFY2(store.ready(), qPrintable(store.error()));
    const auto legacy = store.loadLatest(config().scope(), SERIAL, record().uid);
    QVERIFY(legacy);
    QCOMPARE(legacy->updatedMs, 123456);
    QVERIFY(legacy->reportedValue.isEmpty());
    QCOMPARE(legacy->reportedAtMs, 0);
    QVERIFY(!legacy->deviceVerified);
    QCOMPARE(legacy->verifiedBoot, 0U);
    QCOMPARE(legacy->verifiedRevision, 0U);
    QVERIFY(!store.loadLatestReport(config().scope(), SERIAL, record().uid));
    QVERIFY(store.save(record()));
}

void UomRegistrationCoreTest::_profileMigrationPreservesIdentityAndHistory()
{
    QTemporaryDir directory;
    const QString path = directory.filePath(QStringLiteral("migration.sqlite"));
    UomRegistrationStore store(path);
    QVERIFY(store.ready());
    auto original = record();
    original.deviceState = 0;
    original.deviceVerified = true;
    original.verifiedBoot = 42;
    original.verifiedRevision = 7;
    original.reportedValue = QStringLiteral("2");
    original.reportedScope = original.scope;
    original.reportedAtMs = 90000;
    QVERIFY(store.save(original));
    auto cfg = config();
    cfg.producerId = QStringLiteral("87654321-4321-4321-4321-210987654321");
    auto replacement = original;
    replacement.scope = cfg.scope();
    replacement.configJson = profileJson(cfg);
    replacement.stage = QStringLiteral("profile-recovery");
    replacement.tokenReference.clear();
    replacement.updatedMs += 1;
    auto invalid = replacement;
    invalid.reason = QStringLiteral("changed intent");
    QVERIFY(!store.migrateProfile(original, invalid));
    invalid = replacement;
    invalid.tokenReference = QStringLiteral("old-token");
    QVERIFY(!store.migrateProfile(original, invalid));
    invalid = replacement;
    cfg.environment = QStringLiteral("official");
    invalid.scope = cfg.scope();
    invalid.configJson = profileJson(cfg);
    QVERIFY(!store.migrateProfile(original, invalid));

    const QString connection = QStringLiteral("uom-migration-fixture");
    {
        auto database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection);
        database.setDatabaseName(path);
        QVERIFY(database.open());
        QSqlQuery query(database);
        QVERIFY(query.exec(
            QStringLiteral("CREATE TRIGGER fail_migration BEFORE INSERT ON audit "
                           "WHEN NEW.event LIKE 'profile-recovered:%' BEGIN SELECT RAISE(ABORT,'test'); END")));
        QVERIFY(!store.migrateProfile(original, replacement));
        const auto unchanged = store.loadPendingForDevice(SERIAL, original.uid);
        QVERIFY(unchanged);
        QCOMPARE(unchanged->scope, original.scope);
        QVERIFY(query.exec(QStringLiteral("SELECT COUNT(*) FROM audit")));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toInt(), 0);
        query.finish();
        QVERIFY(query.exec(QStringLiteral("DROP TRIGGER fail_migration")));
        QVERIFY(store.migrateProfile(original, replacement));
        QVERIFY(query.exec(QStringLiteral("SELECT scope,event FROM audit ORDER BY sequence")));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toString(), original.scope);
        QVERIFY(query.value(1).toString().contains(config().producerId));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toString(), replacement.scope);
        QVERIFY(query.value(1).toString().contains(QStringLiteral("87654321-4321-4321-4321-210987654321")));
        QVERIFY(!query.next());
        database.close();
    }
    QSqlDatabase::removeDatabase(connection);
    QVERIFY(!store.migrateProfile(original, replacement));
    QVERIFY(!store.loadPending(original.scope, SERIAL, original.uid));
    const auto migrated = store.loadPending(replacement.scope, SERIAL, original.uid);
    QVERIFY(migrated);
    QVERIFY(migrated->tokenReference.isEmpty());
    QVERIFY(migrated->deviceVerified);
    QCOMPARE(migrated->verifiedBoot, original.verifiedBoot);
    QCOMPARE(migrated->verifiedRevision, original.verifiedRevision);
    QCOMPARE(migrated->reason, original.reason);
    QCOMPARE(migrated->reportedScope, original.scope);
    QCOMPARE(migrated->reportedAtMs, original.reportedAtMs);
    QVERIFY(store.loadLatestReport(original.scope, SERIAL, original.uid));
    QVERIFY(!store.loadLatestReport(replacement.scope, SERIAL, original.uid));
}

void UomRegistrationCoreTest::_encryptedWireAndPersistentLimit()
{
    UomRegistrationStore store(QStringLiteral(":memory:"));
    QVERIFY(store.ready());
    TestNetwork network;
    UomRegistrationClient client(&store, &network, nullptr);
    QSignalSpy completed(&client, &UomRegistrationClient::completed);
    client.send(1, config(), KEY, Operation::Verify, SERIAL);
    QTRY_COMPARE_WITH_TIMEOUT(completed.count(), 1, TestTimeout::mediumMs());
    QVERIFY(qvariant_cast<Result>(completed.at(0).at(1)).success);
    QCOMPARE(network.lastRequest.url(), config().verifyUrl);
    QCOMPARE(network.lastRequest.sslConfiguration().peerVerifyMode(), QSslSocket::VerifyPeer);
    QCOMPARE(network.lastRequest.attribute(QNetworkRequest::RedirectPolicyAttribute).toInt(),
             int(QNetworkRequest::ManualRedirectPolicy));
    QCOMPARE(network.lastRequest.header(QNetworkRequest::ContentTypeHeader).toString(),
             QStringLiteral("application/json;charset=utf-8"));
    const auto envelope = QJsonDocument::fromJson(network.sent).object();
    QCOMPARE(envelope.size(), 2);
    QCOMPARE(envelope.value(QStringLiteral("id")).toString(), config().producerId);
    const auto decrypted = UomRegistration::decrypt(envelope.value(QStringLiteral("body")).toString().toLatin1(), KEY);
    QVERIFY(decrypted);
    QCOMPARE(QJsonDocument::fromJson(*decrypted).object().value(QStringLiteral("UPIC_MSN")).toString(), SERIAL);
    QVERIFY(!network.sent.contains(SERIAL.toUtf8()));
    client.send(2, config(), KEY, Operation::Verify, SERIAL);
    QTRY_COMPARE_WITH_TIMEOUT(completed.count(), 2, TestTimeout::mediumMs());
    QVERIFY(qvariant_cast<Result>(completed.at(1).at(1)).retryAfterSeconds > 0);
    QCOMPARE(network.count, 1);
    auto otherEndpoint = config();
    otherEndpoint.cancelUrl.setPath(QStringLiteral("/different-cancellation-path"));
    client.send(3, otherEndpoint, KEY, Operation::Verify, SERIAL);
    QTRY_COMPARE_WITH_TIMEOUT(completed.count(), 3, TestTimeout::mediumMs());
    QVERIFY(qvariant_cast<Result>(completed.at(2).at(1)).retryAfterSeconds > 0);
    client.send(4, config(), QStringLiteral("bad-key"), Operation::ReportActive, SERIAL);
    QTRY_COMPARE_WITH_TIMEOUT(completed.count(), 4, TestTimeout::mediumMs());
    QCOMPARE(network.count, 1);
}

void UomRegistrationCoreTest::_cancellationHasNoHiddenSerialAndNoLateResult()
{
    QTemporaryDir directory;
    const QString path = directory.filePath(QStringLiteral("wire-audit.sqlite"));
    UomRegistrationStore store(path);
    TestNetwork network;
    network.response = R"({"code":200,"body":{"value":"3"}})";
    UomRegistrationClient client(&store, &network, nullptr);
    QSignalSpy completed(&client, &UomRegistrationClient::completed);
    client.send(1, config(), KEY, Operation::Cancel, SERIAL, QStringLiteral("3"), QStringLiteral("所有权变更"),
                QStringLiteral("private-token"));
    QTRY_COMPARE_WITH_TIMEOUT(completed.count(), 1, TestTimeout::mediumMs());
    const auto envelope = QJsonDocument::fromJson(network.sent).object();
    const auto payload = QJsonDocument::fromJson(*UomRegistration::decrypt(
                                                     envelope.value(QStringLiteral("body")).toString().toLatin1(), KEY))
                             .object();
    QCOMPARE(payload.size(), 3);
    QCOMPARE(payload.value(QStringLiteral("TOKEN")).toString(), QStringLiteral("private-token"));
    QVERIFY(!payload.contains(QStringLiteral("UPIC_MSN")));
    QVERIFY(!network.sent.contains("private-token"));
    network.delayed = true;
    client.send(2, config(), KEY, Operation::Verify, SERIAL);
    client.cancel();
    QCoreApplication::sendPostedEvents();
    QCOMPARE(completed.count(), 1);
    for (const QString& suffix : {QStringLiteral(""), QStringLiteral("-wal")}) {
        QFile file(path + suffix);
        if (file.open(QIODevice::ReadOnly)) {
            const QByteArray bytes = file.readAll();
            QVERIFY(!bytes.contains("private-token"));
            QVERIFY(!bytes.contains(KEY.toUtf8()));
        }
    }
}

void UomRegistrationCoreTest::_memoryCredentialsAreExplicitAndRemovable()
{
    UomRegistrationCredentials credentials(nullptr, true);
    QSignalSpy completed(&credentials, &UomRegistrationCredentials::completed);
    credentials.put(QStringLiteral("profile-key"), KEY);
    QTRY_COMPARE_WITH_TIMEOUT(completed.count(), 1, TestTimeout::mediumMs());
    QCOMPARE(credentials.value(QStringLiteral("profile-key")), KEY);
    QVERIFY(!credentials.persistent(QStringLiteral("profile-key")));
    QVERIFY(completed.at(0).at(1).toBool());
    credentials.remove(QStringLiteral("profile-key"));
    QTRY_COMPARE_WITH_TIMEOUT(completed.count(), 2, TestTimeout::mediumMs());
    QVERIFY(credentials.value(QStringLiteral("profile-key")).isEmpty());
    credentials.get(QStringLiteral("missing"));
    QTRY_COMPARE_WITH_TIMEOUT(completed.count(), 3, TestTimeout::mediumMs());
    QVERIFY(!completed.at(2).at(1).toBool());
}

void UomRegistrationCoreTest::_unavailableKeyringRetainsMemory()
{
    const QByteArray oldAddress = qgetenv("DBUS_SESSION_BUS_ADDRESS");
    const auto restore = qScopeGuard([&]() {
        if (oldAddress.isNull()) {
            qunsetenv("DBUS_SESSION_BUS_ADDRESS");
        } else {
            qputenv("DBUS_SESSION_BUS_ADDRESS", oldAddress);
        }
    });
    qputenv("DBUS_SESSION_BUS_ADDRESS", "unix:path=/nonexistent-qgc-uom-test-bus");
    UomRegistrationCredentials credentials;
    QSignalSpy completed(&credentials, &UomRegistrationCredentials::completed);
    credentials.put(QStringLiteral("qgc-uom-unit-test-unavailable-keyring"), KEY);
    QTRY_COMPARE_WITH_TIMEOUT(completed.count(), 1, TestTimeout::longMs());
    QVERIFY(completed.at(0).at(1).toBool());
    QVERIFY(!credentials.persistent(QStringLiteral("qgc-uom-unit-test-unavailable-keyring")));
    QCOMPARE(credentials.value(QStringLiteral("qgc-uom-unit-test-unavailable-keyring")), KEY);
}

QGC_REGISTER_PORTABLE_TEST(UomRegistrationCoreTest)
