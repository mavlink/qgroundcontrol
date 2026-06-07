// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef DESIGNERSUPPORTPROPERTYCHANGES_H
#define DESIGNERSUPPORTPROPERTYCHANGES_H

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

#include "qquickdesignersupport_p.h"

#include <QVariant>

QT_BEGIN_NAMESPACE

class Q_QUICK_EXPORT QQuickDesignerSupportPropertyChanges
{
public:
    static void detachFromState(QObject *propertyChanges);
    static void attachToState(QObject *propertyChanges);
    static QObject *targetObject(QObject *propertyChanges);
    static void removeProperty(QObject *propertyChanges, const QQuickDesignerSupport::PropertyName &propertyName);
    static QVariant getProperty(QObject *propertyChanges, const QQuickDesignerSupport::PropertyName &propertyName);
    static void changeValue(QObject *propertyChanges, const QQuickDesignerSupport::PropertyName &propertyName, const QVariant &value);
    static void changeExpression(QObject *propertyChanges, const QQuickDesignerSupport::PropertyName &propertyName, const QString &expression);
    static QObject *stateObject(QObject *propertyChanges);
    static bool isNormalProperty(const QQuickDesignerSupport::PropertyName &propertyName);
};

QT_END_NAMESPACE

#endif // DESIGNERSUPPORTPROPERTYCHANGES_H
