// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGDEFAULTRECTANGLENODE_P_H
#define QSGDEFAULTRECTANGLENODE_P_H

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

#include <QtGui/qcolor.h>
#include <QtQuick/qsgrectanglenode.h>
#include <QtQuick/qsgvertexcolormaterial.h>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class QSGDefaultRectangleNode : public QSGRectangleNode
{
public:
    QSGDefaultRectangleNode();

    void setRect(const QRectF &rect) override;
    QRectF rect() const override;

    void setColor(const QColor &color) override;
    QColor color() const override;

private:
    QSGVertexColorMaterial m_material;
    QSGGeometry m_geometry;
    QColor m_color;
};

QT_END_NAMESPACE

#endif // QSGDEFAULTRECTANGLENODE_P_H
