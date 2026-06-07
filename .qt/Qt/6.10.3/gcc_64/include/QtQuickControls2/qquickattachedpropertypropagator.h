// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKATTACHEDOBJECT_H
#define QQUICKATTACHEDOBJECT_H

#include <QtCore/qobject.h>
#include <QtQuickControls2/qtquickcontrols2global.h>

QT_BEGIN_NAMESPACE

class QQuickAttachedPropertyPropagatorPrivate;

class Q_QUICKCONTROLS2_EXPORT QQuickAttachedPropertyPropagator : public QObject
{
    Q_OBJECT

public:
    explicit QQuickAttachedPropertyPropagator(QObject *parent = nullptr);
    ~QQuickAttachedPropertyPropagator();

    QList<QQuickAttachedPropertyPropagator *> attachedChildren() const;

    QQuickAttachedPropertyPropagator *attachedParent() const;

protected:
    void initialize();

    virtual void attachedParentChange(QQuickAttachedPropertyPropagator *newParent, QQuickAttachedPropertyPropagator *oldParent);

private:
#ifndef QT_NO_DEBUG_STREAM
    friend Q_QUICKCONTROLS2_EXPORT QDebug operator<<(QDebug debug, const QQuickAttachedPropertyPropagator *propagator);
#endif

    Q_DECLARE_PRIVATE(QQuickAttachedPropertyPropagator)
};

QT_END_NAMESPACE

#endif // QQUICKATTACHEDOBJECT_H
