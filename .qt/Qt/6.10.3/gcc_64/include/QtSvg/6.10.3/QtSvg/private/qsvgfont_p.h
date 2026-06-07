// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSVGFONT_P_H
#define QSVGFONT_P_H

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

#include "qpainterpath.h"
#include "qlist.h"
#include "qstring.h"
#include "qsvgstyle_p.h"
#include "qtsvgglobal_p.h"

#include <memory>

QT_BEGIN_NAMESPACE

class Q_SVG_EXPORT QSvgGlyph
{
public:
    QSvgGlyph(const QString &unicode, const QPainterPath &path, qreal horizAdvX);
    QSvgGlyph() : m_horizAdvX(0) {}

    QString m_unicode;
    QPainterPath m_path;
    qreal m_horizAdvX;
};


class Q_SVG_EXPORT QSvgFont : public QSvgRefCounted
{
public:
    static constexpr qreal DEFAULT_UNITS_PER_EM = 1000;
    QSvgFont(qreal horizAdvX);

    void setFamilyName(const QString &name);
    QString familyName() const;

    void setUnitsPerEm(qreal upem);

    void addGlyph(const QString &unicode, const QPainterPath &path, qreal horizAdvX = -1);
    bool addMissingGlyph(const QPainterPath &path, qreal horizAdvX);

    void draw(QPainter *p, const QPointF &point, const QString &str,
              qreal pixelSize, Qt::Alignment alignment) const;
    QRectF boundingRect(QPainter *p, const QPointF &point, const QString &str,
                        qreal pixelSize, Qt::Alignment alignment) const;
    const QSvgGlyph *findFirstGlyphFor(QStringView text) const;

public:
    QString m_familyName;
    qreal m_unitsPerEm = DEFAULT_UNITS_PER_EM;
    qreal m_horizAdvX;
    // not about a missing <glyph> element, but the font's <missing-glyph> element:
    std::unique_ptr<const QSvgGlyph> m_missingGlyph;
    // The following needs to preserve the order of glyphs because
    // "17.6 Glyph selection rules" in SVG Tiny 1.2 reads:
    // "the 'font' element must be searched from its first
    // 'glyph' element to its last in logical order"
    QList<QSvgGlyph> m_glyphs;

private:
    // to speed up finding glyphs
    mutable QHash<char32_t, QList<qsizetype>> m_possibleGlyphIndicesForChar;
    mutable qsizetype m_firstUnscannedGlyphIdx = 0;

    void draw_helper(QPainter *p, const QPointF &point, const QString &str, qreal pixelSize,
                     Qt::Alignment alignment, QRectF *boundingRect = nullptr) const;
};

QT_END_NAMESPACE

#endif // QSVGFONT_P_H
