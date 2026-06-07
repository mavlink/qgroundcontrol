// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QSQLDRIVER_P_H
#define QSQLDRIVER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists for the convenience
// of the QtSQL module. This header file may change from version to version
// without notice, or even be removed.
//
// We mean it.
//

#include <QtSql/private/qtsqlglobal_p.h>
#include "private/qobject_p.h"
#include "qsqldriver.h"
#include "qsqlerror.h"

QT_BEGIN_NAMESPACE

class QSqlDriverPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QSqlDriver)

public:
    QSqlDriverPrivate(QSqlDriver::DbmsType type = QSqlDriver::UnknownDbms)
      : QObjectPrivate(),
        dbmsType(type)
    { }

    QString connectionName;
    QSqlError error;
    QSql::NumericalPrecisionPolicy precisionPolicy = QSql::LowPrecisionDouble;
    QSqlDriver::DbmsType dbmsType;
    bool isOpen = false;
    bool isOpenError = false;
};

QT_END_NAMESPACE

#endif // QSQLDRIVER_P_H
