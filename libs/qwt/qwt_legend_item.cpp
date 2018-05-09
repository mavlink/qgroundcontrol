/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

// vim: expandtab

#include <qpainter.h>
#include <qdrawutil.h>
#include <qstyle.h>
#include <qpen.h>
#if QT_VERSION >= 0x040000
#include <qevent.h>
#include <qstyleoption.h>
#endif
#include "qwt_math.h"
#include "qwt_painter.h"
#include "qwt_symbol.h"
#include "qwt_legend_item.h"

static const int ButtonFrame = 2;
static const int Margin = 2;

static QSize buttonShift(const QwtLegendItem *w)
{
#if QT_VERSION < 0x040000
    const int ph = w->style().pixelMetric(
                       QStyle::PM_ButtonShiftHorizontal, w);
    const int pv = w->style().pixelMetric(
                       QStyle::PM_ButtonShiftVertical, w);
#else
    QStyleOption option;
    option.init(w);

    const int ph = w->style()->pixelMetric(
                       QStyle::PM_ButtonShiftHorizontal, &option, w);
    const int pv = w->style()->pixelMetric(
                       QStyle::PM_ButtonShiftVertical, &option, w);
#endif
    return QSize(ph, pv);
}

class QwtLegendItem::PrivateData
{
public:
    PrivateData():
        itemMode(QwtLegend::ReadOnlyItem),
        isDown(false),
        identifierWidth(8),
        identifierMode(QwtLegendItem::ShowLine | QwtLegendItem::ShowText),
        curvePen(Qt::NoPen),
        spacing(Margin) {
        symbol = new QwtSymbol();
    }

    ~PrivateData() {
        delete symbol;
    }

    QwtLegend::LegendItemMode itemMode;
    bool isDown;

    int identifierWidth;
    int identifierMode;
    QwtSymbol *symbol;
    QPen curvePen;

    int spacing;
};

/*!
  \param parent Parent widget
*/
QwtLegendItem::QwtLegendItem(QWidget *parent):
    QwtTextLabel(parent)
{
    d_data = new PrivateData;
    init(QwtText());
}

/*!
  \param symbol Curve symbol
  \param curvePen Curve pen
  \param text Label text
  \param parent Parent widget
*/
QwtLegendItem::QwtLegendItem(const QwtSymbol &symbol,
                             const QPen &curvePen, const QwtText &text,
                             QWidget *parent):
    QwtTextLabel(parent)
{
    d_data = new PrivateData;

    delete d_data->symbol;
    d_data->symbol = symbol.clone();

    d_data->curvePen = curvePen;

    init(text);
}

void QwtLegendItem::init(const QwtText &text)
{
    setMargin(Margin);
    setIndent(margin() + d_data->identifierWidth + 2 * d_data->spacing);
    setText(text);
}

//! Destructor
QwtLegendItem::~QwtLegendItem()
{
    delete d_data;
    d_data = NULL;
}

/*!
   Set the text to the legend item

   \param text Text label
    \sa QwtTextLabel::text()
*/
void QwtLegendItem::setText(const QwtText &text)
{
    const int flags = Qt::AlignLeft | Qt::AlignVCenter
#if QT_VERSION < 0x040000
                      | Qt::WordBreak | Qt::ExpandTabs;
#else
                      | Qt::TextExpandTabs | Qt::TextWordWrap;
#endif

    QwtText txt = text;
    txt.setRenderFlags(flags);

    QwtTextLabel::setText(txt);
}

/*!
   Set the item mode
   The default is QwtLegend::ReadOnlyItem

   \param mode Item mode
   \sa itemMode()
*/
void QwtLegendItem::setItemMode(QwtLegend::LegendItemMode mode)
{
    d_data->itemMode = mode;
    d_data->isDown = false;

#if QT_VERSION >= 0x040000
    using namespace Qt;
#endif
    setFocusPolicy(mode != QwtLegend::ReadOnlyItem ? TabFocus : NoFocus);
    setMargin(ButtonFrame + Margin);

    updateGeometry();
}

/*!
   Return the item mode

   \sa setItemMode()
*/
QwtLegend::LegendItemMode QwtLegendItem::itemMode() const
{
    return d_data->itemMode;
}

/*!
  Set identifier mode.
  Default is ShowLine | ShowText.
  \param mode Or'd values of IdentifierMode

  \sa identifierMode()
*/
void QwtLegendItem::setIdentifierMode(int mode)
{
    if ( mode != d_data->identifierMode ) {
        d_data->identifierMode = mode;
        update();
    }
}

/*!
  Or'd values of IdentifierMode.
  \sa setIdentifierMode(), IdentifierMode
*/
int QwtLegendItem::identifierMode() const
{
    return d_data->identifierMode;
}

/*!
  Set the width for the identifier
  Default is 8 pixels

  \param width New width

  \sa identifierMode(), identifierWidth
*/
void QwtLegendItem::setIdentfierWidth(int width)
{
    width = qwtMax(width, 0);
    if ( width != d_data->identifierWidth ) {
        d_data->identifierWidth = width;
        setIndent(margin() + d_data->identifierWidth
                  + 2 * d_data->spacing);
    }
}
/*!
   Return the width of the identifier

   \sa setIdentfierWidth
*/
int QwtLegendItem::identifierWidth() const
{
    return d_data->identifierWidth;
}

/*!
   Change the spacing
   \param spacing Spacing
   \sa spacing(), identifierWidth(), QwtTextLabel::margin()
*/
void QwtLegendItem::setSpacing(int spacing)
{
    spacing = qwtMax(spacing, 0);
    if ( spacing != d_data->spacing ) {
        d_data->spacing = spacing;
        setIndent(margin() + d_data->identifierWidth
                  + 2 * d_data->spacing);
    }
}

/*!
   Return the spacing
   \sa setSpacing(), identifierWidth(), QwtTextLabel::margin()
*/
int QwtLegendItem::spacing() const
{
    return d_data->spacing;
}

/*!
  Set curve symbol.
  \param symbol Symbol

  \sa symbol()
*/
void QwtLegendItem::setSymbol(const QwtSymbol &symbol)
{
    delete d_data->symbol;
    d_data->symbol = symbol.clone();
    update();
}

/*!
  \return The curve symbol.
  \sa setSymbol()
*/
const QwtSymbol& QwtLegendItem::symbol() const
{
    return *d_data->symbol;
}


/*!
  Set curve pen.
  \param pen Curve pen

  \sa curvePen()
*/
void QwtLegendItem::setCurvePen(const QPen &pen)
{
    if ( pen != d_data->curvePen ) {
        d_data->curvePen = pen;
        update();
    }
}

/*!
  \return The curve pen.
  \sa setCurvePen()
*/
const QPen& QwtLegendItem::curvePen() const
{
    return d_data->curvePen;
}

/*!
  Paint the identifier to a given rect.
  \param painter Painter
  \param rect Rect where to paint
*/
void QwtLegendItem::drawIdentifier(
    QPainter *painter, const QRect &rect) const
{
    if ( rect.isEmpty() )
        return;

    if ( (d_data->identifierMode & ShowLine ) && (d_data->curvePen.style() != Qt::NoPen) ) {
        painter->save();
        painter->setPen(d_data->curvePen);
        QwtPainter::drawLine(painter, rect.left(), rect.center().y(),
                             rect.right(), rect.center().y());
        painter->restore();
    }

    if ( (d_data->identifierMode & ShowSymbol)
            && (d_data->symbol->style() != QwtSymbol::NoSymbol) ) {
        QSize symbolSize =
            QwtPainter::metricsMap().screenToLayout(d_data->symbol->size());

        // scale the symbol size down if it doesn't fit into rect.

        if ( rect.width() < symbolSize.width() ) {
            const double ratio =
                double(symbolSize.width()) / double(rect.width());
            symbolSize.setWidth(rect.width());
            symbolSize.setHeight(qRound(symbolSize.height() / ratio));
        }
        if ( rect.height() < symbolSize.height() ) {
            const double ratio =
                double(symbolSize.width()) / double(rect.width());
            symbolSize.setHeight(rect.height());
            symbolSize.setWidth(qRound(symbolSize.width() / ratio));
        }

        QRect symbolRect;
        symbolRect.setSize(symbolSize);
        symbolRect.moveCenter(rect.center());

        painter->save();
        painter->setBrush(d_data->symbol->brush());
        painter->setPen(d_data->symbol->pen());
        d_data->symbol->draw(painter, symbolRect);
        painter->restore();
    }
}

/*!
  Draw the legend item to a given rect.
  \param painter Painter
  \param rect Rect where to paint the button
*/

void QwtLegendItem::drawItem(QPainter *painter, const QRect &rect) const
{
    painter->save();

    const QwtMetricsMap &map = QwtPainter::metricsMap();

    const int m = map.screenToLayoutX(margin());
    const int spacing = map.screenToLayoutX(d_data->spacing);
    const int identifierWidth = map.screenToLayoutX(d_data->identifierWidth);

    const QRect identifierRect(rect.x() + m, rect.y(),
                               identifierWidth, rect.height());
    drawIdentifier(painter, identifierRect);

    // Label

    QRect titleRect = rect;
    titleRect.setX(identifierRect.right() + 2 * spacing);

    text().draw(painter, titleRect);

    painter->restore();
}

//! Paint event
void QwtLegendItem::paintEvent(QPaintEvent *e)
{
    const QRect cr = contentsRect();

    QPainter painter(this);
    painter.setClipRegion(e->region());

    if ( d_data->isDown ) {
        qDrawWinButton(&painter, 0, 0, width(), height(),
#if QT_VERSION < 0x040000
                       colorGroup(),
#else
                       palette(),
#endif
                       true);
    }

    painter.save();

    if ( d_data->isDown ) {
        const QSize shiftSize = buttonShift(this);
        painter.translate(shiftSize.width(), shiftSize.height());
    }

    painter.setClipRect(cr);

    drawContents(&painter);

    QRect rect = cr;
    rect.setX(rect.x() + margin());
    if ( d_data->itemMode != QwtLegend::ReadOnlyItem )
        rect.setX(rect.x() + ButtonFrame);

    rect.setWidth(d_data->identifierWidth);

    drawIdentifier(&painter, rect);

    painter.restore();
}

//! Handle mouse press events
void QwtLegendItem::mousePressEvent(QMouseEvent *e)
{
    if ( e->button() == Qt::LeftButton ) {
        switch(d_data->itemMode) {
        case QwtLegend::ClickableItem: {
            setDown(true);
            return;
        }
        case QwtLegend::CheckableItem: {
            setDown(!isDown());
            return;
        }
        default:
            ;
        }
    }
    QwtTextLabel::mousePressEvent(e);
}

//! Handle mouse release events
void QwtLegendItem::mouseReleaseEvent(QMouseEvent *e)
{
    if ( e->button() == Qt::LeftButton ) {
        switch(d_data->itemMode) {
        case QwtLegend::ClickableItem: {
            setDown(false);
            return;
        }
        case QwtLegend::CheckableItem: {
            return; // do nothing, but accept
        }
        default:
            ;
        }
    }
    QwtTextLabel::mouseReleaseEvent(e);
}

//! Handle key press events
void QwtLegendItem::keyPressEvent(QKeyEvent *e)
{
    if ( e->key() == Qt::Key_Space ) {
        switch(d_data->itemMode) {
        case QwtLegend::ClickableItem: {
            if ( !e->isAutoRepeat() )
                setDown(true);
            return;
        }
        case QwtLegend::CheckableItem: {
            if ( !e->isAutoRepeat() )
                setDown(!isDown());
            return;
        }
        default:
            ;
        }
    }

    QwtTextLabel::keyPressEvent(e);
}

//! Handle key release events
void QwtLegendItem::keyReleaseEvent(QKeyEvent *e)
{
    if ( e->key() == Qt::Key_Space ) {
        switch(d_data->itemMode) {
        case QwtLegend::ClickableItem: {
            if ( !e->isAutoRepeat() )
                setDown(false);
            return;
        }
        case QwtLegend::CheckableItem: {
            return; // do nothing, but accept
        }
        default:
            ;
        }
    }

    QwtTextLabel::keyReleaseEvent(e);
}

/*!
    Check/Uncheck a the item

    \param on check/uncheck
    \sa setItemMode()
*/
void QwtLegendItem::setChecked(bool on)
{
    if ( d_data->itemMode == QwtLegend::CheckableItem ) {
        const bool isBlocked = signalsBlocked();
        blockSignals(true);

        setDown(on);

        blockSignals(isBlocked);
    }
}

//! Return true, if the item is checked
bool QwtLegendItem::isChecked() const
{
    return d_data->itemMode == QwtLegend::CheckableItem && isDown();
}

//! Set the item being down
void QwtLegendItem::setDown(bool down)
{
    if ( down == d_data->isDown )
        return;

    d_data->isDown = down;
    update();

    if ( d_data->itemMode == QwtLegend::ClickableItem ) {
        if ( d_data->isDown )
            emit pressed();
        else {
            emit released();
            emit clicked();
        }
    }

    if ( d_data->itemMode == QwtLegend::CheckableItem )
        emit checked(d_data->isDown);
}

//! Return true, if the item is down
bool QwtLegendItem::isDown() const
{
    return d_data->isDown;
}

//! Return a size hint
QSize QwtLegendItem::sizeHint() const
{
    QSize sz = QwtTextLabel::sizeHint();
    if ( d_data->itemMode != QwtLegend::ReadOnlyItem )
        sz += buttonShift(this);

    return sz;
}

void QwtLegendItem::drawText(QPainter *painter, const QRect &rect)
{
    QwtTextLabel::drawText(painter, rect);
}
