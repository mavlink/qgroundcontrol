// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QSQLDATABASE_H
#define QSQLDATABASE_H

#include <QtSql/qtsqlglobal.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qstring.h>

// clazy:excludeall=qproperty-without-notify
QT_BEGIN_NAMESPACE


class QSqlError;
class QSqlDriver;
class QSqlIndex;
class QSqlRecord;
class QSqlQuery;
class QSqlDatabasePrivate;
class QThread;

class Q_SQL_EXPORT QSqlDriverCreatorBase
{
public:
    virtual ~QSqlDriverCreatorBase();
    virtual QSqlDriver *createObject() const = 0;
};

template <class T>
class QSqlDriverCreator : public QSqlDriverCreatorBase
{
public:
    QSqlDriver *createObject() const override { return new T; }
};

struct QSqlDatabaseDefaultConnectionName
{
    // separate class because of the static inline constexpr variable
    static constexpr const char defaultConnection[] = "qt_sql_default_connection";
    static QString defaultConnectionName() noexcept
    {
        using namespace Qt::StringLiterals;
        return u"qt_sql_default_connection"_s;
    }
};

class Q_SQL_EXPORT QSqlDatabase : public QSqlDatabaseDefaultConnectionName
{
    Q_GADGET
    Q_PROPERTY(QSql::NumericalPrecisionPolicy numericalPrecisionPolicy READ numericalPrecisionPolicy WRITE setNumericalPrecisionPolicy)

public:

    QSqlDatabase();
    QSqlDatabase(const QSqlDatabase &other);
    ~QSqlDatabase();

    QSqlDatabase &operator=(const QSqlDatabase &other);

    bool open();
    bool open(const QString& user, const QString& password);
    void close();
    bool isOpen() const;
    bool isOpenError() const;
    QStringList tables(QSql::TableType type = QSql::Tables) const;
    QSqlIndex primaryIndex(const QString& tablename) const;
    QSqlRecord record(const QString& tablename) const;
#if QT_DEPRECATED_SINCE(6, 6)
    QT_DEPRECATED_VERSION_X_6_6("Use QSqlQuery::exec() instead.")
    QSqlQuery exec(const QString& query = QString()) const;
#endif
    QSqlError lastError() const;
    bool isValid() const;

    bool transaction();
    bool commit();
    bool rollback();

    void setDatabaseName(const QString& name);
    void setUserName(const QString& name);
    void setPassword(const QString& password);
    void setHostName(const QString& host);
    void setPort(int p);
    void setConnectOptions(const QString& options = QString());
    QString databaseName() const;
    QString userName() const;
    QString password() const;
    QString hostName() const;
    QString driverName() const;
    int port() const;
    QString connectOptions() const;
    QString connectionName() const;
    void setNumericalPrecisionPolicy(QSql::NumericalPrecisionPolicy precisionPolicy);
    QSql::NumericalPrecisionPolicy numericalPrecisionPolicy() const;
    bool moveToThread(QThread *targetThread);
    QThread *thread() const;

    QSqlDriver* driver() const;

#if QT_SQL_REMOVED_SINCE(6, 10)
    static const char *defaultConnection;
#endif

    static QSqlDatabase addDatabase(const QString& type,
                                 const QString &connectionName = defaultConnectionName());
    static QSqlDatabase addDatabase(QSqlDriver* driver,
                                 const QString &connectionName = defaultConnectionName());
    static QSqlDatabase cloneDatabase(const QSqlDatabase &other, const QString &connectionName);
    static QSqlDatabase cloneDatabase(const QString &other, const QString &connectionName);
    static QSqlDatabase database(const QString &connectionName = defaultConnectionName(),
                                 bool open = true);
    static void removeDatabase(const QString &connectionName);
    static bool contains(const QString &connectionName = defaultConnectionName());
    static QStringList drivers();
    static QStringList connectionNames();
    static void registerSqlDriver(const QString &name, QSqlDriverCreatorBase *creator);
    static bool isDriverAvailable(const QString &name);

protected:
    explicit QSqlDatabase(const QString& type);
    explicit QSqlDatabase(QSqlDriver* driver);

private:
    friend class QSqlDatabasePrivate;
    QSqlDatabasePrivate *d;
};

#ifndef QT_NO_DEBUG_STREAM
Q_SQL_EXPORT QDebug operator<<(QDebug, const QSqlDatabase &);
#endif

QT_END_NAMESPACE

#endif // QSQLDATABASE_H
