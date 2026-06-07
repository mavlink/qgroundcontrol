// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKDELIVERYAGENT_P_H
#define QQUICKDELIVERYAGENT_P_H

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

#include <QtQuick/private/qtquickglobal_p.h>

#include <QtQml/qqml.h>
#include <QtQml/private/qqmlglobal_p.h>

#include <QtCore/qobject.h>
#include <QtGui/qevent.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcPtr)
Q_DECLARE_LOGGING_CATEGORY(lcHoverTrace)
Q_DECLARE_LOGGING_CATEGORY(lcHoverCursor)
Q_DECLARE_LOGGING_CATEGORY(lcTouchTarget)

class QQuickItem;
class QQuickDeliveryAgentPrivate;

class Q_QUICK_EXPORT QQuickDeliveryAgent : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickDeliveryAgent)

public:
    struct Q_QUICK_EXPORT Transform
    {
        virtual ~Transform();
        virtual QPointF map(const QPointF &point) = 0;
    };

    explicit QQuickDeliveryAgent(QQuickItem *rootItem);
    virtual ~QQuickDeliveryAgent();

    QQuickItem *rootItem() const;

    Transform *sceneTransform() const;
    void setSceneTransform(Transform *transform);
    bool event(QEvent *ev) override;

Q_SIGNALS:

private:
    Q_DISABLE_COPY(QQuickDeliveryAgent)
};

#ifndef QT_NO_DEBUG_STREAM
QDebug Q_QUICK_EXPORT operator<<(QDebug debug, const QQuickDeliveryAgent *da);
#endif

QT_END_NAMESPACE

#endif // QQUICKDELIVERYAGENT_P_H
