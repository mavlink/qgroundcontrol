// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFONTSUBSET_P_H
#define QFONTSUBSET_P_H

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

#include <QtGui/private/qtguiglobal_p.h>
#include "private/qfontengine_p.h"

QT_BEGIN_NAMESPACE

class QFontSubset
{
public:
    explicit QFontSubset(QFontEngine *fe, uint obj_id = 0)
        : object_id(obj_id), noEmbed(false), fontEngine(fe), downloaded_glyphs(0), standard_font(false)
    {
        fontEngine->ref.ref();
#ifndef QT_NO_PDF
        addGlyph(0);
#endif
    }
    ~QFontSubset() {
        if (!fontEngine->ref.deref())
            delete fontEngine;
    }

    QByteArray toTruetype() const;
#ifndef QT_NO_PDF
    QByteArray widthArray() const;
    QByteArray createToUnicodeMap() const;
    QList<int> getReverseMap() const;

    static QByteArray glyphName(unsigned short unicode, bool symbol);

    qsizetype addGlyph(uint index);
#endif
    const uint object_id;
    bool noEmbed;
    QFontEngine *fontEngine;
    QList<uint> glyph_indices;
    mutable int downloaded_glyphs;
    mutable bool standard_font;
    qsizetype nGlyphs() const { return glyph_indices.size(); }
    mutable QFixed emSquare;
    mutable QList<QFixed> widths;
};

QT_END_NAMESPACE

#endif // QFONTSUBSET_P_H
