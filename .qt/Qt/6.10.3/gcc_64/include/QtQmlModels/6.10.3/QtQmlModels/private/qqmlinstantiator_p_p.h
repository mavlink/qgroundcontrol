// Copyright (C) 2016 Research In Motion.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLINSTANTIATOR_P_P_H
#define QQMLINSTANTIATOR_P_P_H

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

#include "qqmlinstantiator_p.h"
#include <QObject>
#include <private/qobject_p.h>
#include <private/qqmlchangeset_p.h>
#include <private/qqmlobjectmodel_p.h>

#include <QtCore/qpointer.h>

QT_REQUIRE_CONFIG(qml_object_model);

QT_BEGIN_NAMESPACE

class Q_QMLMODELS_EXPORT QQmlInstantiatorPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQmlInstantiator)

public:
    QQmlInstantiatorPrivate();

    void clear();
    void regenerate();
#if QT_CONFIG(qml_delegate_model)
    void makeModel();
#endif
    void _q_createdItem(int, QObject *);
    void _q_modelUpdated(const QQmlChangeSet &, bool);
    QObject *modelObject(int index, bool async);

    static QQmlInstantiatorPrivate *get(QQmlInstantiator *instantiator) { return instantiator->d_func(); }
    static const QQmlInstantiatorPrivate *get(const QQmlInstantiator *instantiator) { return instantiator->d_func(); }

    bool componentComplete:1;
    bool effectiveReset:1;
    bool active:1;
    bool async:1;
#if QT_CONFIG(qml_delegate_model)
    bool ownModel:1;
    QQmlDelegateModel::DelegateModelAccess delegateModelAccess = QQmlDelegateModel::Qt5ReadWrite;
#endif
    int requestedIndex = -1;
    QQmlInstanceModel *instanceModel = nullptr;
    QQmlComponent *delegate = nullptr;
    QVector<QPointer<QObject> > objects;
};

QT_END_NAMESPACE

#endif // QQMLCREATOR_P_P_H
