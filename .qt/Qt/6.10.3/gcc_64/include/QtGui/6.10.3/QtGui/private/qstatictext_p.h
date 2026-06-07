// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSTATICTEXT_P_H
#define QSTATICTEXT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of internal files.  This header file may change from version to version
// without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qtguiglobal_p.h>
#include "qstatictext.h"

#include <private/qtextureglyphcache_p.h>
#include <QtGui/qcolor.h>

QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT QStaticTextUserData
{
public:
    enum Type {
        NoUserData,
        OpenGLUserData
    };

    QStaticTextUserData(Type t) : ref(0), type(t) {}
    virtual ~QStaticTextUserData();

    QAtomicInt ref;
    Type type;
};

class Q_GUI_EXPORT QStaticTextItem
{
public:
    QStaticTextItem() : useBackendOptimizations(false),
                        userDataNeedsUpdate(0), usesRawFont(0),
                        m_fontEngine(nullptr), m_userData(nullptr) {}

    void setUserData(QStaticTextUserData *newUserData)
    {
        m_userData = newUserData;
    }
    QStaticTextUserData *userData() const { return m_userData.data(); }

    void setFontEngine(QFontEngine *fe)
    {
        m_fontEngine = fe;
    }

    QFontEngine *fontEngine() const { return m_fontEngine.data(); }

    union {
        QFixedPoint *glyphPositions;             // 8 bytes per glyph
        int positionOffset;
    };
    union {
        glyph_t *glyphs;                         // 4 bytes per glyph
        int glyphOffset;
    };
                                                 // =================
                                                 // 12 bytes per glyph

                                                 // 8 bytes for pointers
    int numGlyphs;                               // 4 bytes per item
    QFont font;                                  // 8 bytes per item
    QColor color;                                // 10 bytes per item
    char useBackendOptimizations : 1;            // 1 byte per item
    char userDataNeedsUpdate : 1;                //
    char usesRawFont : 1;                        //

private: // private to avoid abuse
    QExplicitlySharedDataPointer<QFontEngine> m_fontEngine;       // 4 bytes per item
    QExplicitlySharedDataPointer<QStaticTextUserData> m_userData; // 8 bytes per item
                                                                  // ================
                                                                  // 43 bytes per item
};
Q_DECLARE_TYPEINFO(QStaticTextItem, Q_RELOCATABLE_TYPE);

class QStaticText;
class Q_AUTOTEST_EXPORT QStaticTextPrivate
{
public:
    QStaticTextPrivate();
    QStaticTextPrivate(const QStaticTextPrivate &other);
    ~QStaticTextPrivate();

    void init();
    void paintText(const QPointF &pos, QPainter *p, const QColor &pen);

    void invalidate()
    {
        needsRelayout = true;
    }

    QAtomicInt ref;                      // 4 bytes per text

    QString text;                        // 4 bytes per text
    QFont font;                          // 8 bytes per text
    qreal textWidth;                     // 8 bytes per text
    QSizeF actualSize;                   // 16 bytes per text
    QPointF position;                    // 16 bytes per text

    QTransform matrix;                   // 80 bytes per text
    QStaticTextItem *items;              // 4 bytes per text
    int itemCount;                       // 4 bytes per text

    glyph_t *glyphPool;                  // 4 bytes per text
    QFixedPoint *positionPool;           // 4 bytes per text

    QTextOption textOption;              // 28 bytes per text

    unsigned char needsRelayout            : 1; // 1 byte per text
    unsigned char useBackendOptimizations  : 1;
    unsigned char textFormat               : 2;
    unsigned char untransformedCoordinates : 1;
                                         // ================
                                         // 191 bytes per text

    static QStaticTextPrivate *get(const QStaticText *q);
};

QT_END_NAMESPACE

#endif // QSTATICTEXT_P_H
