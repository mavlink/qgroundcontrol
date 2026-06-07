// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKCLIPNODE_P_H
#define QQUICKCLIPNODE_P_H

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

#include <private/qtquickglobal_p.h>
#include <QtQuick/qsgnode.h>

QT_BEGIN_NAMESPACE

class Q_QUICK_EXPORT QQuickDefaultClipNode : public QSGClipNode
{
public:
    QQuickDefaultClipNode(const QRectF &);

    void setRect(const QRectF &);
    QRectF rect() const { return m_rect; }

    void setRadius(qreal radius);
    qreal radius() const { return m_radius; }

    virtual void update();

private:
    void updateGeometry();
    QRectF m_rect;
    qreal m_radius;

    uint m_dirty_geometry : 1;
    uint m_reserved : 31;

    QSGGeometry m_geometry;
};

QT_END_NAMESPACE

#endif // QQUICKCLIPNODE_P_H
