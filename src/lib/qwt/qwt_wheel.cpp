/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include <qevent.h>
#include <qdrawutil.h>
#include <qpainter.h>
#include <qstyle.h>
#include "qwt_math.h"
#include "qwt_painter.h"
#include "qwt_paint_buffer.h"
#include "qwt_wheel.h"

#define NUM_COLORS 30

class QwtWheel::PrivateData
{
public:
    PrivateData()
    {
        viewAngle = 175.0;
        totalAngle = 360.0;
        tickCnt = 10;
        intBorder = 2;
        borderWidth = 2;
        wheelWidth = 20;
#if QT_VERSION < 0x040000
        allocContext = 0;
#endif
    };

    QRect sliderRect;
    double viewAngle;
    double totalAngle;
    int tickCnt;
    int intBorder;
    int borderWidth;
    int wheelWidth;
#if QT_VERSION < 0x040000
    int allocContext;
#endif
    QColor colors[NUM_COLORS];
};

//! Constructor
QwtWheel::QwtWheel(QWidget *parent): 
    QwtAbstractSlider(Qt::Horizontal, parent)
{
    initWheel();
}

#if QT_VERSION < 0x040000
QwtWheel::QwtWheel(QWidget *parent, const char *name): 
    QwtAbstractSlider(Qt::Horizontal, parent)
{
    setName(name);
    initWheel();
}
#endif

void QwtWheel::initWheel()
{
    d_data = new PrivateData;

#if QT_VERSION < 0x040000
    setWFlags(Qt::WNoAutoErase);
#endif

    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

#if QT_VERSION >= 0x040000
    setAttribute(Qt::WA_WState_OwnSizePolicy, false);
#else
    clearWState( WState_OwnSizePolicy );
#endif

    setUpdateTime(50);
}

//! Destructor
QwtWheel::~QwtWheel()  
{
#if QT_VERSION < 0x040000
    if ( d_data->allocContext )
        QColor::destroyAllocContext( d_data->allocContext );
#endif
    delete d_data;
}

//! Set up the color array for the background pixmap.
void QwtWheel::setColorArray()
{
    if ( !d_data->colors ) 
        return;

#if QT_VERSION < 0x040000
    const QColor light = colorGroup().light();
    const QColor dark = colorGroup().dark();
#else
    const QColor light = palette().color(QPalette::Light);
    const QColor dark = palette().color(QPalette::Dark);
#endif

    if ( !d_data->colors[0].isValid() ||
        d_data->colors[0] != light ||
        d_data->colors[NUM_COLORS - 1] != dark )
    {
#if QT_VERSION < 0x040000
        if ( d_data->allocContext )
            QColor::destroyAllocContext( d_data->allocContext );

        d_data->allocContext = QColor::enterAllocContext();
#endif

        d_data->colors[0] = light;
        d_data->colors[NUM_COLORS - 1] = dark;

        int dh, ds, dv, lh, ls, lv;
#if QT_VERSION < 0x040000
        d_data->colors[0].rgb(&lh, &ls, &lv);
        d_data->colors[NUM_COLORS - 1].rgb(&dh, &ds, &dv);
#else
        d_data->colors[0].getRgb(&lh, &ls, &lv);
        d_data->colors[NUM_COLORS - 1].getRgb(&dh, &ds, &dv);
#endif

        for ( int i = 1; i < NUM_COLORS - 1; ++i )
        {
            const double factor = double(i) / double(NUM_COLORS);

            d_data->colors[i].setRgb( lh + int( double(dh - lh) * factor ),
                      ls + int( double(ds - ls) * factor ),
                      lv + int( double(dv - lv) * factor ));
        }
#if QT_VERSION < 0x040000
        QColor::leaveAllocContext();
#endif
    }
}

/*!
  \brief Adjust the number of grooves in the wheel's surface.

  The number of grooves is limited to 6 <= cnt <= 50.
  Values outside this range will be clipped.
  The default value is 10.
  \param cnt Number of grooves per 360 degrees
*/
void QwtWheel::setTickCnt(int cnt)
{
    d_data->tickCnt = qwtLim( cnt, 6, 50 );
    update();
}

int QwtWheel::tickCnt() const 
{
    return d_data->tickCnt;
}

/*!
    \return mass
*/
double QwtWheel::mass() const
{
    return QwtAbstractSlider::mass();
}

/*!
  \brief Set the internal border width of the wheel.

  The internal border must not be smaller than 1
  and is limited in dependence on the wheel's size.
  Values outside the allowed range will be clipped.

  The internal border defaults to 2.
  \param w border width
*/
void QwtWheel::setInternalBorder( int w )
{
    const int d = qwtMin( width(), height() ) / 3;
    w = qwtMin( w, d );
    d_data->intBorder = qwtMax( w, 1 );
    layoutWheel();
}

int QwtWheel::internalBorder() const 
{
    return d_data->intBorder;
}

//! Draw the Wheel's background gradient
void QwtWheel::drawWheelBackground( QPainter *p, const QRect &r )
{
    p->save();

    //
    // initialize pens
    //
#if QT_VERSION < 0x040000
    const QColor light = colorGroup().light();
    const QColor dark = colorGroup().dark();
#else
    const QColor light = palette().color(QPalette::Light);
    const QColor dark = palette().color(QPalette::Dark);
#endif

    QPen lightPen;
    lightPen.setColor(light);
    lightPen.setWidth(d_data->intBorder);

    QPen darkPen;
    darkPen.setColor(dark);
    darkPen.setWidth(d_data->intBorder);

    setColorArray();

    //
    // initialize auxiliary variables
    //

    const int nFields = NUM_COLORS * 13 / 10;
    const int hiPos = nFields - NUM_COLORS + 1;

    if ( orientation() == Qt::Horizontal )
    {
        const int rx = r.x();
        int ry = r.y() + d_data->intBorder;
        const int rh = r.height() - 2* d_data->intBorder;
        const int rw = r.width();
        //
        //  draw shaded background
        //
        int x1 = rx;
        for (int i = 1; i < nFields; i++ )
        {
            const int x2 = rx + (rw * i) / nFields;
            p->fillRect(x1, ry, x2-x1 + 1 ,rh, d_data->colors[qwtAbs(i-hiPos)]);
            x1 = x2 + 1;
        }
        p->fillRect(x1, ry, rw - (x1 - rx), rh, d_data->colors[NUM_COLORS - 1]);

        //
        // draw internal border
        //
        p->setPen(lightPen);
        ry = r.y() + d_data->intBorder / 2;
        p->drawLine(r.x(), ry, r.x() + r.width() , ry);

        p->setPen(darkPen);
        ry = r.y() + r.height() - (d_data->intBorder - d_data->intBorder / 2);
        p->drawLine(r.x(), ry , r.x() + r.width(), ry);
    }
    else // Qt::Vertical
    {
        int rx = r.x() + d_data->intBorder;
        const int ry = r.y();
        const int rh = r.height();
        const int rw = r.width() - 2 * d_data->intBorder;

        //
        // draw shaded background
        //
        int y1 = ry;
        for ( int i = 1; i < nFields; i++ )
        {
            const int y2 = ry + (rh * i) / nFields;
            p->fillRect(rx, y1, rw, y2-y1 + 1, d_data->colors[qwtAbs(i-hiPos)]);
            y1 = y2 + 1;
        }
        p->fillRect(rx, y1, rw, rh - (y1 - ry), d_data->colors[NUM_COLORS - 1]);

        //
        //  draw internal borders
        //
        p->setPen(lightPen);
        rx = r.x() + d_data->intBorder / 2;
        p->drawLine(rx, r.y(), rx, r.y() + r.height());

        p->setPen(darkPen);
        rx = r.x() + r.width() - (d_data->intBorder - d_data->intBorder / 2);
        p->drawLine(rx, r.y(), rx , r.y() + r.height());
    }

    p->restore();
}


/*!
  \brief Set the total angle which the wheel can be turned.

  One full turn of the wheel corresponds to an angle of
  360 degrees. A total angle of n*360 degrees means
  that the wheel has to be turned n times around its axis
  to get from the minimum value to the maximum value.

  The default setting of the total angle is 360 degrees.
  \param angle total angle in degrees
*/
void QwtWheel::setTotalAngle(double angle)
{
    if ( angle < 0.0 )
        angle = 0.0;

    d_data->totalAngle = angle;
    update();
}

double QwtWheel::totalAngle() const 
{
    return d_data->totalAngle;
}

/*!
  \brief Set the wheel's orientation.
  \param o Orientation. Allowed values are
           Qt::Horizontal and Qt::Vertical.
   Defaults to Qt::Horizontal.
  \sa QwtAbstractSlider::orientation()
*/
void QwtWheel::setOrientation(Qt::Orientation o)
{
    if ( orientation() == o )
        return;

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
    layoutWheel();
}

/*!
  \brief Specify the visible portion of the wheel.

  You may use this function for fine-tuning the appearance of
  the wheel. The default value is 175 degrees. The value is
  limited from 10 to 175 degrees.
  \param angle Visible angle in degrees
*/
void QwtWheel::setViewAngle(double angle)
{
    d_data->viewAngle = qwtLim( angle, 10.0, 175.0 );
    update();
}

double QwtWheel::viewAngle() const 
{
    return d_data->viewAngle;
}

/*!
  \brief Redraw the wheel
  \param p painter
  \param r contents rectangle
*/
void QwtWheel::drawWheel( QPainter *p, const QRect &r )
{
    //
    // draw background gradient
    //
    drawWheelBackground( p, r );

    if ( maxValue() == minValue() || d_data->totalAngle == 0.0 )
        return;

#if QT_VERSION < 0x040000
    const QColor light = colorGroup().light();
    const QColor dark = colorGroup().dark();
#else
    const QColor light = palette().color(QPalette::Light);
    const QColor dark = palette().color(QPalette::Dark);
#endif

    const double sign = (minValue() < maxValue()) ? 1.0 : -1.0;
    double cnvFactor = qwtAbs(d_data->totalAngle / (maxValue() - minValue()));
    const double halfIntv = 0.5 * d_data->viewAngle / cnvFactor;
    const double loValue = value() - halfIntv;
    const double hiValue = value() + halfIntv;
    const double tickWidth = 360.0 / double(d_data->tickCnt) / cnvFactor;
    const double sinArc = sin(d_data->viewAngle * M_PI / 360.0);
    cnvFactor *= M_PI / 180.0;


    //
    // draw grooves
    //
    if ( orientation() == Qt::Horizontal )
    {
        const double halfSize = double(r.width()) * 0.5;

        int l1 = r.y() + d_data->intBorder;
        int l2 = r.y() + r.height() - d_data->intBorder - 1;

        // draw one point over the border if border > 1
        if ( d_data->intBorder > 1 )
        {
            l1 --;
            l2 ++;
        }

        const int maxpos = r.x() + r.width() - 2;
        const int minpos = r.x() + 2;

        //
        // draw tick marks
        //
        for ( double tickValue = ceil(loValue / tickWidth) * tickWidth;
            tickValue < hiValue; tickValue += tickWidth )
        {
            //
            //  calculate position
            //
            const int tickPos = r.x() + r.width()
                - int( halfSize
                    * (sinArc + sign *  sin((tickValue - value()) * cnvFactor))
                    / sinArc);
            //
            // draw vertical line
            //
            if ( (tickPos <= maxpos) && (tickPos > minpos) )
            {
                p->setPen(dark);
                p->drawLine(tickPos -1 , l1, tickPos - 1,  l2 );  
                p->setPen(light);
                p->drawLine(tickPos, l1, tickPos, l2);  
            }
        }
    }
    else if ( orientation() == Qt::Vertical )
    {
        const double halfSize = double(r.height()) * 0.5;

        int l1 = r.x() + d_data->intBorder;
        int l2 = r.x() + r.width() - d_data->intBorder - 1;

        if ( d_data->intBorder > 1 )
        {
            l1--;
            l2++;
        }

        const int maxpos = r.y() + r.height() - 2;
        const int minpos = r.y() + 2;

        //
        // draw tick marks
        //
        for ( double tickValue = ceil(loValue / tickWidth) * tickWidth;
            tickValue < hiValue; tickValue += tickWidth )
        {

            //
            // calculate position
            //
            const int tickPos = r.y() + int( halfSize *
                (sinArc + sign * sin((tickValue - value()) * cnvFactor))
                / sinArc);

            //
            //  draw horizontal line
            //
            if ( (tickPos <= maxpos) && (tickPos > minpos) )
            {
                p->setPen(dark);
                p->drawLine(l1, tickPos - 1 ,l2, tickPos - 1);  
                p->setPen(light);
                p->drawLine(l1, tickPos, l2, tickPos);  
            }
        }
    }
}


//! Determine the value corresponding to a specified point
double QwtWheel::getValue( const QPoint &p )
{
    // The reference position is arbitrary, but the
    // sign of the offset is important
    int w, dx;
    if ( orientation() == Qt::Vertical )
    {
        w = d_data->sliderRect.height();
        dx = d_data->sliderRect.y() - p.y();
    }
    else
    {
        w = d_data->sliderRect.width();
        dx = p.x() - d_data->sliderRect.x();
    }

    // w pixels is an arc of viewAngle degrees,
    // so we convert change in pixels to change in angle
    const double ang = dx * d_data->viewAngle / w;

    // value range maps to totalAngle degrees,
    // so convert the change in angle to a change in value
    const double val = ang * ( maxValue() - minValue() ) / d_data->totalAngle;

    // Note, range clamping and rasterizing to step is automatically
    // handled by QwtAbstractSlider, so we simply return the change in value
    return val;
}

//! Qt Resize Event
void QwtWheel::resizeEvent(QResizeEvent *)
{
    layoutWheel( false );
}

//! Recalculate the slider's geometry and layout based on
//  the current rect and fonts.
//  \param update_geometry  notify the layout system and call update
//         to redraw the scale
void QwtWheel::layoutWheel( bool update_geometry )
{
    const QRect r = this->rect();
    d_data->sliderRect.setRect(r.x() + d_data->borderWidth, r.y() + d_data->borderWidth,
        r.width() - 2*d_data->borderWidth, r.height() - 2*d_data->borderWidth);

    if ( update_geometry )
    {
        updateGeometry();
        update();
    }
}

//! Qt Paint Event
void QwtWheel::paintEvent(QPaintEvent *e)
{
    // Use double-buffering
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

//! Redraw panel and wheel
void QwtWheel::draw(QPainter *painter, const QRect&)
{
    qDrawShadePanel( painter, rect().x(), rect().y(),
        rect().width(), rect().height(),
#if QT_VERSION < 0x040000
        colorGroup(), 
#else
        palette(), 
#endif
        true, d_data->borderWidth );

    drawWheel( painter, d_data->sliderRect );

    if ( hasFocus() )
        QwtPainter::drawFocusRect(painter, this);
}

//! Notify value change 
void QwtWheel::valueChange()
{
    QwtAbstractSlider::valueChange();
    update();
}


/*!
  \brief Determine the scrolling mode and direction corresponding
         to a specified point
  \param p point
  \param scrollMode scrolling mode
  \param direction direction
*/
void QwtWheel::getScrollMode( const QPoint &p, int &scrollMode, int &direction)
{
    if ( d_data->sliderRect.contains(p) )
        scrollMode = ScrMouse;
    else
        scrollMode = ScrNone;

    direction = 0;
}

/*!
  \brief Set the mass of the wheel

  Assigning a mass turns the wheel into a flywheel.
  \param val the wheel's mass
*/
void QwtWheel::setMass(double val)
{
    QwtAbstractSlider::setMass(val);
}

/*!
  \brief Set the width of the wheel

  Corresponds to the wheel height for horizontal orientation,
  and the wheel width for vertical orientation.
  \param w the wheel's width
*/
void QwtWheel::setWheelWidth(int w)
{
    d_data->wheelWidth = w;
    layoutWheel();
}

/*!
  \return a size hint
*/
QSize QwtWheel::sizeHint() const
{
    return minimumSizeHint();
}

/*!
  \brief Return a minimum size hint
  \warning The return value is based on the wheel width.
*/
QSize QwtWheel::minimumSizeHint() const
{
    QSize sz( 3*d_data->wheelWidth + 2*d_data->borderWidth,
    d_data->wheelWidth + 2*d_data->borderWidth );
    if ( orientation() != Qt::Horizontal )
        sz.transpose();
    return sz;
}

/*!
  \brief Call update() when the palette changes
*/
void QwtWheel::paletteChange( const QPalette& )
{
    update();
}

