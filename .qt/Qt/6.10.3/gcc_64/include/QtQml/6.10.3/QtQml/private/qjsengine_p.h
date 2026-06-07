// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QJSENGINE_P_H
#define QJSENGINE_P_H

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

#include <QtCore/private/qobject_p.h>
#include <QtCore/qmutex.h>
#include <QtCore/qproperty.h>
#include "qjsengine.h"
#include "private/qtqmlglobal_p.h"
#include <private/qqmlmetatype_p.h>

QT_BEGIN_NAMESPACE

class QQmlPropertyCache;

namespace QV4 {
struct ExecutionEngine;
}

class Q_QML_EXPORT QJSEnginePrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QJSEngine)

public:
    static QJSEnginePrivate* get(QJSEngine*e) { return e->d_func(); }
    static const QJSEnginePrivate* get(const QJSEngine*e) { return e->d_func(); }
    static QJSEnginePrivate* get(QV4::ExecutionEngine *e);

    QJSEnginePrivate() = default;
    ~QJSEnginePrivate() override;

    static void addToDebugServer(QJSEngine *q);
    static void removeFromDebugServer(QJSEngine *q);

    void uiLanguageChanged()
    {
        Q_Q(QJSEngine);
        if (q)
            Q_EMIT q->uiLanguageChanged();
    }
    Q_OBJECT_BINDABLE_PROPERTY(QJSEnginePrivate, QString, uiLanguage, &QJSEnginePrivate::uiLanguageChanged);
};

QT_END_NAMESPACE

#endif // QJSENGINE_P_H
