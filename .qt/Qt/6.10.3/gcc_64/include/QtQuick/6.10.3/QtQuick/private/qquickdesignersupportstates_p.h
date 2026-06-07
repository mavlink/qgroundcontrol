// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef DESIGNERSUPPORTSTATES_H
#define DESIGNERSUPPORTSTATES_H

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

class Q_QUICK_EXPORT QQuickDesignerSupportStates
{
public:
    static bool isStateActive(QObject *object, QQmlContext *context);
    static void activateState(QObject *object, QQmlContext *context);
    static void deactivateState(QObject *object);
    static bool changeValueInRevertList(QObject *state,
                                        QObject *target,
                                        const QQuickDesignerSupport::PropertyName &propertyName,
                                        const QVariant &value);

    static bool updateStateBinding(QObject *state, QObject *target,
                                   const QQuickDesignerSupport::PropertyName &propertyName,
                                   const QString &expression);

    static bool resetStateProperty(QObject *state, QObject *target,
                                   const QQuickDesignerSupport::PropertyName &propertyName,
                                   const QVariant &);
};

QT_END_NAMESPACE

#endif // DESIGNERSUPPORTSTATES_H
