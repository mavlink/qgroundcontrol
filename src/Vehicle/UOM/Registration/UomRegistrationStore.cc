#include "UomRegistrationStore.h"

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QUuid>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>

namespace {
qint64 timestamp(qint64 now)
{
    return now > 0 ? now : QDateTime::currentMSecsSinceEpoch();
}

UomRegistration::Record readRecord(const QSqlQuery& query)
{
    UomRegistration::Record record;
    record.id = query.value(QStringLiteral("id")).toString();
    record.scope = query.value(QStringLiteral("scope")).toString();
    record.serial = query.value(QStringLiteral("serial")).toString();
    record.uid = query.value(QStringLiteral("uid")).toString();
    record.kind = query.value(QStringLiteral("kind")).toString();
    record.stage = query.value(QStringLiteral("stage")).toString();
    record.type = query.value(QStringLiteral("type")).toString();
    record.reason = query.value(QStringLiteral("reason")).toString();
    record.tokenReference = query.value(QStringLiteral("token_reference")).toString();
    record.configJson = query.value(QStringLiteral("config_json")).toString();
    record.deviceState = query.value(QStringLiteral("device_state")).toInt();
    record.updatedMs = query.value(QStringLiteral("updated_ms")).toLongLong();
    record.reportedValue = query.value(QStringLiteral("reported_value")).toString();
    record.reportedScope = query.value(QStringLiteral("reported_scope")).toString();
    record.reportedAtMs = query.value(QStringLiteral("reported_ms")).toLongLong();
    record.deviceVerified = query.value(QStringLiteral("device_verified")).toBool();
    record.verifiedBoot = query.value(QStringLiteral("verified_boot")).toUInt();
    record.verifiedRevision = query.value(QStringLiteral("verified_revision")).toUInt();
    return record;
}
}  // namespace

UomRegistrationStore::UomRegistrationStore(const QString& path)
    : _connection(QStringLiteral("UomRegistration-") + QUuid::createUuid().toString(QUuid::WithoutBraces))
{
    if (path.isEmpty() || (path != QStringLiteral(":memory:") && !QDir().mkpath(QFileInfo(path).absolutePath()))) {
        _fail();
        return;
    }
    _database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), _connection);
    _database.setDatabaseName(path);
    _database.setConnectOptions(QStringLiteral("QSQLITE_BUSY_TIMEOUT=5000"));
    if (!_database.open()) {
        _fail();
        return;
    }
    if (path != QStringLiteral(":memory:") &&
        !QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::WriteOwner)) {
        _fail();
        return;
    }
    _ready = _initialize();
}

UomRegistrationStore::~UomRegistrationStore()
{
    _database.close();
    _database = {};
    QSqlDatabase::removeDatabase(_connection);
}

bool UomRegistrationStore::_fail()
{
    _error = QStringLiteral("The UOM transaction database could not be read or saved");
    return false;
}

bool UomRegistrationStore::_initialize()
{
    QSqlQuery query(_database);
    if (!query.exec(QStringLiteral("PRAGMA journal_mode=WAL")) ||
        !query.exec(QStringLiteral("PRAGMA synchronous=FULL")) ||
        !query.exec(QStringLiteral("PRAGMA foreign_keys=ON")) || !query.exec(QStringLiteral("PRAGMA user_version")) ||
        !query.next() || query.value(0).toInt() > 2) {
        return _fail();
    }
    const int version = query.value(0).toInt();
    QStringList statements{
        QStringLiteral("CREATE TABLE IF NOT EXISTS operations (id TEXT PRIMARY KEY, scope TEXT NOT NULL, "
                       "serial TEXT NOT NULL, uid TEXT NOT NULL, kind TEXT NOT NULL, stage TEXT NOT NULL, "
                       "type TEXT NOT NULL, reason TEXT NOT NULL, token_reference TEXT NOT NULL, "
                       "config_json TEXT NOT NULL, device_state INTEGER NOT NULL, updated_ms INTEGER NOT NULL, "
                       "reported_value TEXT NOT NULL DEFAULT '', reported_scope TEXT NOT NULL DEFAULT '', "
                       "reported_ms INTEGER NOT NULL DEFAULT 0, device_verified INTEGER NOT NULL DEFAULT 0, "
                       "verified_boot INTEGER NOT NULL DEFAULT 0, verified_revision INTEGER NOT NULL DEFAULT 0)"),
        QStringLiteral("CREATE INDEX IF NOT EXISTS operation_device ON operations(serial,uid,updated_ms)"),
        QStringLiteral("CREATE UNIQUE INDEX IF NOT EXISTS operation_pending_device ON operations(serial,uid) "
                       "WHERE stage NOT IN ('complete','abandoned')"),
        QStringLiteral("CREATE TABLE IF NOT EXISTS attempts (scope TEXT NOT NULL, fingerprint TEXT NOT NULL, "
                       "last_ms INTEGER NOT NULL, PRIMARY KEY(scope,fingerprint))"),
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS audit (sequence INTEGER PRIMARY KEY, operation_id TEXT NOT NULL, "
            "scope TEXT NOT NULL, serial TEXT NOT NULL, uid TEXT NOT NULL, kind TEXT NOT NULL, "
            "stage TEXT NOT NULL, event TEXT NOT NULL, device_state INTEGER NOT NULL, at_ms INTEGER NOT NULL)"),
        QStringLiteral("PRAGMA user_version=2")};
    if (version == 1) {
        // Version 1 did not distinguish a report acknowledgement from transaction completion.
        // Existing rows therefore retain an unknown report time and no durable-device proof.
        statements << QStringLiteral("ALTER TABLE operations ADD COLUMN reported_value TEXT NOT NULL DEFAULT ''")
                   << QStringLiteral("ALTER TABLE operations ADD COLUMN reported_scope TEXT NOT NULL DEFAULT ''")
                   << QStringLiteral("ALTER TABLE operations ADD COLUMN reported_ms INTEGER NOT NULL DEFAULT 0")
                   << QStringLiteral("ALTER TABLE operations ADD COLUMN device_verified INTEGER NOT NULL DEFAULT 0")
                   << QStringLiteral("ALTER TABLE operations ADD COLUMN verified_boot INTEGER NOT NULL DEFAULT 0")
                   << QStringLiteral("ALTER TABLE operations ADD COLUMN verified_revision INTEGER NOT NULL DEFAULT 0");
    }
    if (!_database.transaction()) {
        return _fail();
    }
    for (const auto& statement : statements) {
        if (!query.exec(statement)) {
            _database.rollback();
            return _fail();
        }
    }
    if (!_database.commit()) {
        _database.rollback();
        return _fail();
    }
    return true;
}

bool UomRegistrationStore::save(const UomRegistration::Record& record)
{
    if (!_ready || record.id.isEmpty() || record.scope.isEmpty() || record.serial.isEmpty() || record.uid.isEmpty() ||
        record.kind.isEmpty() || record.stage.isEmpty()) {
        return _fail();
    }
    QSqlQuery query(_database);
    query.prepare(QStringLiteral(
        "INSERT INTO operations (id,scope,serial,uid,kind,stage,type,reason,token_reference,config_json,"
        "device_state,updated_ms,reported_value,reported_scope,reported_ms,device_verified,verified_boot,"
        "verified_revision) VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?) ON CONFLICT(id) DO UPDATE SET "
        "kind=excluded.kind,stage=excluded.stage,type=excluded.type,reason=excluded.reason,"
        "token_reference=excluded.token_reference,config_json=excluded.config_json,device_state="
        "excluded.device_state,updated_ms=excluded.updated_ms,reported_value=excluded.reported_value,"
        "reported_scope=excluded.reported_scope,reported_ms=excluded.reported_ms,"
        "device_verified=excluded.device_verified,verified_boot=excluded.verified_boot,"
        "verified_revision=excluded.verified_revision "
        "WHERE scope=excluded.scope AND serial=excluded.serial AND uid=excluded.uid"));
    for (const QString& value : {record.id, record.scope, record.serial, record.uid, record.kind, record.stage,
                                 record.type, record.reason, record.tokenReference, record.configJson}) {
        query.addBindValue(value.isNull() ? QStringLiteral("") : value);
    }
    query.addBindValue(record.deviceState);
    query.addBindValue(timestamp(record.updatedMs));
    query.addBindValue(record.reportedValue.isNull() ? QStringLiteral("") : record.reportedValue);
    query.addBindValue(record.reportedScope.isNull() ? QStringLiteral("") : record.reportedScope);
    query.addBindValue(record.reportedAtMs);
    query.addBindValue(record.deviceVerified);
    query.addBindValue(record.verifiedBoot);
    query.addBindValue(record.verifiedRevision);
    if (!query.exec() || query.numRowsAffected() != 1) {
        return _fail();
    }
    _error.clear();
    return true;
}

std::optional<UomRegistration::Record> UomRegistrationStore::loadPending(const QString& scope, const QString& serial,
                                                                         const QString& uid)
{
    if (!_ready) {
        _fail();
        return std::nullopt;
    }
    QSqlQuery query(_database);
    query.prepare(
        QStringLiteral("SELECT * FROM operations WHERE scope=? AND serial=? AND uid=? "
                       "AND stage NOT IN ('complete','abandoned') ORDER BY updated_ms DESC LIMIT 1"));
    query.addBindValue(scope);
    query.addBindValue(serial);
    query.addBindValue(uid);
    if (!query.exec()) {
        _fail();
        return std::nullopt;
    }
    _error.clear();
    return query.next() ? std::optional<UomRegistration::Record>(readRecord(query)) : std::nullopt;
}

std::optional<UomRegistration::Record> UomRegistrationStore::loadPendingForDevice(const QString& serial,
                                                                                  const QString& uid)
{
    if (!_ready) {
        _fail();
        return std::nullopt;
    }
    QSqlQuery query(_database);
    query.prepare(
        QStringLiteral("SELECT * FROM operations WHERE serial=? AND uid=? "
                       "AND stage NOT IN ('complete','abandoned') ORDER BY updated_ms DESC LIMIT 1"));
    query.addBindValue(serial);
    query.addBindValue(uid);
    if (!query.exec()) {
        _fail();
        return std::nullopt;
    }
    _error.clear();
    return query.next() ? std::optional<UomRegistration::Record>(readRecord(query)) : std::nullopt;
}

std::optional<UomRegistration::Record> UomRegistrationStore::loadLatest(const QString& scope, const QString& serial,
                                                                        const QString& uid)
{
    if (!_ready) {
        _fail();
        return std::nullopt;
    }
    QSqlQuery query(_database);
    query.prepare(
        QStringLiteral("SELECT * FROM operations WHERE scope=? AND serial=? AND uid=? "
                       "ORDER BY updated_ms DESC LIMIT 1"));
    query.addBindValue(scope);
    query.addBindValue(serial);
    query.addBindValue(uid);
    if (!query.exec()) {
        _fail();
        return std::nullopt;
    }
    _error.clear();
    return query.next() ? std::optional<UomRegistration::Record>(readRecord(query)) : std::nullopt;
}

std::optional<UomRegistration::Record> UomRegistrationStore::loadLatestReport(const QString& scope,
                                                                              const QString& serial, const QString& uid)
{
    if (!_ready) {
        _fail();
        return std::nullopt;
    }
    QSqlQuery query(_database);
    query.prepare(
        QStringLiteral("SELECT * FROM operations WHERE reported_scope=? AND serial=? AND uid=? "
                       "AND reported_value IN ('1','2') AND reported_ms>0 ORDER BY reported_ms DESC LIMIT 1"));
    query.addBindValue(scope);
    query.addBindValue(serial);
    query.addBindValue(uid);
    if (!query.exec()) {
        _fail();
        return std::nullopt;
    }
    _error.clear();
    return query.next() ? std::optional<UomRegistration::Record>(readRecord(query)) : std::nullopt;
}

bool UomRegistrationStore::migrateProfile(const UomRegistration::Record& previous,
                                          const UomRegistration::Record& replacement)
{
    const auto readProfile = [](const UomRegistration::Record& record) -> std::optional<UomRegistration::Config> {
        const auto object = QJsonDocument::fromJson(record.configJson.toUtf8()).object();
        UomRegistration::Config config;
        config.environment = object.value(QStringLiteral("environment")).toString();
        config.producerId = object.value(QStringLiteral("producerId")).toString();
        config.expectedSerial = object.value(QStringLiteral("expectedSerial")).toString();
        config.verifyUrl = QUrl(object.value(QStringLiteral("verifyUrl")).toString());
        config.reportUrl = QUrl(object.value(QStringLiteral("reportUrl")).toString());
        config.cancelUrl = QUrl(object.value(QStringLiteral("cancelUrl")).toString());
        return config.valid() && config.scope() == record.scope && config.expectedSerial == record.serial
                   ? std::optional(config)
                   : std::nullopt;
    };
    const auto oldConfig = readProfile(previous);
    const auto newConfig = readProfile(replacement);
    if (!_ready || !oldConfig || !newConfig || oldConfig->environment != newConfig->environment ||
        previous.id != replacement.id || previous.serial != replacement.serial || previous.uid != replacement.uid ||
        previous.kind != replacement.kind || previous.type != replacement.type ||
        previous.reason != replacement.reason || previous.reportedValue != replacement.reportedValue ||
        previous.reportedScope != replacement.reportedScope || previous.reportedAtMs != replacement.reportedAtMs ||
        previous.deviceState != replacement.deviceState || previous.deviceVerified != replacement.deviceVerified ||
        previous.verifiedBoot != replacement.verifiedBoot ||
        previous.verifiedRevision != replacement.verifiedRevision || !replacement.tokenReference.isEmpty() ||
        replacement.stage != QStringLiteral("profile-recovery") || !_database.transaction()) {
        return _fail();
    }
    const auto rollback = [this]() {
        _database.rollback();
        return _fail();
    };
    QSqlQuery query(_database);
    query.prepare(QStringLiteral("SELECT * FROM operations WHERE id=?"));
    query.addBindValue(previous.id);
    if (!query.exec() || !query.next()) {
        return rollback();
    }
    const auto stored = readRecord(query);
    if (stored.scope != previous.scope || stored.serial != previous.serial || stored.uid != previous.uid ||
        stored.configJson != previous.configJson || stored.stage != previous.stage || stored.kind != previous.kind ||
        stored.type != previous.type || stored.reason != previous.reason ||
        stored.tokenReference != previous.tokenReference || stored.deviceState != previous.deviceState ||
        stored.deviceVerified != previous.deviceVerified || stored.verifiedBoot != previous.verifiedBoot ||
        stored.verifiedRevision != previous.verifiedRevision || stored.reportedValue != previous.reportedValue ||
        stored.reportedScope != previous.reportedScope || stored.reportedAtMs != previous.reportedAtMs ||
        stored.updatedMs != previous.updatedMs || stored.stage == QStringLiteral("complete") ||
        stored.stage == QStringLiteral("abandoned")) {
        return rollback();
    }
    query.finish();
    const auto profileEvent = [](const QString& action, const UomRegistration::Config& config) {
        const QJsonObject profile{{QStringLiteral("environment"), config.environment},
                                  {QStringLiteral("producerId"), config.producerId},
                                  {QStringLiteral("expectedSerial"), config.expectedSerial},
                                  {QStringLiteral("verifyUrl"), config.verifyUrl.toString()},
                                  {QStringLiteral("reportUrl"), config.reportUrl.toString()},
                                  {QStringLiteral("cancelUrl"), config.cancelUrl.toString()}};
        return action + QString::fromUtf8(QJsonDocument(profile).toJson(QJsonDocument::Compact));
    };
    if (!audit(previous, profileEvent(QStringLiteral("profile-replaced:"), *oldConfig))) {
        return rollback();
    }
    query.prepare(QStringLiteral("UPDATE operations SET scope=? WHERE id=? AND scope=?"));
    query.addBindValue(replacement.scope);
    query.addBindValue(previous.id);
    query.addBindValue(previous.scope);
    if (!query.exec() || query.numRowsAffected() != 1 || !save(replacement) ||
        !audit(replacement, profileEvent(QStringLiteral("profile-recovered:"), *newConfig)) || !_database.commit()) {
        return rollback();
    }
    _error.clear();
    return true;
}

bool UomRegistrationStore::complete(const QString& id, const QString& stage)
{
    if (!_ready || (stage != QStringLiteral("complete") && stage != QStringLiteral("abandoned"))) {
        return _fail();
    }
    QSqlQuery query(_database);
    query.prepare(QStringLiteral("UPDATE operations SET stage=?,updated_ms=? WHERE id=?"));
    query.addBindValue(stage);
    query.addBindValue(QDateTime::currentMSecsSinceEpoch());
    query.addBindValue(id);
    if (!query.exec() || query.numRowsAffected() != 1) {
        return _fail();
    }
    _error.clear();
    return true;
}

int UomRegistrationStore::retryAfter(const QString& scope, const QString& fingerprint, qint64 nowMs)
{
    if (!_ready) {
        _fail();
        return 60;
    }
    QSqlQuery query(_database);
    query.prepare(QStringLiteral("SELECT last_ms FROM attempts WHERE scope=? AND fingerprint=?"));
    query.addBindValue(scope);
    query.addBindValue(fingerprint);
    if (!query.exec()) {
        _fail();
        return 60;
    }
    _error.clear();
    if (!query.next()) {
        return 0;
    }
    const qint64 elapsed = timestamp(nowMs) - query.value(0).toLongLong();
    // A backward clock adjustment never silently permits a duplicate request.
    return elapsed < 0 ? 60 : int(qMax<qint64>(0, (60000 - elapsed + 999) / 1000));
}

bool UomRegistrationStore::recordAttempt(const QString& scope, const QString& fingerprint, qint64 nowMs)
{
    if (!_ready || scope.isEmpty() || fingerprint.size() != 64) {
        return _fail();
    }
    QSqlQuery query(_database);
    query.prepare(
        QStringLiteral("INSERT INTO attempts VALUES(?,?,?) ON CONFLICT(scope,fingerprint) "
                       "DO UPDATE SET last_ms=excluded.last_ms WHERE last_ms<=excluded.last_ms-60000"));
    query.addBindValue(scope);
    query.addBindValue(fingerprint);
    query.addBindValue(timestamp(nowMs));
    if (!query.exec() || query.numRowsAffected() != 1) {
        return _fail();
    }
    _error.clear();
    return true;
}

bool UomRegistrationStore::audit(const UomRegistration::Record& record, const QString& event)
{
    if (!_ready || event.isEmpty()) {
        return _fail();
    }
    QSqlQuery query(_database);
    query.prepare(
        QStringLiteral("INSERT INTO audit(operation_id,scope,serial,uid,kind,stage,event,device_state,at_ms) "
                       "VALUES(?,?,?,?,?,?,?,?,?)"));
    for (const QString& value :
         {record.id, record.scope, record.serial, record.uid, record.kind, record.stage, event}) {
        query.addBindValue(value.isNull() ? QStringLiteral("") : value);
    }
    query.addBindValue(record.deviceState);
    query.addBindValue(QDateTime::currentMSecsSinceEpoch());
    if (!query.exec()) {
        return _fail();
    }
    _error.clear();
    return true;
}
