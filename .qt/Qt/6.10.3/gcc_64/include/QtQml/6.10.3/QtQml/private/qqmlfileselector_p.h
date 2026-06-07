// Copyright (C) 2016 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLFILESELECTOR_P_H
#define QQMLFILESELECTOR_P_H

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

#include "qqmlfileselector.h"
#include <QSet>
#include <QQmlAbstractUrlInterceptor>
#include <private/qobject_p.h>
#include <private/qtqmlglobal_p.h>

#include <QtCore/qpointer.h>

QT_BEGIN_NAMESPACE

class QFileSelector;
class QQmlFileSelectorInterceptor;
class Q_QML_EXPORT QQmlFileSelectorPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQmlFileSelector)
public:
    QQmlFileSelectorPrivate();
    ~QQmlFileSelectorPrivate();

    QFileSelector* selector;
    QPointer<QQmlEngine> engine;
    bool ownSelector;
    QScopedPointer<QQmlFileSelectorInterceptor> myInstance;
};

class Q_QML_EXPORT QQmlFileSelectorInterceptor : public QQmlAbstractUrlInterceptor
{
public:
    QQmlFileSelectorInterceptor(QQmlFileSelectorPrivate* pd);
    QQmlFileSelectorPrivate* d;
protected:
    QUrl intercept(const QUrl &path, DataType type) override;
};

QT_END_NAMESPACE

#endif
