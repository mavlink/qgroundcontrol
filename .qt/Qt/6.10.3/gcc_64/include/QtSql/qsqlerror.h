// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QSQLERROR_H
#define QSQLERROR_H

#include <QtSql/qtsqlglobal.h>
#include <QtCore/qshareddata.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

class QSqlErrorPrivate;
QT_DECLARE_QESDP_SPECIALIZATION_DTOR_WITH_EXPORT(QSqlErrorPrivate, Q_SQL_EXPORT)

class Q_SQL_EXPORT QSqlError
{
public:
    enum ErrorType {
        NoError,
        ConnectionError,
        StatementError,
        TransactionError,
        UnknownError
    };
    QSqlError(const QString &driverText = QString(),
              const QString &databaseText = QString(),
              ErrorType type = NoError,
              const QString &nativeErrorCode = QString());
    QSqlError(const QSqlError &other);
    QSqlError(QSqlError &&other) noexcept = default;
    QSqlError& operator=(const QSqlError &other);
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_PURE_SWAP(QSqlError)
    ~QSqlError();

    bool operator==(const QSqlError &other) const;
    bool operator!=(const QSqlError &other) const;

    void swap(QSqlError &other) noexcept { d.swap(other.d); }

    QString driverText() const;
    QString databaseText() const;
    ErrorType type() const;
    QString nativeErrorCode() const;
    QString text() const;
    bool isValid() const;

private:
    QExplicitlySharedDataPointer<QSqlErrorPrivate> d;
};

Q_DECLARE_SHARED(QSqlError)

#ifndef QT_NO_DEBUG_STREAM
Q_SQL_EXPORT QDebug operator<<(QDebug, const QSqlError &);
#endif

QT_END_NAMESPACE

#endif // QSQLERROR_H
