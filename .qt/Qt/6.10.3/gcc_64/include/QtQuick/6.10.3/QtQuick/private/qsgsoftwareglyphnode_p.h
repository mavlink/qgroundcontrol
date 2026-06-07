// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGSOFTWAREGLYPHNODE_H
#define QSGSOFTWAREGLYPHNODE_H

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

class QSGSoftwareGlyphNode : public QSGGlyphNode
{
public:
    QSGSoftwareGlyphNode();

    void setGlyphs(const QPointF &position, const QGlyphRun &glyphs) override;
    void setColor(const QColor &color) override;
    void setStyle(QQuickText::TextStyle style) override;
    void setStyleColor(const QColor &color) override;
    QPointF baseLine() const override;
    void setPreferredAntialiasingMode(AntialiasingMode) override;
    void update() override;

    void paint(QPainter *painter);

private:
    void recalculateBoundingRect();
    QPointF m_position;
    QGlyphRun m_glyphRun;
    QColor m_color;
    QSGGeometry m_geometry;
    QQuickText::TextStyle m_style;
    QColor m_styleColor;
};

QT_END_NAMESPACE

#endif // QSGSOFTWAREGLYPHNODE_H
