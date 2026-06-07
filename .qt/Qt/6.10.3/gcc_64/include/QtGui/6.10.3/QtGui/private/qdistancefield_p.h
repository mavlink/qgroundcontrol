// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QDISTANCEFIELD_H
#define QDISTANCEFIELD_H

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
#include <qrawfont.h>
#include <private/qfontengine_p.h>
#include <QtCore/qshareddata.h>
#include <QtCore/qglobal.h>
#include <QLoggingCategory>

QT_BEGIN_NAMESPACE

bool Q_GUI_EXPORT qt_fontHasNarrowOutlines(const QRawFont &f);
bool Q_GUI_EXPORT qt_fontHasNarrowOutlines(QFontEngine *fontEngine);

int Q_GUI_EXPORT QT_DISTANCEFIELD_BASEFONTSIZE(bool narrowOutlineFont);
int Q_GUI_EXPORT QT_DISTANCEFIELD_TILESIZE(bool narrowOutlineFont);
int Q_GUI_EXPORT QT_DISTANCEFIELD_SCALE(bool narrowOutlineFont);
int Q_GUI_EXPORT QT_DISTANCEFIELD_RADIUS(bool narrowOutlineFont);
int Q_GUI_EXPORT QT_DISTANCEFIELD_HIGHGLYPHCOUNT();

class Q_GUI_EXPORT QDistanceFieldData : public QSharedData
{
public:
    QDistanceFieldData() : glyph(0), width(0), height(0), nbytes(0), data(nullptr) {}
    QDistanceFieldData(const QDistanceFieldData &other);
    ~QDistanceFieldData();

    static QDistanceFieldData *create(const QSize &size);
    static QDistanceFieldData *create(const QPainterPath &path, bool doubleResolution);
    static QDistanceFieldData *create(QSize size, const QPainterPath &path, bool doubleResolution);

    glyph_t glyph;
    int width;
    int height;
    int nbytes;
    uchar *data;
};

class Q_GUI_EXPORT QDistanceField
{
public:
    QDistanceField();
    QDistanceField(int width, int height);
    QDistanceField(const QRawFont &font, glyph_t glyph, bool doubleResolution = false);
    QDistanceField(QFontEngine *fontEngine, glyph_t glyph, bool doubleResolution = false);
    QDistanceField(const QPainterPath &path, glyph_t glyph, bool doubleResolution = false);
    QDistanceField(QSize size, const QPainterPath &path, glyph_t glyph, bool doubleResolution = false);

    bool isNull() const;

    glyph_t glyph() const;
    void setGlyph(const QRawFont &font, glyph_t glyph, bool doubleResolution = false);
    void setGlyph(QFontEngine *fontEngine, glyph_t glyph, bool doubleResolution = false);

    int width() const;
    int height() const;

    QDistanceField copy(const QRect &rect = QRect()) const;
    inline QDistanceField copy(int x, int y, int w, int h) const
        { return copy(QRect(x, y, w, h)); }

    uchar *bits();
    const uchar *bits() const;
    const uchar *constBits() const;

    uchar *scanLine(int);
    const uchar *scanLine(int) const;
    const uchar *constScanLine(int) const;

    QImage toImage(QImage::Format format = QImage::Format_ARGB32_Premultiplied) const;

private:
    QDistanceField(QDistanceFieldData *data);
    QSharedDataPointer<QDistanceFieldData> d;

    friend class QDistanceFieldData;
};

QT_END_NAMESPACE

#endif // QDISTANCEFIELD_H
