// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QSQLRESULT_P_H
#define QSQLRESULT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of qsql*model.h .  This header file may change from version to version
// without notice, or even be removed.
//
// We mean it.
//

#include <QtSql/private/qtsqlglobal_p.h>
#include <QtCore/qpointer.h>
#include <QtCore/qhash.h>
#include "qsqlerror.h"
#include "qsqlresult.h"
#include "qsqldriver.h"

QT_BEGIN_NAMESPACE

// convenience method Q*ResultPrivate::drv_d_func() returns pointer to private driver. Compare to Q_DECLARE_PRIVATE in qglobal.h.
#define Q_DECLARE_SQLDRIVER_PRIVATE(Class) \
    inline const Class##Private* drv_d_func() const { return !sqldriver ? nullptr : reinterpret_cast<const Class *>(static_cast<const QSqlDriver*>(sqldriver))->d_func(); } \
    inline Class##Private* drv_d_func()  { return !sqldriver ? nullptr : reinterpret_cast<Class *>(static_cast<QSqlDriver*>(sqldriver))->d_func(); }

struct QHolder {
    QHolder(const QString &hldr = QString(), qsizetype index = -1): holderName(hldr), holderPos(index) { }
    bool operator==(const QHolder &h) const { return h.holderPos == holderPos && h.holderName == holderName; }
    bool operator!=(const QHolder &h) const { return h.holderPos != holderPos || h.holderName != holderName; }
    QString holderName;
    qsizetype holderPos;
};

class Q_SQL_EXPORT QSqlResultPrivate
{
    Q_DECLARE_PUBLIC(QSqlResult)

public:
    QSqlResultPrivate(QSqlResult *q, const QSqlDriver *drv)
      : q_ptr(q),
        sqldriver(const_cast<QSqlDriver *>(drv))
    { }
    virtual ~QSqlResultPrivate() = default;

    void clearValues()
    {
        values.clear();
        bindCount = 0;
    }

    void resetBindCount()
    {
        bindCount = 0;
    }

    void clearIndex()
    {
        indexes.clear();
        holders.clear();
        types.clear();
    }

    void clear()
    {
        clearValues();
        clearIndex();
    }

    virtual QString fieldSerial(qsizetype) const;
    QString positionalToNamedBinding(const QString &query) const;
    QString namedToPositionalBinding(const QString &query);
    QString holderAt(int index) const;

    QSqlResult *q_ptr = nullptr;
    QPointer<QSqlDriver> sqldriver;
    QString sql;
    QSqlError error;

    QString executedQuery;
    QHash<int, QSql::ParamType> types;
    QList<QVariant> values;
    using IndexMap = QHash<QString, QList<int>>;
    IndexMap indexes;

    using QHolderVector = QList<QHolder>;
    QHolderVector holders;

    QSqlResult::BindingSyntax binds = QSqlResult::PositionalBinding;
    QSql::NumericalPrecisionPolicy precisionPolicy = QSql::LowPrecisionDouble;
    int idx = QSql::BeforeFirstRow;
    int bindCount = 0;
    bool active = false;
    bool isSel = false;
    bool forwardOnly = false;
    bool positionalBindingEnabled = true;

    static bool isVariantNull(const QVariant &variant);
};

QT_END_NAMESPACE

#endif // QSQLRESULT_P_H
