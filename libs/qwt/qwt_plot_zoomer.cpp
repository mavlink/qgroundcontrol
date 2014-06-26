/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include "qwt_plot_zoomer.h"
#include "qwt_plot.h"
#include "qwt_scale_div.h"
#include "qwt_picker_machine.h"
#include <qalgorithms.h>

class QwtPlotZoomer::PrivateData
{
public:
    uint zoomRectIndex;
    QStack<QRectF> zoomStack;

    int maxStackDepth;
};

/*!
  \brief Create a zoomer for a plot canvas.

  The zoomer is set to those x- and y-axis of the parent plot of the
  canvas that are enabled. If both or no x-axis are enabled, the picker
  is set to QwtPlot::xBottom. If both or no y-axis are
  enabled, it is set to QwtPlot::yLeft.

  The zoomer is initialized with a QwtPickerDragRectMachine,
  the tracker mode is set to QwtPicker::ActiveOnly and the rubber band
  is set to QwtPicker::RectRubberBand

  \param canvas Plot canvas to observe, also the parent object
  \param doReplot Call QwtPlot::replot() for the attached plot before initializing
                  the zoomer with its scales. This might be necessary,
                  when the plot is in a state with pending scale changes.

  \sa QwtPlot::autoReplot(), QwtPlot::replot(), setZoomBase()
*/
QwtPlotZoomer::QwtPlotZoomer( QWidget *canvas, bool doReplot ):
    QwtPlotPicker( canvas )
{
    if ( canvas )
        init( doReplot );
}

/*!
  \brief Create a zoomer for a plot canvas.

  The zoomer is initialized with a QwtPickerDragRectMachine,
  the tracker mode is set to QwtPicker::ActiveOnly and the rubber band
  is set to QwtPicker;;RectRubberBand

  \param xAxis X axis of the zoomer
  \param yAxis Y axis of the zoomer
  \param canvas Plot canvas to observe, also the parent object
  \param doReplot Call QwtPlot::replot() for the attached plot before initializing
                  the zoomer with its scales. This might be necessary,
                  when the plot is in a state with pending scale changes.

  \sa QwtPlot::autoReplot(), QwtPlot::replot(), setZoomBase()
*/

QwtPlotZoomer::QwtPlotZoomer( int xAxis, int yAxis,
        QWidget *canvas, bool doReplot ):
    QwtPlotPicker( xAxis, yAxis, canvas )
{
    if ( canvas )
        init( doReplot );
}

//! Init the zoomer, used by the constructors
void QwtPlotZoomer::init( bool doReplot )
{
    d_data = new PrivateData;

    d_data->maxStackDepth = -1;

    setTrackerMode( ActiveOnly );
    setRubberBand( RectRubberBand );
    setStateMachine( new QwtPickerDragRectMachine() );

    if ( doReplot && plot() )
        plot()->replot();

    setZoomBase( scaleRect() );
}

QwtPlotZoomer::~QwtPlotZoomer()
{
    delete d_data;
}

/*!
  \brief Limit the number of recursive zoom operations to depth.

  A value of -1 set the depth to unlimited, 0 disables zooming.
  If the current zoom rectangle is below depth, the plot is unzoomed.

  \param depth Maximum for the stack depth
  \sa maxStackDepth()
  \note depth doesn't include the zoom base, so zoomStack().count() might be
              maxStackDepth() + 1.
*/
void QwtPlotZoomer::setMaxStackDepth( int depth )
{
    d_data->maxStackDepth = depth;

    if ( depth >= 0 )
    {
        // unzoom if the current depth is below d_data->maxStackDepth

        const int zoomOut =
            int( d_data->zoomStack.count() ) - 1 - depth; // -1 for the zoom base

        if ( zoomOut > 0 )
        {
            zoom( -zoomOut );
            for ( int i = int( d_data->zoomStack.count() ) - 1;
                i > int( d_data->zoomRectIndex ); i-- )
            {
                ( void )d_data->zoomStack.pop(); // remove trailing rects
            }
        }
    }
}

/*!
  \return Maximal depth of the zoom stack.
  \sa setMaxStackDepth()
*/
int QwtPlotZoomer::maxStackDepth() const
{
    return d_data->maxStackDepth;
}

/*!
  \return The zoom stack. zoomStack()[0] is the zoom base,
          zoomStack()[1] the first zoomed rectangle.

  \sa setZoomStack(), zoomRectIndex()
*/
const QStack<QRectF> &QwtPlotZoomer::zoomStack() const
{
    return d_data->zoomStack;
}

/*!
  \return Initial rectangle of the zoomer
  \sa setZoomBase(), zoomRect()
*/
QRectF QwtPlotZoomer::zoomBase() const
{
    return d_data->zoomStack[0];
}

/*!
  Reinitialized the zoom stack with scaleRect() as base.

  \param doReplot Call QwtPlot::replot() for the attached plot before initializing
                  the zoomer with its scales. This might be necessary,
                  when the plot is in a state with pending scale changes.

  \sa zoomBase(), scaleRect() QwtPlot::autoReplot(), QwtPlot::replot().
*/
void QwtPlotZoomer::setZoomBase( bool doReplot )
{
    QwtPlot *plt = plot();
    if ( plt == NULL )
        return;

    if ( doReplot )
        plt->replot();

    d_data->zoomStack.clear();
    d_data->zoomStack.push( scaleRect() );
    d_data->zoomRectIndex = 0;

    rescale();
}

/*!
  \brief Set the initial size of the zoomer.

  base is united with the current scaleRect() and the zoom stack is
  reinitialized with it as zoom base. plot is zoomed to scaleRect().

  \param base Zoom base

  \sa zoomBase(), scaleRect()
*/
void QwtPlotZoomer::setZoomBase( const QRectF &base )
{
    const QwtPlot *plt = plot();
    if ( !plt )
        return;

    const QRectF sRect = scaleRect();
    const QRectF bRect = base | sRect;

    d_data->zoomStack.clear();
    d_data->zoomStack.push( bRect );
    d_data->zoomRectIndex = 0;

    if ( base != sRect )
    {
        d_data->zoomStack.push( sRect );
        d_data->zoomRectIndex++;
    }

    rescale();
}

/*!
  \return Rectangle at the current position on the zoom stack.
  \sa zoomRectIndex(), scaleRect().
*/
QRectF QwtPlotZoomer::zoomRect() const
{
    return d_data->zoomStack[d_data->zoomRectIndex];
}

/*!
  \return Index of current position of zoom stack.
*/
uint QwtPlotZoomer::zoomRectIndex() const
{
    return d_data->zoomRectIndex;
}

/*!
  \brief Zoom in

  Clears all rectangles above the current position of the
  zoom stack and pushes the normalized rectangle on it.

  \note If the maximal stack depth is reached, zoom is ignored.
  \note The zoomed signal is emitted.
*/

void QwtPlotZoomer::zoom( const QRectF &rect )
{
    if ( d_data->maxStackDepth >= 0 &&
            int( d_data->zoomRectIndex ) >= d_data->maxStackDepth )
    {
        return;
    }

    const QRectF zoomRect = rect.normalized();
    if ( zoomRect != d_data->zoomStack[d_data->zoomRectIndex] )
    {
        for ( uint i = int( d_data->zoomStack.count() ) - 1;
           i > d_data->zoomRectIndex; i-- )
        {
            ( void )d_data->zoomStack.pop();
        }

        d_data->zoomStack.push( zoomRect );
        d_data->zoomRectIndex++;

        rescale();

        Q_EMIT zoomed( zoomRect );
    }
}

/*!
  \brief Zoom in or out

  Activate a rectangle on the zoom stack with an offset relative
  to the current position. Negative values of offset will zoom out,
  positive zoom in. A value of 0 zooms out to the zoom base.

  \param offset Offset relative to the current position of the zoom stack.
  \note The zoomed signal is emitted.
  \sa zoomRectIndex()
*/
void QwtPlotZoomer::zoom( int offset )
{
    if ( offset == 0 )
        d_data->zoomRectIndex = 0;
    else
    {
        int newIndex = d_data->zoomRectIndex + offset;
        newIndex = qMax( 0, newIndex );
        newIndex = qMin( int( d_data->zoomStack.count() ) - 1, newIndex );

        d_data->zoomRectIndex = uint( newIndex );
    }

    rescale();

    Q_EMIT zoomed( zoomRect() );
}

/*!
  \brief Assign a zoom stack

  In combination with other types of navigation it might be useful to
  modify to manipulate the complete zoom stack.

  \param zoomStack New zoom stack
  \param zoomRectIndex Index of the current position of zoom stack.
                       In case of -1 the current position is at the top
                       of the stack.

  \note The zoomed signal might be emitted.
  \sa zoomStack(), zoomRectIndex()
*/
void QwtPlotZoomer::setZoomStack(
    const QStack<QRectF> &zoomStack, int zoomRectIndex )
{
    if ( zoomStack.isEmpty() )
        return;

    if ( d_data->maxStackDepth >= 0 &&
        int( zoomStack.count() ) > d_data->maxStackDepth )
    {
        return;
    }

    if ( zoomRectIndex < 0 || zoomRectIndex > int( zoomStack.count() ) )
        zoomRectIndex = zoomStack.count() - 1;

    const bool doRescale = zoomStack[zoomRectIndex] != zoomRect();

    d_data->zoomStack = zoomStack;
    d_data->zoomRectIndex = uint( zoomRectIndex );

    if ( doRescale )
    {
        rescale();
        Q_EMIT zoomed( zoomRect() );
    }
}

/*!
  Adjust the observed plot to zoomRect()

  \note Initiates QwtPlot::replot()
*/

void QwtPlotZoomer::rescale()
{
    QwtPlot *plt = plot();
    if ( !plt )
        return;

    const QRectF &rect = d_data->zoomStack[d_data->zoomRectIndex];
    if ( rect != scaleRect() )
    {
        const bool doReplot = plt->autoReplot();
        plt->setAutoReplot( false );

        double x1 = rect.left();
        double x2 = rect.right();
        if ( !plt->axisScaleDiv( xAxis() ).isIncreasing() )
            qSwap( x1, x2 );

        plt->setAxisScale( xAxis(), x1, x2 );

        double y1 = rect.top();
        double y2 = rect.bottom();
        if ( !plt->axisScaleDiv( yAxis() ).isIncreasing() )
            qSwap( y1, y2 );

        plt->setAxisScale( yAxis(), y1, y2 );

        plt->setAutoReplot( doReplot );

        plt->replot();
    }
}

/*!
  Reinitialize the axes, and set the zoom base to their scales.

  \param xAxis X axis
  \param yAxis Y axis
*/

void QwtPlotZoomer::setAxis( int xAxis, int yAxis )
{
    if ( xAxis != QwtPlotPicker::xAxis() || yAxis != QwtPlotPicker::yAxis() )
    {
        QwtPlotPicker::setAxis( xAxis, yAxis );
        setZoomBase( scaleRect() );
    }
}

/*!
   Qt::MidButton zooms out one position on the zoom stack,
   Qt::RightButton to the zoom base.

   Changes the current position on the stack, but doesn't pop
   any rectangle.

   \note The mouse events can be changed, using
         QwtEventPattern::setMousePattern: 2, 1
*/
void QwtPlotZoomer::widgetMouseReleaseEvent( QMouseEvent *me )
{
    if ( mouseMatch( MouseSelect2, me ) )
        zoom( 0 );
    else if ( mouseMatch( MouseSelect3, me ) )
        zoom( -1 );
    else if ( mouseMatch( MouseSelect6, me ) )
        zoom( +1 );
    else
        QwtPlotPicker::widgetMouseReleaseEvent( me );
}

/*!
   Qt::Key_Plus zooms in, Qt::Key_Minus zooms out one position on the
   zoom stack, Qt::Key_Escape zooms out to the zoom base.

   Changes the current position on the stack, but doesn't pop
   any rectangle.

   \note The keys codes can be changed, using
         QwtEventPattern::setKeyPattern: 3, 4, 5
*/

void QwtPlotZoomer::widgetKeyPressEvent( QKeyEvent *ke )
{
    if ( !isActive() )
    {
        if ( keyMatch( KeyUndo, ke ) )
            zoom( -1 );
        else if ( keyMatch( KeyRedo, ke ) )
            zoom( +1 );
        else if ( keyMatch( KeyHome, ke ) )
            zoom( 0 );
    }

    QwtPlotPicker::widgetKeyPressEvent( ke );
}

/*!
  Move the current zoom rectangle.

  \param dx X offset
  \param dy Y offset

  \note The changed rectangle is limited by the zoom base
*/
void QwtPlotZoomer::moveBy( double dx, double dy )
{
    const QRectF &rect = d_data->zoomStack[d_data->zoomRectIndex];
    moveTo( QPointF( rect.left() + dx, rect.top() + dy ) );
}

/*!
  Move the the current zoom rectangle.

  \param pos New position

  \sa QRectF::moveTo()
  \note The changed rectangle is limited by the zoom base
*/
void QwtPlotZoomer::moveTo( const QPointF &pos )
{
    double x = pos.x();
    double y = pos.y();

    if ( x < zoomBase().left() )
        x = zoomBase().left();
    if ( x > zoomBase().right() - zoomRect().width() )
        x = zoomBase().right() - zoomRect().width();

    if ( y < zoomBase().top() )
        y = zoomBase().top();
    if ( y > zoomBase().bottom() - zoomRect().height() )
        y = zoomBase().bottom() - zoomRect().height();

    if ( x != zoomRect().left() || y != zoomRect().top() )
    {
        d_data->zoomStack[d_data->zoomRectIndex].moveTo( x, y );
        rescale();
    }
}

/*!
  \brief Check and correct a selected rectangle

  Reject rectangles with a height or width < 2, otherwise
  expand the selected rectangle to a minimum size of 11x11
  and accept it.

  \return true If the rectangle is accepted, or has been changed
          to an accepted one.
*/

bool QwtPlotZoomer::accept( QPolygon &pa ) const
{
    if ( pa.count() < 2 )
        return false;

    QRect rect = QRect( pa[0], pa[int( pa.count() ) - 1] );
    rect = rect.normalized();

    const int minSize = 2;
    if ( rect.width() < minSize && rect.height() < minSize )
        return false;

    const int minZoomSize = 11;

    const QPoint center = rect.center();
    rect.setSize( rect.size().expandedTo( QSize( minZoomSize, minZoomSize ) ) );
    rect.moveCenter( center );

    pa.resize( 2 );
    pa[0] = rect.topLeft();
    pa[1] = rect.bottomRight();

    return true;
}

/*!
  \brief Limit zooming by a minimum rectangle

  \return zoomBase().width() / 10e4, zoomBase().height() / 10e4
*/
QSizeF QwtPlotZoomer::minZoomSize() const
{
    return QSizeF( d_data->zoomStack[0].width() / 10e4,
        d_data->zoomStack[0].height() / 10e4 );
}

/*!
  Rejects selections, when the stack depth is too deep, or
  the zoomed rectangle is minZoomSize().

  \sa minZoomSize(), maxStackDepth()
*/
void QwtPlotZoomer::begin()
{
    if ( d_data->maxStackDepth >= 0 )
    {
        if ( d_data->zoomRectIndex >= uint( d_data->maxStackDepth ) )
            return;
    }

    const QSizeF minSize = minZoomSize();
    if ( minSize.isValid() )
    {
        const QSizeF sz =
            d_data->zoomStack[d_data->zoomRectIndex].size() * 0.9999;

        if ( minSize.width() >= sz.width() &&
            minSize.height() >= sz.height() )
        {
            return;
        }
    }

    QwtPlotPicker::begin();
}

/*!
  Expand the selected rectangle to minZoomSize() and zoom in
  if accepted.

  \param ok If true, complete the selection and emit selected signals
            otherwise discard the selection.

  \sa accept(), minZoomSize()
  \return True if the selection has been accepted, false otherwise
*/
bool QwtPlotZoomer::end( bool ok )
{
    ok = QwtPlotPicker::end( ok );
    if ( !ok )
        return false;

    QwtPlot *plot = QwtPlotZoomer::plot();
    if ( !plot )
        return false;

    const QPolygon &pa = selection();
    if ( pa.count() < 2 )
        return false;

    QRect rect = QRect( pa[0], pa[int( pa.count() - 1 )] );
    rect = rect.normalized();

    QRectF zoomRect = invTransform( rect ).normalized();

    const QSizeF minSize = minZoomSize();
    if ( minSize.isValid() )
    {
        const QPointF center = zoomRect.center();
        zoomRect.setSize( zoomRect.size().expandedTo( minZoomSize() ) );
        zoomRect.moveCenter( center );
    }

    zoom( zoomRect );

    return true;
}
