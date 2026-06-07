// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGCURVEGLYPHNODE_P_H
#define QSGCURVEGLYPHNODE_P_H

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

#include <qtquickexports.h>
#include <private/qsgadaptationlayer_p.h>
#include <private/qsgbasicglyphnode_p.h>

QT_BEGIN_NAMESPACE

class QSGCurveGlyphAtlas;
class QSGCurveFillNode;
class QSGCurveAbstractNode;

class Q_QUICK_EXPORT QSGCurveGlyphNode : public QSGGlyphNode
{
public:
    QSGCurveGlyphNode(QSGRenderContext *context);
    ~QSGCurveGlyphNode();
    void setGlyphs(const QPointF &position, const QGlyphRun &glyphs) override;
    void update() override;
    void preprocess() override;
    void setPreferredAntialiasingMode(AntialiasingMode) override;
    void updateGeometry();
    void setColor(const QColor &color) override;
    void setStyle(QQuickText::TextStyle style) override;

    void setStyleColor(const QColor &color) override;
    QPointF baseLine() const override { return m_baseLine; }

private:
    QSGRenderContext *m_context;
    QSGGeometry m_geometry;
    QColor m_color = Qt::black;

    struct GlyphInfo {
        QVector<quint32> indexes;
        QVector<QPointF> positions;
    };

    uint m_dirtyGeometry: 1;
    qreal m_fontSize = 0.0f;
    QGlyphRun m_glyphs;
    QQuickText::TextStyle m_style;
    QColor m_styleColor;
    QPointF m_baseLine;
    QPointF m_position;

    QSGCurveFillNode *m_glyphNode = nullptr;
    QSGCurveAbstractNode *m_styleNode = nullptr;
};

QT_END_NAMESPACE

#endif
