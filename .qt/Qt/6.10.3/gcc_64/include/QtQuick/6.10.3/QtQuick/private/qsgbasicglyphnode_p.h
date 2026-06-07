// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGBASICGLYPHNODE_P_H
#define QSGBASICGLYPHNODE_P_H

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

#include <private/qsgadaptationlayer_p.h>

QT_BEGIN_NAMESPACE

class QSGMaterial;

class Q_QUICK_EXPORT QSGBasicGlyphNode: public QSGGlyphNode
{
public:
    QSGBasicGlyphNode();
    virtual ~QSGBasicGlyphNode();

    QPointF baseLine() const override { return m_baseLine; }
    void setGlyphs(const QPointF &position, const QGlyphRun &glyphs) override;
    void setColor(const QColor &color) override;

    void setPreferredAntialiasingMode(AntialiasingMode) override { }
    void setStyle(QQuickText::TextStyle) override;
    void setStyleColor(const QColor &) override;

    virtual void setMaterialColor(const QColor &color) = 0;
    void update() override = 0;

protected:
    QGlyphRun m_glyphs;
    QPointF m_position;
    QColor m_color;
    QQuickText::TextStyle m_style;
    QColor m_styleColor;

    QPointF m_baseLine;
    QSGMaterial *m_material;

    QSGGeometry m_geometry;
};

QT_END_NAMESPACE

#endif
