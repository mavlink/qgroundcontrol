// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef LINEEXTRUDER_H
#define LINEEXTRUDER_H

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

class Q_QUICKPARTICLES_EXPORT QQuickLineExtruder : public QQuickParticleExtruder
{
    Q_OBJECT
    //Default is topleft to bottom right. Flipped makes it topright to bottom left
    Q_PROPERTY(bool mirrored READ mirrored WRITE setMirrored NOTIFY mirroredChanged)
    QML_NAMED_ELEMENT(LineShape)
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickLineExtruder(QObject *parent = nullptr);
    QPointF extrude(const QRectF &) override;
    bool mirrored() const
    {
        return m_mirrored;
    }

Q_SIGNALS:

    void mirroredChanged(bool arg);

public Q_SLOTS:

    void setMirrored(bool arg)
    {
        if (m_mirrored != arg) {
            m_mirrored = arg;
            Q_EMIT mirroredChanged(arg);
        }
    }
private:
    bool m_mirrored;
};

QT_END_NAMESPACE

#endif // LINEEXTRUDER_H
