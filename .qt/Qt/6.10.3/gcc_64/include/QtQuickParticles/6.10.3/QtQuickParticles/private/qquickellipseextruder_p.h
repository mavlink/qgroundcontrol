// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef ELLIPSEEXTRUDER_H
#define ELLIPSEEXTRUDER_H

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
#include "qquickparticleextruder_p.h"

QT_BEGIN_NAMESPACE

class Q_QUICKPARTICLES_EXPORT QQuickEllipseExtruder : public QQuickParticleExtruder
{
    Q_OBJECT
    Q_PROPERTY(bool fill READ fill WRITE setFill NOTIFY fillChanged)//###Use base class? If it's still box
    QML_NAMED_ELEMENT(EllipseShape)
    QML_ADDED_IN_VERSION(2, 0)
public:
    explicit QQuickEllipseExtruder(QObject *parent = nullptr);
    QPointF extrude(const QRectF &) override;
    bool contains(const QRectF &bounds, const QPointF &point) override;

    bool fill() const
    {
        return m_fill;
    }

Q_SIGNALS:

    void fillChanged(bool arg);

public Q_SLOTS:

    void setFill(bool arg)
    {
        if (m_fill != arg) {
            m_fill = arg;
            Q_EMIT fillChanged(arg);
        }
    }
private:
    bool m_fill;
};

QT_END_NAMESPACE
#endif // ELLIPSEEXTRUDER_H
