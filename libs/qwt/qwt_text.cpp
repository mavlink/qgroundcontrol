/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2003   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

// vim: expandtab

#include <qmap.h>
#include <qfont.h>
#include <qcolor.h>
#include <qpen.h>
#include <qbrush.h>
#include <qpainter.h>
#include "qwt_painter.h"
#include "qwt_text_engine.h"
#include "qwt_text.h"
#if QT_VERSION >= 0x040000
#include <qapplication.h>
#include <qdesktopwidget.h>
#endif

class QwtTextEngineDict
{
public:
    QwtTextEngineDict();
    ~QwtTextEngineDict();

    void setTextEngine(QwtText::TextFormat, QwtTextEngine *);
    const QwtTextEngine *textEngine(QwtText::TextFormat) const;
    const QwtTextEngine *textEngine(const QString &,
                                    QwtText::TextFormat) const;

private:
    typedef QMap<int, QwtTextEngine *> EngineMap;

    inline const QwtTextEngine *engine(EngineMap::const_iterator &it) const {
#if QT_VERSION < 0x040000
        return it.data();
#else
        return it.value();
#endif
    }

    EngineMap d_map;
};

QwtTextEngineDict::QwtTextEngineDict()
{
    d_map.insert(QwtText::PlainText, new QwtPlainTextEngine());
#ifndef QT_NO_RICHTEXT
    d_map.insert(QwtText::RichText, new QwtRichTextEngine());
#endif
}

QwtTextEngineDict::~QwtTextEngineDict()
{
    for ( EngineMap::const_iterator it = d_map.begin();
            it != d_map.end(); ++it ) {
        QwtTextEngine *textEngine = (QwtTextEngine *)engine(it);
        delete textEngine;
    }
}

const QwtTextEngine *QwtTextEngineDict::textEngine(const QString& text,
        QwtText::TextFormat format) const
{
    if ( format == QwtText::AutoText ) {
        for ( EngineMap::const_iterator it = d_map.begin();
                it != d_map.end(); ++it ) {
            if ( it.key() != QwtText::PlainText ) {
                const QwtTextEngine *e = engine(it);
                if ( e && e->mightRender(text) )
                    return (QwtTextEngine *)e;
            }
        }
    }

    EngineMap::const_iterator it = d_map.find(format);
    if ( it != d_map.end() ) {
        const QwtTextEngine *e = engine(it);
        if ( e )
            return e;
    }

    it = d_map.find(QwtText::PlainText);
    return engine(it);
}

void QwtTextEngineDict::setTextEngine(QwtText::TextFormat format,
                                      QwtTextEngine *engine)
{
    if ( format == QwtText::AutoText )
        return;

    if ( format == QwtText::PlainText && engine == NULL )
        return;

    EngineMap::const_iterator it = d_map.find(format);
    if ( it != d_map.end() ) {
        const QwtTextEngine *e = this->engine(it);
        if ( e )
            delete e;

        d_map.remove(format);
    }

    if ( engine != NULL )
        d_map.insert(format, engine);
}

const QwtTextEngine *QwtTextEngineDict::textEngine(
    QwtText::TextFormat format) const
{
    const QwtTextEngine *e = NULL;

    EngineMap::const_iterator it = d_map.find(format);
    if ( it != d_map.end() )
        e = engine(it);

    return e;
}

static QwtTextEngineDict *engineDict = NULL;

class QwtText::PrivateData
{
public:
    PrivateData():
        renderFlags(Qt::AlignCenter),
        backgroundPen(Qt::NoPen),
        backgroundBrush(Qt::NoBrush),
        paintAttributes(0),
        layoutAttributes(0),
        textEngine(NULL) {
    }

    int renderFlags;
    QString text;
    QFont font;
    QColor color;
    QPen backgroundPen;
    QBrush backgroundBrush;

    int paintAttributes;
    int layoutAttributes;

    const QwtTextEngine *textEngine;
};

class QwtText::LayoutCache
{
public:
    void invalidate() {
        textSize = QSize();
    }

    QFont font;
    QSize textSize;
};

/*!
   Constructor

   \param text Text content
   \param textFormat Text format
*/
QwtText::QwtText(const QString &text, QwtText::TextFormat textFormat)
{
    d_data = new PrivateData;
    d_data->text = text;
    d_data->textEngine = textEngine(text, textFormat);

    d_layoutCache = new LayoutCache;
}

//! Copy constructor
QwtText::QwtText(const QwtText &other)
{
    d_data = new PrivateData;
    *d_data = *other.d_data;

    d_layoutCache = new LayoutCache;
    *d_layoutCache = *other.d_layoutCache;
}

//! Destructor
QwtText::~QwtText()
{
    delete d_data;
    delete d_layoutCache;
}

//! Assignement operator
QwtText &QwtText::operator=(const QwtText &other)
{
    *d_data = *other.d_data;
    *d_layoutCache = *other.d_layoutCache;
    return *this;
}

int QwtText::operator==(const QwtText &other) const
{
    return d_data->renderFlags == other.d_data->renderFlags &&
           d_data->text == other.d_data->text &&
           d_data->font == other.d_data->font &&
           d_data->color == other.d_data->color &&
           d_data->backgroundPen == other.d_data->backgroundPen &&
           d_data->backgroundBrush == other.d_data->backgroundBrush &&
           d_data->paintAttributes == other.d_data->paintAttributes &&
           d_data->textEngine == other.d_data->textEngine;
}

int QwtText::operator!=(const QwtText &other) const // invalidate
{
    return !(other == *this);
}

/*!
   Assign a new text content

   \param text Text content
   \param textFormat Text format
*/
void QwtText::setText(const QString &text,
                      QwtText::TextFormat textFormat)
{
    d_data->text = text;
    d_data->textEngine = textEngine(text, textFormat);
    d_layoutCache->invalidate();
}

/*!
   Return the text.
   \sa setText
*/
QString QwtText::text() const
{
    return d_data->text;
}

/*!
   \brief Change the render flags

   The default setting is Qt::AlignCenter

   \param renderFlags Bitwise OR of the flags used like in QPainter::drawText

   \sa renderFlags, QwtTextEngine::draw
   \note Some renderFlags might have no effect, depending on the text format.
*/
void QwtText::setRenderFlags(int renderFlags)
{
    if ( renderFlags != d_data->renderFlags ) {
        d_data->renderFlags = renderFlags;
        d_layoutCache->invalidate();
    }
}

/*!
   \return Render flags
   \sa setRenderFlags
*/
int QwtText::renderFlags() const
{
    return d_data->renderFlags;
}

/*!
   Set the font.

   \param font Font
   \note Setting the font might have no effect, when
         the text contains control sequences for setting fonts.
*/
void QwtText::setFont(const QFont &font)
{
    d_data->font = font;
    setPaintAttribute(PaintUsingTextFont);
}

//! Return the font.
QFont QwtText::font() const
{
    return d_data->font;
}

/*!
  Return the font of the text, if it has one.
  Otherwise return defaultFont.

  \param defaultFont Default font
  \sa setFont, font, PaintAttributes
*/
QFont QwtText::usedFont(const QFont &defaultFont) const
{
    if ( d_data->paintAttributes & PaintUsingTextFont )
        return d_data->font;

    return defaultFont;
}

/*!
   Set the pen color used for painting the text.

   \param color Color
   \note Setting the color might have no effect, when
         the text contains control sequences for setting colors.
*/
void QwtText::setColor(const QColor &color)
{
    d_data->color = color;
    setPaintAttribute(PaintUsingTextColor);
}

//! Return the pen color, used for painting the text
QColor QwtText::color() const
{
    return d_data->color;
}

/*!
  Return the color of the text, if it has one.
  Otherwise return defaultColor.

  \param defaultColor Default color
  \sa setColor, color, PaintAttributes
*/
QColor QwtText::usedColor(const QColor &defaultColor) const
{
    if ( d_data->paintAttributes & PaintUsingTextColor )
        return d_data->color;

    return defaultColor;
}

/*!
   Set the background pen

   \param pen Background pen
   \sa backgroundPen, setBackgroundBrush
*/
void QwtText::setBackgroundPen(const QPen &pen)
{
    d_data->backgroundPen = pen;
    setPaintAttribute(PaintBackground);
}

/*!
   \return Background pen
   \sa setBackgroundPen, backgroundBrush
*/
QPen QwtText::backgroundPen() const
{
    return d_data->backgroundPen;
}

/*!
   Set the background brush

   \param brush Background brush
   \sa backgroundBrush, setBackgroundPen
*/
void QwtText::setBackgroundBrush(const QBrush &brush)
{
    d_data->backgroundBrush = brush;
    setPaintAttribute(PaintBackground);
}

/*!
   \return Background brush
   \sa setBackgroundBrush, backgroundPen
*/
QBrush QwtText::backgroundBrush() const
{
    return d_data->backgroundBrush;
}

/*!
   Change a paint attribute

   \param attribute Paint attribute
   \param on On/Off

   \note Used by setFont, setColor, setBackgroundPen and setBackgroundBrush
   \sa testPaintAttribute
*/
void QwtText::setPaintAttribute(PaintAttribute attribute, bool on)
{
    if ( on )
        d_data->paintAttributes |= attribute;
    else
        d_data->paintAttributes &= ~attribute;
}

/*!
   Test a paint attribute

   \param attribute Paint attribute
   \return true, if attribute is enabled

   \sa setPaintAttribute
*/
bool QwtText::testPaintAttribute(PaintAttribute attribute) const
{
    return d_data->paintAttributes & attribute;
}

/*!
   Change a layout attribute

   \param attribute Layout attribute
   \param on On/Off
   \sa testLayoutAttribute
*/
void QwtText::setLayoutAttribute(LayoutAttribute attribute, bool on)
{
    if ( on )
        d_data->layoutAttributes |= attribute;
    else
        d_data->layoutAttributes &= ~attribute;
}

/*!
   Test a layout attribute

   \param attribute Layout attribute
   \return true, if attribute is enabled

   \sa setLayoutAttribute
*/
bool QwtText::testLayoutAttribute(LayoutAttribute attribute) const
{
    return d_data->layoutAttributes | attribute;
}

/*!
   Find the height for a given width

   \param defaultFont Font, used for the calculation if the text has no font
   \param width Width

   \return Calculated height
*/
int QwtText::heightForWidth(int width, const QFont &defaultFont) const
{
    const QwtMetricsMap map = QwtPainter::metricsMap();
    width = map.layoutToScreenX(width);

#if QT_VERSION < 0x040000
    const QFont font = usedFont(defaultFont);
#else
    // We want to calculate in screen metrics. So
    // we need a font that uses screen metrics

    const QFont font(usedFont(defaultFont), QApplication::desktop());
#endif

    int h = 0;

    if ( d_data->layoutAttributes & MinimumLayout ) {
        int left, right, top, bottom;
        d_data->textEngine->textMargins(font, d_data->text,
                                        left, right, top, bottom);

        h = d_data->textEngine->heightForWidth(
                font, d_data->renderFlags, d_data->text,
                width + left + right);

        h -= top + bottom;
    } else {
        h = d_data->textEngine->heightForWidth(
                font, d_data->renderFlags, d_data->text, width);
    }

    h = map.screenToLayoutY(h);
    return h;
}

/*!
   Find the height for a given width

   \param defaultFont Font, used for the calculation if the text has no font

   \return Calculated height
*/

/*!
   Returns the size, that is needed to render text

   \param defaultFont Font of the text
   \return Caluclated size
*/
QSize QwtText::textSize(const QFont &defaultFont) const
{
#if QT_VERSION < 0x040000
    const QFont font(usedFont(defaultFont));
#else
    // We want to calculate in screen metrics. So
    // we need a font that uses screen metrics

    const QFont font(usedFont(defaultFont), QApplication::desktop());
#endif

    if ( !d_layoutCache->textSize.isValid()
            || d_layoutCache->font != font ) {
        d_layoutCache->textSize = d_data->textEngine->textSize(
                                      font, d_data->renderFlags, d_data->text);
        d_layoutCache->font = font;
    }

    QSize sz = d_layoutCache->textSize;

    const QwtMetricsMap map = QwtPainter::metricsMap();

    if ( d_data->layoutAttributes & MinimumLayout ) {
        int left, right, top, bottom;
        d_data->textEngine->textMargins(font, d_data->text,
                                        left, right, top, bottom);
        sz -= QSize(left + right, top + bottom);
#if QT_VERSION >= 0x040000
        if ( !map.isIdentity() ) {
#ifdef __GNUC__
#endif
            /*
                When printing in high resolution, the tick labels
                of are cut of. We need to find out why, but for
                the moment we add a couple of pixels instead.
             */
            sz += QSize(3, 0);
        }
#endif
    }

    sz = map.screenToLayout(sz);
    return sz;
}

/*!
   Draw a text into a rectangle

   \param painter Painter
   \param rect Rectangle
*/
void QwtText::draw(QPainter *painter, const QRect &rect) const
{
    if ( d_data->paintAttributes & PaintBackground ) {
        if ( d_data->backgroundPen != Qt::NoPen ||
                d_data->backgroundBrush != Qt::NoBrush ) {
            painter->save();
            painter->setPen(d_data->backgroundPen);
            painter->setBrush(d_data->backgroundBrush);
#if QT_VERSION < 0x040000
            QwtPainter::drawRect(painter, rect);
#else
            const QRect r(rect.x(), rect.y(),
                          rect.width() - 1, rect.height() - 1);
            QwtPainter::drawRect(painter, r);
#endif
            painter->restore();
        }
    }

    painter->save();

    if ( d_data->paintAttributes & PaintUsingTextFont ) {
        painter->setFont(d_data->font);
    }

    if ( d_data->paintAttributes & PaintUsingTextColor ) {
        if ( d_data->color.isValid() )
            painter->setPen(d_data->color);
    }

    QRect expandedRect = rect;
    if ( d_data->layoutAttributes & MinimumLayout ) {
#if QT_VERSION < 0x040000
        const QFont font(painter->font());
#else
        // We want to calculate in screen metrics. So
        // we need a font that uses screen metrics

        const QFont font(painter->font(), QApplication::desktop());
#endif

        int left, right, top, bottom;
        d_data->textEngine->textMargins(
            font, d_data->text,
            left, right, top, bottom);

        const QwtMetricsMap map = QwtPainter::metricsMap();
        left = map.screenToLayoutX(left);
        right = map.screenToLayoutX(right);
        top = map.screenToLayoutY(top);
        bottom = map.screenToLayoutY(bottom);

        expandedRect.setTop(rect.top() - top);
        expandedRect.setBottom(rect.bottom() + bottom);
        expandedRect.setLeft(rect.left() - left);
        expandedRect.setRight(rect.right() + right);
    }

    d_data->textEngine->draw(painter, expandedRect,
                             d_data->renderFlags, d_data->text);

    painter->restore();
}

/*!
   Find the text engine for a text format

   In case of QwtText::AutoText the first text engine
   (beside QwtPlainTextEngine) is returned, where QwtTextEngine::mightRender
   returns true. If there is none QwtPlainTextEngine is returnd.

   If no text engine is registered for the format QwtPlainTextEngine
   is returnd.

   \param text Text, needed in case of AutoText
   \param format Text format
*/
const QwtTextEngine *QwtText::textEngine(const QString &text,
        QwtText::TextFormat format)
{
    if ( engineDict == NULL ) {
        /*
          Note: engineDict is allocated, the first time it is used,
                but never deleted, because there is no known last access time.
                So don't be irritated, if it is reported as a memory leak
                from your memory profiler.
         */
        engineDict = new QwtTextEngineDict();
    }

    return engineDict->textEngine(text, format);
}

/*!
   Assign/Replace a text engine for a text format

   With setTextEngine it is possible to extend Qwt with
   other types of text formats.

   Owner of a commercial Qt license can build the qwtmathml library,
   that is based on the MathML renderer, that is included in MML Widget
   component of the Qt solutions package.

   For QwtText::PlainText it is not allowed to assign a engine == NULL.

   \param format Text format
   \param engine Text engine

   \sa QwtMathMLTextEngine
   \warning Using QwtText::AutoText does nothing.
*/
void QwtText::setTextEngine(QwtText::TextFormat format,
                            QwtTextEngine *engine)
{
    if ( engineDict == NULL )
        engineDict = new QwtTextEngineDict();

    engineDict->setTextEngine(format, engine);
}

/*!
   \brief Find the text engine for a text format

   textEngine can be used to find out if a text format is supported.
   F.e, if one wants to use MathML labels, the MathML renderer from the
   commercial Qt solutions package might be required, that is not
   available in Qt Open Source Edition environments.

   \param format Text format
   \return The text engine, or NULL if no engine is available.
*/
const QwtTextEngine *QwtText::textEngine(QwtText::TextFormat format)
{
    if ( engineDict == NULL )
        engineDict = new QwtTextEngineDict();

    return engineDict->textEngine(format);
}
