// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKDEFERREDEXECUTE_P_P_H
#define QQUICKDEFERREDEXECUTE_P_P_H

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

#include <QtCore/qglobal.h>
#include <QtQuickTemplates2/private/qquickdeferredpointer_p_p.h>
#include <QtQuickTemplates2/private/qtquicktemplates2global_p.h>

#include <QtQml/private/qqmlvme_p.h>

QT_BEGIN_NAMESPACE

class QString;
class QObject;

namespace QtQuickPrivate {
    Q_QUICKTEMPLATES2_EXPORT void beginDeferred(QObject *object, const QString &property,
        QQuickUntypedDeferredPointer *delegate, bool isOwnState, QQmlEngine *engine = nullptr);
    Q_QUICKTEMPLATES2_EXPORT void cancelDeferred(QObject *object, const QString &property);
    Q_QUICKTEMPLATES2_EXPORT void completeDeferred(QObject *object, const QString &property,
        QQuickUntypedDeferredPointer *delegate, QQmlEngine *engine = nullptr);
}

template<typename T>
void quickBeginDeferred(QObject *object, const QString &property, QQuickDeferredPointer<T> &delegate)
{
    if (!QQmlVME::componentCompleteEnabled())
        return;

    QtQuickPrivate::beginDeferred(object, property, &delegate, delegate.setExecuting(true));
    delegate.setExecuting(false);
}

template<typename T>
void quickBeginAttachedDeferred(QObject *object, const QString &property,
    QQuickDeferredPointer<T> &delegate, QQmlEngine *engine)
{
    if (!QQmlVME::componentCompleteEnabled())
        return;

    QtQuickPrivate::beginDeferred(object, property, &delegate, delegate.setExecuting(true), engine);
    delegate.setExecuting(false);
}

inline void quickCancelDeferred(QObject *object, const QString &property)
{
    QtQuickPrivate::cancelDeferred(object, property);
}

template<typename T>
void quickCompleteDeferred(QObject *object, const QString &property, QQuickDeferredPointer<T> &delegate)
{
    Q_ASSERT(!delegate.wasExecuted());
    QtQuickPrivate::completeDeferred(object, property, &delegate);
    delegate.setExecuted();
}

template<typename T>
void quickCompleteAttachedDeferred(QObject *object, const QString &property,
    QQuickDeferredPointer<T> &delegate, QQmlEngine *engine)
{
    Q_ASSERT(!delegate.wasExecuted());
    QtQuickPrivate::completeDeferred(object, property, &delegate, engine);
    delegate.setExecuted();
}

QT_END_NAMESPACE

#endif // QQUICKDEFERREDEXECUTE_P_P_H
