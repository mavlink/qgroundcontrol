// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLLOCALSTORAGE_P_H
#define QQMLLOCALSTORAGE_P_H

#include "qqmllocalstorageglobal_p.h"

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qobject.h>
#include <QtQml/qqml.h>
#include <QtQml/private/qv4engine_p.h>

QT_BEGIN_NAMESPACE

class Q_QMLLOCALSTORAGE_EXPORT QQmlLocalStorage : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(LocalStorage)
    QML_ADDED_IN_VERSION(2, 0)
    QML_SINGLETON

public:
    QQmlLocalStorage(QObject *parent = nullptr) : QObject(parent) {}
    ~QQmlLocalStorage() override = default;

    Q_INVOKABLE void openDatabaseSync(QQmlV4FunctionPtr args);
};

QT_END_NAMESPACE

#endif // QQMLLOCALSTORAGE_P_H
