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
#include <qevent.h>
#include "qwt_painter.h"
#include "qwt_color_map.h"
#include "qwt_scale_widget.h"
#include "qwt_scale_map.h"
#include "qwt_math.h"
#include "qwt_paint_buffer.h"
#include "qwt_scale_div.h"
#include "qwt_text.h"

class QwtScaleWidget::PrivateData
{
public:
    PrivateData():
        scaleDraw(NULL)
    {
        colorBar.colorMap = NULL;
    }

    ~PrivateData()
    {
        delete scaleDraw;
        delete colorBar.colorMap;
    }

    QwtScaleDraw *scaleDraw;

    int borderDist[2];
    int minBorderDist[2];
    int scaleLength;
    int margin;
    int penWidth;

    int titleOffset;
    int spacing;
    QwtText title;

    struct t_colorBar
    {
        bool isEnabled;
        int width;
        QwtDoubleInterval interval;
        QwtColorMap *colorMap;
    } colorBar;
};

/*!
  \brief Create a scale with the position QwtScaleWidget::Left
  \param parent Parent widget
*/
QwtScaleWidget::QwtScaleWidget(QWidget *parent):
    QWidget(parent)
{
    initScale(QwtScaleDraw::LeftScale);
}

#if QT_VERSION < 0x040000
/*!
  \brief Create a scale with the position QwtScaleWidget::Left
  \param parent Parent widget
  \param name Object name
*/
QwtScaleWidget::QwtScaleWidget(QWidget *parent, const char *name):
    QWidget(parent, name)
{
    initScale(QwtScaleDraw::LeftScale);
}
#endif

/*!
  \brief Constructor
  \param align Alignment. 
  \param parent Parent widget
*/
QwtScaleWidget::QwtScaleWidget(
        QwtScaleDraw::Alignment align, QWidget *parent):
    QWidget(parent)
{
    initScale(align);
}

//! Destructor
QwtScaleWidget::~QwtScaleWidget()
{
    delete d_data;
}

//! Initialize the scale
void QwtScaleWidget::initScale(QwtScaleDraw::Alignment align)
{
    d_data = new PrivateData;

#if QT_VERSION < 0x040000
    setWFlags(Qt::WNoAutoErase);
#endif 

    d_data->borderDist[0] = 0;
    d_data->borderDist[1] = 0;
    d_data->minBorderDist[0] = 0;
    d_data->minBorderDist[1] = 0;
    d_data->margin = 4;
    d_data->penWidth = 0;
    d_data->titleOffset = 0;
    d_data->spacing = 2;

    d_data->scaleDraw = new QwtScaleDraw;
    d_data->scaleDraw->setAlignment(align);
    d_data->scaleDraw->setLength(10);

    d_data->colorBar.colorMap = new QwtLinearColorMap();
    d_data->colorBar.isEnabled = false;
    d_data->colorBar.width = 10;
    
    const int flags = Qt::AlignHCenter
#if QT_VERSION < 0x040000
        | Qt::WordBreak | Qt::ExpandTabs;
#else
        | Qt::TextExpandTabs | Qt::TextWordWrap;
#endif
    d_data->title.setRenderFlags(flags); 
    d_data->title.setFont(font()); 

    QSizePolicy policy(QSizePolicy::MinimumExpanding,
        QSizePolicy::Fixed);
    if ( d_data->scaleDraw->orientation() == Qt::Vertical )
        policy.transpose();

    setSizePolicy(policy);
    
#if QT_VERSION >= 0x040000
    setAttribute(Qt::WA_WState_OwnSizePolicy, false);
#else
    clearWState( WState_OwnSizePolicy );
#endif

}

void QwtScaleWidget::setTitle(const QString &title)
{
    if ( d_data->title.text() != title )
    {
        d_data->title.setText(title);
        layoutScale();
    }
}

/*!
  \brief Give title new text contents
  \param title New title
  \sa QwtScaleWidget::title
  \warning The title flags are interpreted in
               direction of the label, AlignTop, AlignBottom can't be set
               as the title will always be aligned to the scale.
*/
void QwtScaleWidget::setTitle(const QwtText &title)
{
    QwtText t = title;
    const int flags = title.renderFlags() & ~(Qt::AlignTop | Qt::AlignBottom);
    t.setRenderFlags(flags);

    if (t != d_data->title)
    {
        d_data->title = t;
        layoutScale();
    }
}

/*!
  Change the alignment

  \param alignment New alignment
  \sa QwtScaleWidget::alignment
*/
void QwtScaleWidget::setAlignment(QwtScaleDraw::Alignment alignment)
{
#if QT_VERSION >= 0x040000
    if ( !testAttribute(Qt::WA_WState_OwnSizePolicy) )
#else
    if ( !testWState( WState_OwnSizePolicy ) )
#endif
    {
        QSizePolicy policy(QSizePolicy::MinimumExpanding,
            QSizePolicy::Fixed);
        if ( d_data->scaleDraw->orientation() == Qt::Vertical )
            policy.transpose();
        setSizePolicy(policy);

#if QT_VERSION >= 0x040000
        setAttribute(Qt::WA_WState_OwnSizePolicy, false);
#else
        clearWState( WState_OwnSizePolicy );
#endif
    }

    if (d_data->scaleDraw)
        d_data->scaleDraw->setAlignment(alignment);
    layoutScale();
}

        
/*! 
    \return position 
    \sa QwtScaleWidget::setPosition
*/
QwtScaleDraw::Alignment QwtScaleWidget::alignment() const 
{
    if (!scaleDraw())
        return QwtScaleDraw::LeftScale;

    return scaleDraw()->alignment();
}

/*!
  Specify distances of the scale's endpoints from the
  widget's borders. The actual borders will never be less
  than minimum border distance.
  \param dist1 Left or top Distance
  \param dist2 Right or bottom distance
  \sa QwtScaleWidget::borderDist
*/
void QwtScaleWidget::setBorderDist(int dist1, int dist2)
{
    if ( dist1 != d_data->borderDist[0] || dist2 != d_data->borderDist[1] )
    {
        d_data->borderDist[0] = dist1;
        d_data->borderDist[1] = dist2;
        layoutScale();
    }
}

/*!
  \brief Specify the margin to the colorBar/base line.
  \param margin Margin
  \sa QwtScaleWidget::margin
*/
void QwtScaleWidget::setMargin(int margin)
{
    margin = qwtMax( 0, margin );
    if ( margin != d_data->margin )
    {
        d_data->margin = margin;
        layoutScale();
    }
}

/*!
  \brief Specify the distance between color bar, scale and title
  \param spacing Spacing
  \sa QwtScaleWidget::spacing
*/
void QwtScaleWidget::setSpacing(int spacing)
{
    spacing = qwtMax( 0, spacing );
    if ( spacing != d_data->spacing )
    {
        d_data->spacing = spacing;
        layoutScale();
    }
}

/*!
  \brief Specify the width of the scale pen
  \param width Pen width
  \sa QwtScaleWidget::penWidth
*/
void QwtScaleWidget::setPenWidth(int width)
{
    if ( width < 0 )
        width = 0;

    if ( width != d_data->penWidth )
    {
        d_data->penWidth = width;
        layoutScale();
    }
}

/*!
  \brief Change the alignment for the labels.

  \sa QwtScaleDraw::setLabelAlignment(), QwtScaleWidget::setLabelRotation()
*/
#if QT_VERSION < 0x040000
void QwtScaleWidget::setLabelAlignment(int alignment)
#else
void QwtScaleWidget::setLabelAlignment(Qt::Alignment alignment)
#endif
{
    d_data->scaleDraw->setLabelAlignment(alignment);
    layoutScale();
}

/*!
  \brief Change the rotation for the labels.
  See QwtScaleDraw::setLabelRotation().
  \sa QwtScaleDraw::setLabelRotation(), QwtScaleWidget::setLabelFlags()
*/
void QwtScaleWidget::setLabelRotation(double rotation)
{
    d_data->scaleDraw->setLabelRotation(rotation);
    layoutScale();
}

/*!
  \brief Set a scale draw
  sd has to be created with new and will be deleted in
  QwtScaleWidget::~QwtScale or the next call of QwtScaleWidget::setScaleDraw.
*/
void QwtScaleWidget::setScaleDraw(QwtScaleDraw *sd)
{
    if ( sd == NULL || sd == d_data->scaleDraw )
        return;

    if ( d_data->scaleDraw )
        sd->setAlignment(d_data->scaleDraw->alignment());

    delete d_data->scaleDraw;
    d_data->scaleDraw = sd;

    layoutScale();
}

/*! 
    scaleDraw of this scale
    \sa QwtScaleDraw::setScaleDraw
*/
const QwtScaleDraw *QwtScaleWidget::scaleDraw() const 
{ 
    return d_data->scaleDraw; 
}

/*! 
    scaleDraw of this scale
    \sa QwtScaleDraw::setScaleDraw
*/
QwtScaleDraw *QwtScaleWidget::scaleDraw() 
{ 
    return d_data->scaleDraw; 
}

/*! 
    \return title 
    \sa QwtScaleWidget::setTitle
*/
QwtText QwtScaleWidget::title() const 
{
    return d_data->title;
}

/*! 
    \return start border distance 
    \sa QwtScaleWidget::setBorderDist
*/
int QwtScaleWidget::startBorderDist() const 
{ 
    return d_data->borderDist[0]; 
}  

/*! 
    \return end border distance 
    \sa QwtScaleWidget::setBorderDist
*/
int QwtScaleWidget::endBorderDist() const 
{ 
    return d_data->borderDist[1]; 
}

/*! 
    \return margin
    \sa QwtScaleWidget::setMargin
*/
int QwtScaleWidget::margin() const 
{ 
    return d_data->margin; 
}

/*! 
    \return distance between scale and title
    \sa QwtScaleWidget::setMargin
*/
int QwtScaleWidget::spacing() const 
{ 
    return d_data->spacing; 
}

/*! 
    \return Scale pen width
    \sa QwtScaleWidget::setPenWidth
*/
int QwtScaleWidget::penWidth() const
{
    return d_data->penWidth;
} 
/*!
  \brief paintEvent
*/
void QwtScaleWidget::paintEvent(QPaintEvent *e)
{
    const QRect &ur = e->rect();
    if ( ur.isValid() )
    {
#if QT_VERSION < 0x040000
        QwtPaintBuffer paintBuffer(this, ur);
        draw(paintBuffer.painter());
#else
        QPainter painter(this);
        draw(&painter);
#endif
    }
}

/*!
  \brief draw the scale
*/
void QwtScaleWidget::draw(QPainter *painter) const
{
    painter->save();

    QPen scalePen = painter->pen();
    scalePen.setWidth(d_data->penWidth);
    painter->setPen(scalePen);
    
#if QT_VERSION < 0x040000
    d_data->scaleDraw->draw(painter, colorGroup());
#else
    d_data->scaleDraw->draw(painter, palette());
#endif
    painter->restore();

    if ( d_data->colorBar.isEnabled && d_data->colorBar.width > 0 &&
        d_data->colorBar.interval.isValid() )
    {
        drawColorBar(painter, colorBarRect(rect()));
    }

    QRect r = rect();
    if ( d_data->scaleDraw->orientation() == Qt::Horizontal )
    {
        r.setLeft(r.left() + d_data->borderDist[0]);
        r.setWidth(r.width() - d_data->borderDist[1]);
    }
    else
    {
        r.setTop(r.top() + d_data->borderDist[0]);
        r.setHeight(r.height() - d_data->borderDist[1]);
    }

    if ( !d_data->title.isEmpty() )
    {
        QRect tr = r;
        switch(d_data->scaleDraw->alignment())
        {
            case QwtScaleDraw::LeftScale:
                tr.setRight( r.right() - d_data->titleOffset );
                break;

            case QwtScaleDraw::RightScale:
                tr.setLeft( r.left() + d_data->titleOffset );
                break;

            case QwtScaleDraw::BottomScale:
                tr.setTop( r.top() + d_data->titleOffset );
                break;

            case QwtScaleDraw::TopScale:
            default:
                tr.setBottom( r.bottom() - d_data->titleOffset );
                break;
        }

        drawTitle(painter, d_data->scaleDraw->alignment(), tr);
    }
}

QRect QwtScaleWidget::colorBarRect(const QRect& rect) const
{
    QRect cr = rect;

    if ( d_data->scaleDraw->orientation() == Qt::Horizontal )
    {
        cr.setLeft(cr.left() + d_data->borderDist[0]);
        cr.setWidth(cr.width() - d_data->borderDist[1] + 1);
    }
    else
    {
        cr.setTop(cr.top() + d_data->borderDist[0]);
        cr.setHeight(cr.height() - d_data->borderDist[1] + 1);
    }

    switch(d_data->scaleDraw->alignment())
    {
        case QwtScaleDraw::LeftScale:
        {
            cr.setLeft( cr.right() - d_data->spacing 
                - d_data->colorBar.width + 1 );
            cr.setWidth(d_data->colorBar.width);
            break;
        }

        case QwtScaleDraw::RightScale:
        {
            cr.setLeft( cr.left() + d_data->spacing );
            cr.setWidth(d_data->colorBar.width);
            break;
        }

        case QwtScaleDraw::BottomScale:
        {
            cr.setTop( cr.top() + d_data->spacing );
            cr.setHeight(d_data->colorBar.width);
            break;
        }

        case QwtScaleDraw::TopScale:
        {
            cr.setTop( cr.bottom() - d_data->spacing
                - d_data->colorBar.width + 1 );
            cr.setHeight(d_data->colorBar.width);
            break;
        }
    }

    return cr;
}

/*!
  \brief resizeEvent
*/
void QwtScaleWidget::resizeEvent(QResizeEvent *)
{
    layoutScale(false);
}

//! Recalculate the scale's geometry and layout based on
//  the current rect and fonts.
//  \param update_geometry   notify the layout system and call update
//         to redraw the scale

void QwtScaleWidget::layoutScale( bool update_geometry )
{
    int bd0, bd1;
    getBorderDistHint(bd0, bd1);
    if ( d_data->borderDist[0] > bd0 )
        bd0 = d_data->borderDist[0];
    if ( d_data->borderDist[1] > bd1 )
        bd1 = d_data->borderDist[1];

    int colorBarWidth = 0;
    if ( d_data->colorBar.isEnabled && d_data->colorBar.interval.isValid() )
        colorBarWidth = d_data->colorBar.width + d_data->spacing;

    const QRect r = rect();
    int x, y, length;

    if ( d_data->scaleDraw->orientation() == Qt::Vertical )
    {
        y = r.top() + bd0;
        length = r.height() - (bd0 + bd1);

        if ( d_data->scaleDraw->alignment() == QwtScaleDraw::LeftScale )
            x = r.right() - d_data->margin - colorBarWidth;
        else
            x = r.left() + d_data->margin + colorBarWidth;
    }
    else
    {
        x = r.left() + bd0; 
        length = r.width() - (bd0 + bd1);

        if ( d_data->scaleDraw->alignment() == QwtScaleDraw::BottomScale )
            y = r.top() + d_data->margin + colorBarWidth;
        else
            y = r.bottom() - d_data->margin - colorBarWidth;
    }

    d_data->scaleDraw->move(x, y);
    d_data->scaleDraw->setLength(length);

    d_data->titleOffset = d_data->margin + d_data->spacing +
        colorBarWidth +
        d_data->scaleDraw->extent(QPen(Qt::black, d_data->penWidth), font());

    if ( update_geometry )
    {
      updateGeometry();
      update();
    }
}

void QwtScaleWidget::drawColorBar(QPainter *painter, const QRect& rect) const
{
    if ( !d_data->colorBar.interval.isValid() )
        return;

    const QwtScaleDraw* sd = d_data->scaleDraw;

    QwtPainter::drawColorBar(painter, *d_data->colorBar.colorMap, 
        d_data->colorBar.interval.normalized(), sd->map(), 
        sd->orientation(), rect);
}

/*!
  Rotate and paint a title according to its position into a given rectangle.
  \param painter Painter
  \param align Alignment
  \param rect Bounding rectangle
*/

void QwtScaleWidget::drawTitle(QPainter *painter,
    QwtScaleDraw::Alignment align, const QRect &rect) const
{
    QRect r;
    double angle;
    int flags = d_data->title.renderFlags() & 
        ~(Qt::AlignTop | Qt::AlignBottom | Qt::AlignVCenter);

    switch(align)
    {
        case QwtScaleDraw::LeftScale:
            flags |= Qt::AlignTop;
            angle = -90.0;
            r.setRect(rect.left(), rect.bottom(), rect.height(), rect.width());
            break;
        case QwtScaleDraw::RightScale:
            flags |= Qt::AlignTop;
            angle = 90.0;
            r.setRect(rect.right(), rect.top(), rect.height(), rect.width());
            break;
        case QwtScaleDraw::TopScale:
            flags |= Qt::AlignTop;
            angle = 0.0;
            r = rect;
            break;
        case QwtScaleDraw::BottomScale:
        default:
            flags |= Qt::AlignBottom;
            angle = 0.0;
            r = rect;
            break;
    }

    painter->save();
    painter->setFont(font());
#if QT_VERSION < 0x040000
    painter->setPen(colorGroup().color(QColorGroup::Text));
#else
    painter->setPen(palette().color(QPalette::Text));
#endif

    painter->translate(r.x(), r.y());
    if (angle != 0.0)
        painter->rotate(angle);

    QwtText title = d_data->title;
    title.setRenderFlags(flags);
    title.draw(painter, QRect(0, 0, r.width(), r.height()));

    painter->restore();
}

/*!
  \brief Notify a change of the scale

  This virtual function can be overloaded by derived
  classes. The default implementation updates the geometry
  and repaints the widget.
*/

void QwtScaleWidget::scaleChange()
{
    layoutScale();
}

/*!
  \return a size hint
*/
QSize QwtScaleWidget::sizeHint() const
{
    return minimumSizeHint();
}

/*!
  \return a minimum size hint
*/
QSize QwtScaleWidget::minimumSizeHint() const
{
    const Qt::Orientation o = d_data->scaleDraw->orientation();

    // Border Distance cannot be less than the scale borderDistHint
    // Note, the borderDistHint is already included in minHeight/minWidth
    int length = 0;
    int mbd1, mbd2;
    getBorderDistHint(mbd1, mbd2);
    length += qwtMax( 0, d_data->borderDist[0] - mbd1 );
    length += qwtMax( 0, d_data->borderDist[1] - mbd2 );
    length += d_data->scaleDraw->minLength(
        QPen(Qt::black, d_data->penWidth), font());

    int dim = dimForLength(length, font());
    if ( length < dim )
    {
        // compensate for long titles
        length = dim;
        dim = dimForLength(length, font());
    }

    QSize size(length + 2, dim);
    if ( o == Qt::Vertical )
        size.transpose();

    return size;
}

/*!
  \brief Find the height of the title for a given width.
  \param width Width
  \return height Height
 */

int QwtScaleWidget::titleHeightForWidth(int width) const
{
    return d_data->title.heightForWidth(width, font());
}

/*!
  \brief Find the minimum dimension for a given length.
         dim is the height, length the width seen in
         direction of the title.
  \param length width for horizontal, height for vertical scales
  \param scaleFont Font of the scale
  \return height for horizontal, width for vertical scales
*/

int QwtScaleWidget::dimForLength(int length, const QFont &scaleFont) const
{
    int dim = d_data->margin;
    dim += d_data->scaleDraw->extent(
        QPen(Qt::black, d_data->penWidth), scaleFont);

    if ( !d_data->title.isEmpty() )
        dim += titleHeightForWidth(length) + d_data->spacing;

    if ( d_data->colorBar.isEnabled && d_data->colorBar.interval.isValid() )
        dim += d_data->colorBar.width + d_data->spacing;

    return dim;
}

/*!
  \brief Calculate a hint for the border distances.

  This member function calculates the distance
  of the scale's endpoints from the widget borders which
  is required for the mark labels to fit into the widget.
  The maximum of this distance an the minimum border distance
  is returned.

  \warning
  <ul> <li>The minimum border distance depends on the font.</ul>
  \sa setMinBorderDist(), getMinBorderDist(), setBorderDist()
*/
void QwtScaleWidget::getBorderDistHint(int &start, int &end) const
{
    d_data->scaleDraw->getBorderDistHint(font(), start, end);

    if ( start < d_data->minBorderDist[0] )
        start = d_data->minBorderDist[0];

    if ( end < d_data->minBorderDist[1] )
        end = d_data->minBorderDist[1];
}

/*!
  Set a minimum value for the distances of the scale's endpoints from 
  the widget borders. This is useful to avoid that the scales
  are "jumping", when the tick labels or their positions change 
  often.

  \sa getMinBorderDist(), getBorderDistHint()
*/
void QwtScaleWidget::setMinBorderDist(int start, int end)
{
    d_data->minBorderDist[0] = start;
    d_data->minBorderDist[1] = end;
}

/*!
  Get the minimum value for the distances of the scale's endpoints from 
  the widget borders.

  \sa setMinBorderDist(), getBorderDistHint()
*/
void QwtScaleWidget::getMinBorderDist(int &start, int &end) const
{
    start = d_data->minBorderDist[0];
    end = d_data->minBorderDist[1];
}

#if QT_VERSION < 0x040000

/*!
  \brief Notify a change of the font

  This virtual function may be overloaded by derived widgets.
  The default implementation resizes the scale and repaints
  the widget.
  \param oldFont Previous font
*/
void QwtScaleWidget::fontChange(const QFont &oldFont)
{
    QWidget::fontChange( oldFont );
    layoutScale();
}

#endif

/*!
  \brief Assign a scale division

  The scale division determines where to set the tick marks.

  \param transformation Transformation, needed to translate between
                        scale and pixal values
  \param scaleDiv Scale Division
  \sa For more information about scale divisions, see QwtScaleDiv.
*/
void QwtScaleWidget::setScaleDiv(
    QwtScaleTransformation *transformation,
    const QwtScaleDiv &scaleDiv)
{
    QwtScaleDraw *sd = d_data->scaleDraw;
    if (sd->scaleDiv() != scaleDiv ||
        sd->map().transformation()->type() != transformation->type() )
    {
        sd->setTransformation(transformation);
        sd->setScaleDiv(scaleDiv);
        layoutScale();

        emit scaleDivChanged();
    }
    else
        delete transformation;
}

void QwtScaleWidget::setColorBarEnabled(bool on)
{
    if ( on != d_data->colorBar.isEnabled )
    {
        d_data->colorBar.isEnabled = on;
        layoutScale();
    }
}

bool QwtScaleWidget::isColorBarEnabled() const
{
    return d_data->colorBar.isEnabled;
}


void QwtScaleWidget::setColorBarWidth(int width)
{
    if ( width != d_data->colorBar.width )
    {
        d_data->colorBar.width = width;
        if ( isColorBarEnabled() )
            layoutScale();
    }
}

int QwtScaleWidget::colorBarWidth() const
{
    return d_data->colorBar.width;
}

QwtDoubleInterval QwtScaleWidget::colorBarInterval() const
{
    return d_data->colorBar.interval;
}

void QwtScaleWidget::setColorMap(const QwtDoubleInterval &interval,
    const QwtColorMap &colorMap)
{
    d_data->colorBar.interval = interval;

    delete d_data->colorBar.colorMap;
    d_data->colorBar.colorMap = colorMap.copy();

    if ( isColorBarEnabled() )
        layoutScale();
}

const QwtColorMap &QwtScaleWidget::colorMap() const
{
    return *d_data->colorBar.colorMap;
}
