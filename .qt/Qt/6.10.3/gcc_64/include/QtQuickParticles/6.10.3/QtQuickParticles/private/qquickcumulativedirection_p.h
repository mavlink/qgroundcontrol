// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQuickCUMULATIVEDIRECTION_P_H
#define QQuickCUMULATIVEDIRECTION_P_H

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
#include "qquickdirection_p.h"
#include <QQmlListProperty>
#include <QtQml/qqml.h>
#include <QtQuickParticles/qtquickparticlesexports.h>

QT_BEGIN_NAMESPACE

class Q_QUICKPARTICLES_EXPORT QQuickCumulativeDirection : public QQuickDirection
{
    Q_OBJECT
    Q_PROPERTY(QQmlListProperty<QQuickDirection> directions READ directions)
    Q_CLASSINFO("DefaultProperty", "directions")
    QML_NAMED_ELEMENT(CumulativeDirection)
    QML_ADDED_IN_VERSION(2, 0)
public:
    explicit QQuickCumulativeDirection(QObject *parent = nullptr);
    QQmlListProperty<QQuickDirection> directions();
    QPointF sample(const QPointF &from) override;
private:
    QList<QQuickDirection*> m_directions;
};

QT_END_NAMESPACE

#endif // QQuickCUMULATIVEDIRECTION_P_H
