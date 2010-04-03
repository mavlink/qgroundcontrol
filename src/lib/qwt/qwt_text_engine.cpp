/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2003   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

// vim: expandtab

#include <qpainter.h>
#include <qpixmap.h>
#include <qimage.h>
#include <qmap.h>
#include <qwidget.h>
#include "qwt_math.h"
#include "qwt_painter.h"
#include "qwt_text_engine.h"

static QString taggedRichText(const QString &text, int flags)
{
    QString richText = text;

    // By default QSimpleRichText is Qt::AlignLeft
    if (flags & Qt::AlignJustify)
    {
        richText.prepend(QString::fromLatin1("<div align=\"justify\">"));
        richText.append(QString::fromLatin1("</div>"));
    }
    else if (flags & Qt::AlignRight)
    {
        richText.prepend(QString::fromLatin1("<div align=\"right\">"));
        richText.append(QString::fromLatin1("</div>"));
    }
    else if (flags & Qt::AlignHCenter)
    {
        richText.prepend(QString::fromLatin1("<div align=\"center\">"));
        richText.append(QString::fromLatin1("</div>"));
    }

    return richText;
}

#if QT_VERSION < 0x040000

#include <qsimplerichtext.h>
#include <qstylesheet.h>

class QwtRichTextDocument: public QSimpleRichText
{
public:
    QwtRichTextDocument(const QString &text, int flags, const QFont &font):
        QSimpleRichText(taggedRichText(text, flags), font)
    {
    }
};

#else // QT_VERSION >= 0x040000

#include <qtextobject.h>
#include <qtextdocument.h>
#include <qabstracttextdocumentlayout.h>

#if QT_VERSION < 0x040200
#include <qlabel.h>
#endif

class QwtRichTextDocument: public QTextDocument
{
public:
    QwtRichTextDocument(const QString &text, int flags, const QFont &font)
    {
        setUndoRedoEnabled(false);
        setDefaultFont(font);
#if QT_VERSION >= 0x040300
        setHtml(text);
#else
        setHtml(taggedRichText(text, flags));
#endif

        // make sure we have a document layout
        (void)documentLayout();

#if QT_VERSION >= 0x040300
        QTextOption option = defaultTextOption();
        if ( flags & Qt::TextWordWrap )
            option.setWrapMode(QTextOption::WordWrap);
        else
            option.setWrapMode(QTextOption::NoWrap);

        option.setAlignment((Qt::Alignment) flags);
        setDefaultTextOption(option);

        QTextFrame *root = rootFrame();
        QTextFrameFormat fm = root->frameFormat();
        fm.setBorder(0);
        fm.setMargin(0);
        fm.setPadding(0);
        fm.setBottomMargin(0);
        fm.setLeftMargin(0);
        root->setFrameFormat(fm);

        adjustSize();
#endif
    }
};

#endif

class QwtPlainTextEngine::PrivateData
{
public:
    int effectiveAscent(const QFont &font) const
    {
        const QString fontKey = font.key();

        QMap<QString, int>::const_iterator it = 
            d_ascentCache.find(fontKey);
        if ( it == d_ascentCache.end() )
        {
            int ascent = findAscent(font);
            it = d_ascentCache.insert(fontKey, ascent);
        }

        return (*it);
    }

private:
    int findAscent(const QFont &font) const
    {
        static const QString dummy("E");
        static const QColor white(Qt::white);

        const QFontMetrics fm(font);
        QPixmap pm(fm.width(dummy), fm.height()); 
        pm.fill(white);

        QPainter p(&pm);
        p.setFont(font);  
        p.drawText(0, 0,  pm.width(), pm.height(), 0, dummy);
        p.end();

#if QT_VERSION < 0x040000
        const QImage img = pm.convertToImage();
#else
        const QImage img = pm.toImage();
#endif

        int row = 0;
        for ( row = 0; row < img.height(); row++ )
        {   
            const QRgb *line = (const QRgb *)img.scanLine(row);

            const int w = pm.width();
            for ( int col = 0; col < w; col++ )
            {   
                if ( line[col] != white.rgb() )
                    return fm.ascent() - row + 1;
            }
        }

        return fm.ascent();
    }   

    mutable QMap<QString, int> d_ascentCache;
};

//! Constructor
QwtTextEngine::QwtTextEngine()
{
}

//! Destructor
QwtTextEngine::~QwtTextEngine()
{
}

//! Constructor
QwtPlainTextEngine::QwtPlainTextEngine()
{
    d_data = new PrivateData;
}

//! Destructor
QwtPlainTextEngine::~QwtPlainTextEngine()
{
    delete d_data;
}

/*!
   Find the height for a given width

   \param font Font of the text
   \param flags Bitwise OR of the flags used like in QPainter::drawText
   \param text Text to be rendered
   \param width Width  

   \return Calculated height
*/
int QwtPlainTextEngine::heightForWidth(const QFont& font, int flags,
        const QString& text, int width) const
{
    const QFontMetrics fm(font);
    const QRect rect = fm.boundingRect(
        0, 0, width, QWIDGETSIZE_MAX, flags, text);

    return rect.height();
}

/*!
  Returns the size, that is needed to render text

  \param font Font of the text
  \param flags Bitwise OR of the flags used like in QPainter::drawText
  \param text Text to be rendered

  \return Caluclated size
*/
QSize QwtPlainTextEngine::textSize(const QFont &font,
    int flags, const QString& text) const
{
    const QFontMetrics fm(font);
    const QRect rect = fm.boundingRect(
        0, 0, QWIDGETSIZE_MAX, QWIDGETSIZE_MAX, flags, text);

    return rect.size();
}

/*!
  Return margins around the texts

  \param font Font of the text
  \param left Return 0
  \param right Return 0
  \param top Return value for the top margin
  \param bottom Return value for the bottom margin
*/
void QwtPlainTextEngine::textMargins(const QFont &font, const QString &,
    int &left, int &right, int &top, int &bottom) const
{
    left = right = top = 0;

    const QFontMetrics fm(font);
    top = fm.ascent() - d_data->effectiveAscent(font);
    bottom = fm.descent() + 1;
}

/*!
  \brief Draw the text in a clipping rectangle
      
  A wrapper for QPainter::drawText.

  \param painter Painter
  \param rect Clipping rectangle
  \param flags Bitwise OR of the flags used like in QPainter::drawText
  \param text Text to be rendered
*/
void QwtPlainTextEngine::draw(QPainter *painter, const QRect &rect,
    int flags, const QString& text) const
{
    QwtPainter::drawText(painter, rect, flags, text);
}

/*! 
  Test if a string can be rendered by this text engine.
  \return Always true. All texts can be rendered by QwtPlainTextEngine
*/
bool QwtPlainTextEngine::mightRender(const QString &) const
{
    return true;
}

#ifndef QT_NO_RICHTEXT

//! Constructor
QwtRichTextEngine::QwtRichTextEngine()
{
}

/*!
   Find the height for a given width
  
   \param font Font of the text
   \param flags Bitwise OR of the flags used like in QPainter::drawText
   \param text Text to be rendered
   \param width Width  

   \return Calculated height
*/
int QwtRichTextEngine::heightForWidth(const QFont& font, int flags,
        const QString& text, int width) const
{
    QwtRichTextDocument doc(text, flags, font);

#if QT_VERSION < 0x040000
    doc.setWidth(width);
    const int h = doc.height();
#else
    doc.setPageSize(QSize(width, QWIDGETSIZE_MAX));
    const int h = qRound(doc.documentLayout()->documentSize().height());
#endif
    return h;
}

/*!
  Returns the size, that is needed to render text
  
  \param font Font of the text
  \param flags Bitwise OR of the flags used like in QPainter::drawText
  \param text Text to be rendered

  \return Caluclated size
*/

QSize QwtRichTextEngine::textSize(const QFont &font,
    int flags, const QString& text) const
{
    QwtRichTextDocument doc(text, flags, font);

#if QT_VERSION < 0x040000
    doc.setWidth(QWIDGETSIZE_MAX);

    const int w = doc.widthUsed();
    const int h = doc.height();
    return QSize(w, h);

#else // QT_VERSION >= 0x040000

#if QT_VERSION < 0x040200
    /*
      Unfortunately offering the bounding rect calculation in the
      API of QTextDocument has been forgotten in Qt <= 4.1.x. It
      is planned to come with Qt 4.2.x.
      In the meantime we need a hack with a temporary QLabel,
      to reengineer the internal calculations.
    */

    static int off = 0;
    static QLabel *label = NULL;
    if ( label == NULL )
    {
        label = new QLabel;
        label->hide();

        const char *s = "XXXXX";
        label->setText(s);
        int w1 = label->sizeHint().width();
        const QFontMetrics fm(label->font());
        int w2 = fm.width(s);
        off = w1 - w2;
    }
    label->setFont(doc.defaultFont());
    label->setText(text);

    int w = qwtMax(label->sizeHint().width() - off, 0);
    doc.setPageSize(QSize(w, QWIDGETSIZE_MAX));

    int h = qRound(doc.documentLayout()->documentSize().height());
    return QSize(w, h);

#else // QT_VERSION >= 0x040200

#if QT_VERSION >= 0x040300
    QTextOption option = doc.defaultTextOption();
    if ( option.wrapMode() != QTextOption::NoWrap )
    {
        option.setWrapMode(QTextOption::NoWrap);
        doc.setDefaultTextOption(option);
        doc.adjustSize();
    }
#endif

    return doc.size().toSize();
#endif
#endif
}

/*!
  Draw the text in a clipping rectangle

  \param painter Painter
  \param rect Clipping rectangle
  \param flags Bitwise OR of the flags like in for QPainter::drawText
  \param text Text to be rendered
*/
void QwtRichTextEngine::draw(QPainter *painter, const QRect &rect,
    int flags, const QString& text) const
{
    QwtRichTextDocument doc(text, flags, painter->font());
    QwtPainter::drawSimpleRichText(painter, rect, flags, doc);
}

/*! 
   Wrap text into <div align=...> </div> tags according flags

   \param text Text
   \param flags Bitwise OR of the flags like in for QPainter::drawText

   \return Tagged text
*/
QString QwtRichTextEngine::taggedText(const QString &text, int flags) const
{
    return taggedRichText(text, flags);
}

/*!
  Test if a string can be rendered by this text engine

  \param text Text to be tested
  \return QStyleSheet::mightBeRichText(text);
*/
bool QwtRichTextEngine::mightRender(const QString &text) const
{
#if QT_VERSION < 0x040000
    return QStyleSheet::mightBeRichText(text);
#else
    return Qt::mightBeRichText(text);
#endif
}

/*!
  Return margins around the texts

  \param left Return 0
  \param right Return 0
  \param top Return 0
  \param bottom Return 0
*/
void QwtRichTextEngine::textMargins(const QFont &, const QString &,
    int &left, int &right, int &top, int &bottom) const
{
    left = right = top = bottom = 0;
}

#endif // !QT_NO_RICHTEXT
