// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QSQLNULLDRIVER_P_H
#define QSQLNULLDRIVER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  This header file may
// change from version to version without notice, or even be
// removed.
//
// We mean it.
//

#include <QtSql/private/qtsqlglobal_p.h>
#include "QtCore/qvariant.h"
#include "QtSql/qsqldriver.h"
#include "QtSql/qsqlerror.h"
#include "QtSql/qsqlresult.h"

QT_BEGIN_NAMESPACE

class QSqlNullResult : public QSqlResult
{
public:
    inline explicit QSqlNullResult(const QSqlDriver* d): QSqlResult(d)
    { QSqlResult::setLastError(
            QSqlError(QLatin1StringView("Driver not loaded"), QLatin1StringView("Driver not loaded"), QSqlError::ConnectionError)); }
protected:
    inline QVariant data(int) override { return QVariant(); }
    inline bool reset (const QString&) override { return false; }
    inline bool fetch(int) override { return false; }
    inline bool fetchFirst() override { return false; }
    inline bool fetchLast() override { return false; }
    inline bool isNull(int) override { return false; }
    inline int size() override { return -1; }
    inline int numRowsAffected() override { return 0; }

    inline void setAt(int) override {}
    inline void setActive(bool) override {}
    inline void setLastError(const QSqlError&) override {}
    inline void setQuery(const QString&) override {}
    inline void setSelect(bool) override {}
    inline void setForwardOnly(bool) override {}

    inline bool exec() override { return false; }
    inline bool prepare(const QString&) override { return false; }
    inline bool savePrepare(const QString&) override { return false; }
    inline void bindValue(int, const QVariant&, QSql::ParamType) override {}
    inline void bindValue(const QString&, const QVariant&, QSql::ParamType) override {}
};

class QSqlNullDriver : public QSqlDriver
{
public:
    inline QSqlNullDriver(): QSqlDriver()
    { QSqlDriver::setLastError(
            QSqlError(QLatin1StringView("Driver not loaded"), QLatin1StringView("Driver not loaded"), QSqlError::ConnectionError)); }
    inline bool hasFeature(DriverFeature) const override { return false; }
    inline bool open(const QString &, const QString &, const QString &, const QString &, int, const QString&) override
    { return false; }
    inline void close() override {}
    inline QSqlResult *createResult() const override { return new QSqlNullResult(this); }

protected:
    inline void setOpen(bool) override {}
    inline void setOpenError(bool) override {}
    inline void setLastError(const QSqlError&) override {}
};

QT_END_NAMESPACE

#endif // QSQLNULLDRIVER_P_H
