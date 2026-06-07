// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QSQLCACHEDRESULT_P_H
#define QSQLCACHEDRESULT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtSql/private/qtsqlglobal_p.h>
#include "QtSql/qsqlresult.h"
#include "QtSql/private/qsqlresult_p.h"
#include <QtCore/qcontainerfwd.h>

QT_BEGIN_NAMESPACE

class QVariant;

class QSqlCachedResultPrivate;

class Q_SQL_EXPORT QSqlCachedResult: public QSqlResult
{
    Q_DECLARE_PRIVATE(QSqlCachedResult)

public:
    typedef QList<QVariant> ValueCache;

protected:
    QSqlCachedResult(QSqlCachedResultPrivate &d);

    void init(int colCount);
    void cleanup();
    void clearValues();

    virtual bool gotoNext(ValueCache &values, int index) = 0;

    QVariant data(int i) override;
    bool isNull(int i) override;
    bool fetch(int i) override;
    bool fetchNext() override;
    bool fetchPrevious() override;
    bool fetchFirst() override;
    bool fetchLast() override;

    int colCount() const;
    ValueCache &cache();

    void virtual_hook(int id, void *data) override;
    void detachFromResultSet() override;
    void setNumericalPrecisionPolicy(QSql::NumericalPrecisionPolicy policy) override;
private:
    bool cacheNext();
};

class Q_SQL_EXPORT QSqlCachedResultPrivate: public QSqlResultPrivate
{
    Q_DECLARE_PUBLIC(QSqlCachedResult)

public:
    using QSqlResultPrivate::QSqlResultPrivate;

    bool canSeek(int i) const;
    inline int cacheCount() const;
    void init(int count, bool fo);
    void cleanup();
    int nextIndex();
    void revertLast();

    QSqlCachedResult::ValueCache cache;
    int rowCacheEnd = 0;
    int colCount = 0;
    bool atEnd = false;
};

QT_END_NAMESPACE

#endif // QSQLCACHEDRESULT_P_H
