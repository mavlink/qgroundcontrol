// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFONTENGINEGLYPHCACHE_P_H
#define QFONTENGINEGLYPHCACHE_P_H

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

#include <QtCore/qshareddata.h>
#include <QtGui/private/qtguiglobal_p.h>
#include "QtCore/qatomic.h"
#include <QtCore/qvarlengtharray.h>
#include "private/qfont_p.h"
#include "private/qfontengine_p.h"



QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT QFontEngineGlyphCache: public QSharedData
{
public:
    QFontEngineGlyphCache(QFontEngine::GlyphFormat format, const QTransform &matrix, const QColor &color = QColor())
        : m_format(format)
        , m_transform(matrix)
        , m_color(color)
    {
        Q_ASSERT(m_format != QFontEngine::Format_None);
    }

    virtual ~QFontEngineGlyphCache();

    QFontEngine::GlyphFormat glyphFormat() const { return m_format; }
    const QTransform &transform() const { return m_transform; }
    const QColor &color() const { return m_color; }

    QFontEngine::GlyphFormat m_format;
    QTransform m_transform;
    QColor m_color;
};
typedef QHash<void *, QList<QFontEngineGlyphCache *> > GlyphPointerHash;
typedef QHash<int, QList<QFontEngineGlyphCache *> > GlyphIntHash;

QT_END_NAMESPACE

#endif
