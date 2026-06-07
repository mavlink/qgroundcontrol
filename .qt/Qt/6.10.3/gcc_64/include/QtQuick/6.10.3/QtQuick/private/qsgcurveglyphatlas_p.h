// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGCURVEGLYPHATLAS_P_H
#define QSGCURVEGLYPHATLAS_P_H

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

#include <QtGui/qrawfont.h>
#include <QtGui/private/qtextengine_p.h>
#include <QtQuick/qtquickexports.h>

QT_BEGIN_NAMESPACE

class QSGCurveFillNode;
class QSGCurveStrokeNode;

class Q_QUICK_EXPORT QSGCurveGlyphAtlas
{
public:
    QSGCurveGlyphAtlas(const QRawFont &font);
    virtual ~QSGCurveGlyphAtlas();

    void populate(const QList<glyph_t> &glyphs);
    void addGlyph(QSGCurveFillNode *node,
                  glyph_t glyph,
                  const QPointF &position,
                  qreal pixelSize) const;
    void addStroke(QSGCurveStrokeNode *node,
                   glyph_t glyph,
                   const QPointF &position) const;

    qreal fontSize() const
    {
        return m_font.pixelSize();
    }

private:
    struct Glyph
    {
        QList<QVector2D> vertices;
        QList<QVector3D> uvs;
        QList<QVector2D> normals;
        QList<QVector2D> duvdx;
        QList<QVector2D> duvdy;

        QList<QVector2D> strokeVertices;
        QList<QVector2D> strokeUvs;
        QList<QVector2D> strokeNormals;
        QList<bool> strokeElementIsLine;
    };

    QHash<glyph_t, Glyph> m_glyphs;
    QRawFont m_font;
};

QT_END_NAMESPACE


#endif // QSGCURVEGLYPHATLAS_P_H
