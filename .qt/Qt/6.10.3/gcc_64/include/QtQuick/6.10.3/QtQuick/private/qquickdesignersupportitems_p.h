// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef DESIGNERSUPPORTITEM_H
#define DESIGNERSUPPORTITEM_H

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

#include <QObject>
#include <QString>
#include <QVariant>
#include <QList>
#include <QByteArray>
#include <QTypeRevision>
#include <QQmlContext>
#include <QQmlListReference>

QT_BEGIN_NAMESPACE

class Q_QUICK_EXPORT QQuickDesignerSupportItems
{
public:
    static QObject *createPrimitive(const QString &typeName, QTypeRevision version, QQmlContext *context);
    static QObject *createComponent(const QUrl &componentUrl, QQmlContext *context);
    static void tweakObjects(QObject *object);
    static bool objectWasDeleted(QObject *object);
    static void disableNativeTextRendering(QQuickItem *item);
    static void disableTextCursor(QQuickItem *item);
    static void disableTransition(QObject *object);
    static void disableBehaivour(QObject *object);
    static void stopUnifiedTimer();
    static void registerFixResourcePathsForObjectCallBack(void (*callback)(QObject *));
};

QT_END_NAMESPACE

#endif // DESIGNERSUPPORTITEM_H
