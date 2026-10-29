#pragma once

#include <optional>

#include <QtSql/QSqlDatabase>

#include "UomRegistrationTypes.h"

/// Durable transaction and audit data. Secret keys and cancellation tokens never enter this store.
class UomRegistrationStore final
{
public:
    explicit UomRegistrationStore(const QString& path);
    ~UomRegistrationStore();
    UomRegistrationStore(const UomRegistrationStore&) = delete;
    UomRegistrationStore& operator=(const UomRegistrationStore&) = delete;

    [[nodiscard]] bool ready() const { return _ready; }

    [[nodiscard]] QString error() const { return _error; }

    [[nodiscard]] bool save(const UomRegistration::Record& record);
    [[nodiscard]] std::optional<UomRegistration::Record> loadPending(const QString& scope, const QString& serial,
                                                                     const QString& uid);
    [[nodiscard]] std::optional<UomRegistration::Record> loadPendingForDevice(const QString& serial,
                                                                              const QString& uid);
    [[nodiscard]] std::optional<UomRegistration::Record> loadLatest(const QString& scope, const QString& serial,
                                                                    const QString& uid);
    [[nodiscard]] std::optional<UomRegistration::Record> loadLatestReport(const QString& scope, const QString& serial,
                                                                          const QString& uid);
    [[nodiscard]] bool migrateProfile(const UomRegistration::Record& previous,
                                      const UomRegistration::Record& replacement);
    [[nodiscard]] bool complete(const QString& id, const QString& stage = QStringLiteral("complete"));
    [[nodiscard]] int retryAfter(const QString& scope, const QString& fingerprint, qint64 nowMs = 0);
    [[nodiscard]] bool recordAttempt(const QString& scope, const QString& fingerprint, qint64 nowMs = 0);
    [[nodiscard]] bool audit(const UomRegistration::Record& record, const QString& event);

private:
    bool _initialize();
    bool _fail();
    QSqlDatabase _database;
    QString _connection;
    QString _error;
    bool _ready = false;
};
