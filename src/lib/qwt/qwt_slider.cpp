/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

// vim: expandtab

#include <math.h>
#include <qevent.h>
#include <qdrawutil.h>
#include <qpainter.h>
#include "qwt_painter.h"
#include "qwt_paint_buffer.h"
#include "qwt_scale_draw.h"
#include "qwt_scale_map.h"
#include "qwt_slider.h"

class QwtSlider::PrivateData
{
public:
    QRect sliderRect;

    int thumbLength;
    int thumbWidth;
    int borderWidth;
    int scaleDist;
    int xMargin;
    int yMargin;

    QwtSlider::ScalePos scalePos;
    QwtSlider::BGSTYLE bgStyle;

    /*
      Scale and values might have different maps. This is
      confusing and I can't see strong arguments for such
      a feature. TODO ...
     */
    QwtScaleMap map; // linear map
    mutable QSize sizeHintCache;
};

/*!
  \brief Constructor
  \param parent parent widget
  \param orientation Orientation of the slider. Can be Qt::Horizontal
         or Qt::Vertical. Defaults to Qt::Horizontal.
  \param scalePos Position of the scale.  
         Defaults to QwtSlider::NoScale.
  \param bgStyle Background style. QwtSlider::BgTrough draws the
         slider button in a trough, QwtSlider::BgSlot draws
         a slot underneath the button. An or-combination of both
         may also be used. The default is QwtSlider::BgTrough.

  QwtSlider enforces valid combinations of its orientation and scale position.
  If the combination is invalid, the scale position will be set to NoScale. 
  Valid combinations are:
  - Qt::Horizonal with NoScale, TopScale, or BottomScale;
  - Qt::Vertical with NoScale, LeftScale, or RightScale.
*/
QwtSlider::QwtSlider(QWidget *parent,
        Qt::Orientation orientation, ScalePos scalePos, BGSTYLE bgStyle): 
    QwtAbstractSlider(orientation, parent)
{
    initSlider(orientation, scalePos, bgStyle);
}

#if QT_VERSION < 0x040000
/*!
  \brief Constructor

  Build a horizontal slider with no scale and BgTrough as 
  background style

  \param parent parent widget
  \param name Object name
*/
QwtSlider::QwtSlider(QWidget *parent, const char* name):
    QwtAbstractSlider(Qt::Horizontal, parent)
{
    setName(name);
    initSlider(Qt::Horizontal, NoScale, BgTrough);
}
#endif

void QwtSlider::initSlider(Qt::Orientation orientation, 
    ScalePos scalePos, BGSTYLE bgStyle)
{
    if (orientation == Qt::Vertical) 
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    else
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

#if QT_VERSION >= 0x040000
    setAttribute(Qt::WA_WState_OwnSizePolicy, false);
#else
    clearWState( WState_OwnSizePolicy );
#endif


#if QT_VERSION < 0x040000
    setWFlags(Qt::WNoAutoErase);
#endif

    d_data = new QwtSlider::PrivateData;

    d_data->borderWidth = 2;
    d_data->scaleDist = 4;
    d_data->scalePos = scalePos;
    d_data->xMargin = 0;
    d_data->yMargin = 0;
    d_data->bgStyle = bgStyle;

    if (bgStyle == BgSlot)
    {
        d_data->thumbLength = 16;
        d_data->thumbWidth = 30;
    }
    else
    {
        d_data->thumbLength = 31;
        d_data->thumbWidth = 16;
    }

    d_data->sliderRect.setRect(0,0,8,8);

    QwtScaleDraw::Alignment align;
    if ( orientation == Qt::Vertical )
    {
        // enforce a valid combination of scale position and orientation
        if ((d_data->scalePos == BottomScale) || (d_data->scalePos == TopScale))
            d_data->scalePos = NoScale;
        // adopt the policy of layoutSlider (NoScale lays out like Left)
        if (d_data->scalePos == RightScale)
           align = QwtScaleDraw::RightScale;
        else
           align = QwtScaleDraw::LeftScale;
    }
    else
    {
        // enforce a valid combination of scale position and orientation
        if ((d_data->scalePos == LeftScale) || (d_data->scalePos == RightScale))
            d_data->scalePos = NoScale;
        // adopt the policy of layoutSlider (NoScale lays out like Bottom)
        if (d_data->scalePos == TopScale)
           align = QwtScaleDraw::TopScale;
        else
           align = QwtScaleDraw::BottomScale;
    }

    scaleDraw()->setAlignment(align);
    scaleDraw()->setLength(100);

    setRange(0.0, 100.0, 1.0);
    setValue(0.0);
}

QwtSlider::~QwtSlider()
{
    delete d_data;
}

/*!
  \brief Set the orientation.
  \param o Orientation. Allowed values are Qt::Horizontal and Qt::Vertical.
  
  If the new orientation and the old scale position are an invalid combination,
  the scale position will be set to QwtSlider::NoScale.
  \sa QwtAbstractSlider::orientation()
*/
void QwtSlider::setOrientation(Qt::Orientation o) 
{
    if ( o == orientation() )
        return;

    if (o == Qt::Horizontal)
    {
        if ((d_data->scalePos == LeftScale) || (d_data->scalePos == RightScale))
            d_data->scalePos = NoScale;
    }
    else // if (o == Qt::Vertical)
    {
        if ((d_data->scalePos == BottomScale) || (d_data->scalePos == TopScale))
            d_data->scalePos = NoScale;
    }

#if QT_VERSION >= 0x040000
    if ( !testAttribute(Qt::WA_WState_OwnSizePolicy) )
#else
    if ( !testWState( WState_OwnSizePolicy ) ) 
#endif
    {
        QSizePolicy sp = sizePolicy();
        sp.transpose();
        setSizePolicy(sp);

#if QT_VERSION >= 0x040000
        setAttribute(Qt::WA_WState_OwnSizePolicy, false);
#else
        clearWState( WState_OwnSizePolicy );
#endif
    }

    QwtAbstractSlider::setOrientation(o);
    layoutSlider();
}

/*!
  \brief Change the scale position (and slider orientation).

  \param s Position of the scale.

  A valid combination of scale position and orientation is enforced:
  - if the new scale position is Left or Right, the scale orientation will
    become Qt::Vertical;
  - if the new scale position is Bottom or Top the scale orientation will
    become Qt::Horizontal;
  - if the new scale position is QwtSlider::NoScale, the scale 
    orientation will not change.
*/
void QwtSlider::setScalePosition(ScalePos s)
{
    if ( d_data->scalePos == s )
        return;

    d_data->scalePos = s;

    switch(d_data->scalePos)
    {
        case BottomScale:
        {
            setOrientation(Qt::Horizontal);
            scaleDraw()->setAlignment(QwtScaleDraw::BottomScale);
            break;
        }
        case TopScale:
        {
            setOrientation(Qt::Horizontal);
            scaleDraw()->setAlignment(QwtScaleDraw::TopScale);
            break;
        }
        case LeftScale:
        {
            setOrientation(Qt::Vertical);
            scaleDraw()->setAlignment(QwtScaleDraw::LeftScale);
            break;
        }
        case RightScale:
        {
            setOrientation(Qt::Vertical);
            scaleDraw()->setAlignment(QwtScaleDraw::RightScale);
            break;
        }
        default:
        {
            // nothing
        }
    }

    layoutSlider();
}

//! Return the scale position.
QwtSlider::ScalePos QwtSlider::scalePosition() const
{
    return d_data->scalePos;
}

/*!
  \brief Change the slider's border width
  \param bd border width
*/
void QwtSlider::setBorderWidth(int bd)
{
    if ( bd < 0 )
        bd = 0;

    if ( bd != d_data->borderWidth )
    {
        d_data->borderWidth = bd;
        layoutSlider();
    }
}

/*!
  \brief Set the slider's thumb length
  \param thumbLength new length
*/
void QwtSlider::setThumbLength(int thumbLength)
{
    if ( thumbLength < 8 )
        thumbLength = 8;

    if ( thumbLength != d_data->thumbLength )
    {
        d_data->thumbLength = thumbLength;
        layoutSlider();
    }
}

/*!
  \brief Change the width of the thumb
  \param w new width
*/
void QwtSlider::setThumbWidth(int w)
{
    if ( w < 4 )
        w = 4;

    if ( d_data->thumbWidth != w )
    {
        d_data->thumbWidth = w;
        layoutSlider();
    }
}

/*!
  \brief Set a scale draw

  For changing the labels of the scales, it
  is necessary to derive from QwtScaleDraw and
  overload QwtScaleDraw::label().

  \param scaleDraw ScaleDraw object, that has to be created with 
                   new and will be deleted in ~QwtSlider or the next 
                   call of setScaleDraw().
*/
void QwtSlider::setScaleDraw(QwtScaleDraw *scaleDraw)
{
    const QwtScaleDraw *previousScaleDraw = this->scaleDraw();
    if ( scaleDraw == NULL || scaleDraw == previousScaleDraw )
        return;

    if ( previousScaleDraw )
        scaleDraw->setAlignment(previousScaleDraw->alignment());

    setAbstractScaleDraw(scaleDraw);
    layoutSlider();
}

/*!
  \return the scale draw of the slider
  \sa setScaleDraw()
*/
const QwtScaleDraw *QwtSlider::scaleDraw() const
{
    return (QwtScaleDraw *)abstractScaleDraw();
}

/*!
  \return the scale draw of the slider
  \sa setScaleDraw()
*/
QwtScaleDraw *QwtSlider::scaleDraw()
{
    return (QwtScaleDraw *)abstractScaleDraw();
}

//! Notify changed scale
void QwtSlider::scaleChange()
{
    layoutSlider();
}


//! Notify change in font
void QwtSlider::fontChange(const QFont &f)
{
    QwtAbstractSlider::fontChange( f );
    layoutSlider();
}

//! Draw the slider into the specified rectangle.
void QwtSlider::drawSlider(QPainter *p, const QRect &r)
{
    QRect cr(r);

    if (d_data->bgStyle & BgTrough)
    {
        qDrawShadePanel(p, r.x(), r.y(),
            r.width(), r.height(),
#if QT_VERSION < 0x040000
            colorGroup(), 
#else
            palette(), 
#endif
            true, d_data->borderWidth,0);

        cr.setRect(r.x() + d_data->borderWidth,
            r.y() + d_data->borderWidth,
            r.width() - 2 * d_data->borderWidth,
            r.height() - 2 * d_data->borderWidth);

        p->fillRect(cr.x(), cr.y(), cr.width(), cr.height(), 
#if QT_VERSION < 0x040000
            colorGroup().brush(QColorGroup::Mid)
#else
            palette().brush(QPalette::Mid)
#endif
        );
    }

    if ( d_data->bgStyle & BgSlot)
    {
        int ws = 4;
        int ds = d_data->thumbLength / 2 - 4;
        if ( ds < 1 )
            ds = 1;

        QRect rSlot;
        if (orientation() == Qt::Horizontal)
        {
            if ( cr.height() & 1 )
                ws++;
            rSlot = QRect(cr.x() + ds, 
                    cr.y() + (cr.height() - ws) / 2,
                    cr.width() - 2 * ds, ws);
        }
        else
        {
            if ( cr.width() & 1 )
                ws++;
            rSlot = QRect(cr.x() + (cr.width() - ws) / 2, 
                    cr.y() + ds,
                    ws, cr.height() - 2 * ds);
        }
        p->fillRect(rSlot.x(), rSlot.y(), rSlot.width(), rSlot.height(),
#if QT_VERSION < 0x040000
            colorGroup().brush(QColorGroup::Dark)
#else
            palette().brush(QPalette::Dark)
#endif
        );
        qDrawShadePanel(p, rSlot.x(), rSlot.y(),
            rSlot.width(), rSlot.height(), 
#if QT_VERSION < 0x040000
            colorGroup(), 
#else
            palette(), 
#endif
            true, 1 ,0);

    }

    if ( isValid() )
        drawThumb(p, cr, xyPosition(value()));
}

//! Draw the thumb at a position
void QwtSlider::drawThumb(QPainter *p, const QRect &sliderRect, int pos)
{
    pos++; // shade line points one pixel below
    if (orientation() == Qt::Horizontal)
    {
        qDrawShadePanel(p, pos - d_data->thumbLength / 2, 
            sliderRect.y(), d_data->thumbLength, sliderRect.height(),
#if QT_VERSION < 0x040000
            colorGroup(), 
#else
            palette(), 
#endif
            false, d_data->borderWidth, 
#if QT_VERSION < 0x040000
            &colorGroup().brush(QColorGroup::Button)
#else
            &palette().brush(QPalette::Button)
#endif
        );

        qDrawShadeLine(p, pos, sliderRect.y(), 
            pos, sliderRect.y() + sliderRect.height() - 2, 
#if QT_VERSION < 0x040000
            colorGroup(), 
#else
            palette(), 
#endif
            true, 1);
    }
    else // Vertical
    {
        qDrawShadePanel(p,sliderRect.x(), pos - d_data->thumbLength / 2, 
            sliderRect.width(), d_data->thumbLength,
#if QT_VERSION < 0x040000
            colorGroup(),
#else
            palette(), 
#endif
            false, d_data->borderWidth, 
#if QT_VERSION < 0x040000
            &colorGroup().brush(QColorGroup::Button)
#else
            &palette().brush(QPalette::Button)
#endif
        );

        qDrawShadeLine(p, sliderRect.x(), pos,
            sliderRect.x() + sliderRect.width() - 2, pos, 
#if QT_VERSION < 0x040000
            colorGroup(), 
#else
            palette(), 
#endif
            true, 1);
    }
}

//! Find the x/y position for a given value v
int QwtSlider::xyPosition(double v) const
{
    return d_data->map.transform(v);
}

//! Determine the value corresponding to a specified mouse location.
double QwtSlider::getValue(const QPoint &p)
{
    return d_data->map.invTransform(
        orientation() == Qt::Horizontal ? p.x() : p.y());
}


/*!
  \brief Determine scrolling mode and direction
  \param p point
  \param scrollMode Scrolling mode
  \param direction Direction
*/
void QwtSlider::getScrollMode(const QPoint &p, 
    int &scrollMode, int &direction )
{
    if (!d_data->sliderRect.contains(p))
    {
        scrollMode = ScrNone;
        direction = 0;
        return;
    }

    const int pos = ( orientation() == Qt::Horizontal ) ? p.x() : p.y();
    const int markerPos = xyPosition(value());

    if ((pos > markerPos - d_data->thumbLength / 2)
        && (pos < markerPos + d_data->thumbLength / 2))
    {
        scrollMode = ScrMouse;
        direction = 0;
        return;
    }

    scrollMode = ScrPage;
    direction = (pos > markerPos) ? 1 : -1;

    if ( scaleDraw()->map().p1() > scaleDraw()->map().p2() )
        direction = -direction;
}

//! Qt paint event
void QwtSlider::paintEvent(QPaintEvent *e)
{
    const QRect &ur = e->rect();
    if ( ur.isValid() )
    {
#if QT_VERSION < 0x040000
        QwtPaintBuffer paintBuffer(this, ur);
        draw(paintBuffer.painter(), ur);
#else
        QPainter painter(this);
        draw(&painter, ur);
#endif
    }
}

//! Draw the QwtSlider
void QwtSlider::draw(QPainter *painter, const QRect&)
{
    if (d_data->scalePos != NoScale)
    {
#if QT_VERSION < 0x040000
        scaleDraw()->draw(painter, colorGroup());
#else
        scaleDraw()->draw(painter, palette());
#endif
    }

    drawSlider(painter, d_data->sliderRect);

    if ( hasFocus() )
        QwtPainter::drawFocusRect(painter, this, d_data->sliderRect);
}

//! Qt resize event
void QwtSlider::resizeEvent(QResizeEvent *)
{
    layoutSlider( false );
}

/*!
  Recalculate the slider's geometry and layout based on
  the current rect and fonts.
  \param update_geometry  notify the layout system and call update
         to redraw the scale
*/
void QwtSlider::layoutSlider( bool update_geometry )
{
    int sliderWidth = d_data->thumbWidth;
    int sld1 = d_data->thumbLength / 2 - 1;
    int sld2 = d_data->thumbLength / 2 + d_data->thumbLength % 2;
    if ( d_data->bgStyle & BgTrough )
    {
        sliderWidth += 2 * d_data->borderWidth;
        sld1 += d_data->borderWidth;
        sld2 += d_data->borderWidth;
    }

    int scd = 0;
    if ( d_data->scalePos != NoScale )
    {
        int d1, d2;
        scaleDraw()->getBorderDistHint(font(), d1, d2);
        scd = qwtMax(d1, d2);
    }

    int slo = scd - sld1;
    if ( slo < 0 )
        slo = 0;

    int x, y, length;

    const QRect r = rect();
    if (orientation() == Qt::Horizontal)
    {
        switch (d_data->scalePos)
        {
            case TopScale:
            {
                d_data->sliderRect.setRect(
                    r.x() + d_data->xMargin + slo,
                    r.y() + r.height() -
                    d_data->yMargin - sliderWidth,
                    r.width() - 2 * d_data->xMargin - 2 * slo,
                    sliderWidth);

                x = d_data->sliderRect.x() + sld1;
                y = d_data->sliderRect.y() - d_data->scaleDist;

                break;
            }

            case BottomScale:
            {
                d_data->sliderRect.setRect(
                    r.x() + d_data->xMargin + slo,
                    r.y() + d_data->yMargin,
                    r.width() - 2 * d_data->xMargin - 2 * slo,
                    sliderWidth);
    
                x = d_data->sliderRect.x() + sld1;
                y = d_data->sliderRect.y() + d_data->sliderRect.height() 
                    + d_data->scaleDist;

                break;
            }

            case NoScale: // like Bottom, but no scale. See QwtSlider().
            default:   // inconsistent orientation and scale position
            {
                d_data->sliderRect.setRect(
                    r.x() + d_data->xMargin + slo,
                    r.y() + d_data->yMargin,
                    r.width() - 2 * d_data->xMargin - 2 * slo,
                    sliderWidth);

                x = d_data->sliderRect.x() + sld1;
                y = 0;

                break;
            }
        }
        length = d_data->sliderRect.width() - (sld1 + sld2);
    }
    else // if (orientation() == Qt::Vertical
    {
        switch (d_data->scalePos)
        {
            case RightScale:
                d_data->sliderRect.setRect(
                    r.x() + d_data->xMargin,
                    r.y() + d_data->yMargin + slo,
                    sliderWidth,
                    r.height() - 2 * d_data->yMargin - 2 * slo);

                x = d_data->sliderRect.x() + d_data->sliderRect.width() 
                    + d_data->scaleDist;
                y = d_data->sliderRect.y() + sld1;

                break;

            case LeftScale:
                d_data->sliderRect.setRect(
                    r.x() + r.width() - sliderWidth - d_data->xMargin,
                    r.y() + d_data->yMargin + slo,
                    sliderWidth,
                    r.height() - 2 * d_data->yMargin - 2 * slo);

                x = d_data->sliderRect.x() - d_data->scaleDist;
                y = d_data->sliderRect.y() + sld1;

                break;

            case NoScale: // like Left, but no scale. See QwtSlider().
            default:   // inconsistent orientation and scale position
                d_data->sliderRect.setRect(
                    r.x() + r.width() - sliderWidth - d_data->xMargin,
                    r.y() + d_data->yMargin + slo,
                    sliderWidth,
                    r.height() - 2 * d_data->yMargin - 2 * slo);

                x = 0;
                y = d_data->sliderRect.y() + sld1;

                break;
        }
        length = d_data->sliderRect.height() - (sld1 + sld2);
    }

    scaleDraw()->move(x, y);
    scaleDraw()->setLength(length);

    d_data->map.setPaintXInterval(scaleDraw()->map().p1(),
        scaleDraw()->map().p2());

    if ( update_geometry )
    {
        d_data->sizeHintCache = QSize(); // invalidate
        updateGeometry();
        update();
    }
}

//! Notify change of value
void QwtSlider::valueChange()
{
    QwtAbstractSlider::valueChange();
    update();
}


//! Notify change of range
void QwtSlider::rangeChange()
{
    d_data->map.setScaleInterval(minValue(), maxValue());

    if (autoScale())
        rescale(minValue(), maxValue());

    QwtAbstractSlider::rangeChange();
    layoutSlider();
}

/*!
  \brief Set distances between the widget's border and internals.
  \param xMargin Horizontal margin
  \param yMargin Vertical margin
*/
void QwtSlider::setMargins(int xMargin, int yMargin)
{
    if ( xMargin < 0 )
        xMargin = 0;
    if ( yMargin < 0 )
        yMargin = 0;

    if ( xMargin != d_data->xMargin || yMargin != d_data->yMargin )
    {
        d_data->xMargin = xMargin;
        d_data->yMargin = yMargin;
        layoutSlider();
    }
}

/*!
  Set the background style.
*/
void QwtSlider::setBgStyle(BGSTYLE st) 
{
    d_data->bgStyle = st; 
    layoutSlider();
}

/*!
  \return the background style.
*/
QwtSlider::BGSTYLE QwtSlider::bgStyle() const 
{ 
    return d_data->bgStyle; 
}

/*!
  \return the thumb length.
*/
int QwtSlider::thumbLength() const 
{
    return d_data->thumbLength;
}

/*!
  \return the thumb width.
*/
int QwtSlider::thumbWidth() const 
{
    return d_data->thumbWidth;
}

/*!
  \return the border width.
*/
int QwtSlider::borderWidth() const 
{
    return d_data->borderWidth;
}

/*!
  \return QwtSlider::minimumSizeHint()
*/
QSize QwtSlider::sizeHint() const
{
    return minimumSizeHint();
}

/*!
  \brief Return a minimum size hint
  \warning The return value of QwtSlider::minimumSizeHint() depends on 
           the font and the scale.
*/
QSize QwtSlider::minimumSizeHint() const
{
    if (!d_data->sizeHintCache.isEmpty()) 
        return d_data->sizeHintCache;

    int sliderWidth = d_data->thumbWidth;
    if (d_data->bgStyle & BgTrough)
        sliderWidth += 2 * d_data->borderWidth;

    int w = 0, h = 0;
    if (d_data->scalePos != NoScale)
    {
        int d1, d2;
        scaleDraw()->getBorderDistHint(font(), d1, d2);
        int msMbd = qwtMax(d1, d2);

        int mbd = d_data->thumbLength / 2;
        if (d_data->bgStyle & BgTrough)
            mbd += d_data->borderWidth;

        if ( mbd < msMbd )
            mbd = msMbd;

        const int sdExtent = scaleDraw()->extent( QPen(), font() );
        const int sdLength = scaleDraw()->minLength( QPen(), font() );

        h = sliderWidth + sdExtent + d_data->scaleDist;
        w = sdLength - 2 * msMbd + 2 * mbd;
    }
    else  // no scale
    {
        w = 200;
        h = sliderWidth;
    }

    if ( orientation() == Qt::Vertical )
        qSwap(w, h);

    w += 2 * d_data->xMargin;
    h += 2 * d_data->yMargin;

    d_data->sizeHintCache = QSize(w, h);
    return d_data->sizeHintCache;
}
