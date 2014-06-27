/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef QWT_DIAL_H
#define QWT_DIAL_H 1

#include "qwt_global.h"
#include "qwt_abstract_slider.h"
#include "qwt_abstract_scale_draw.h"
#include <qframe.h>
#include <qpalette.h>

class QwtDialNeedle;
class QwtRoundScaleDraw;

/*!
  \brief QwtDial class provides a rounded range control.

  QwtDial is intended as base class for dial widgets like
  speedometers, compass widgets, clocks ...

  \image html dials2.png

  A dial contains a scale and a needle indicating the current value
  of the dial. Depending on Mode one of them is fixed and the
  other is rotating. If not isReadOnly() the
  dial can be rotated by dragging the mouse or using keyboard inputs
  (see QwtAbstractSlider::keyPressEvent()). A dial might be wrapping, what means
  a rotation below/above one limit continues on the other limit (f.e compass).
  The scale might cover any arc of the dial, its values are related to
  the origin() of the dial.

  Often dials have to be updated very often according to values from external
  devices. For these high refresh rates QwtDial caches as much as possible.
  For derived classes it might be necessary to clear these caches manually
  according to attribute changes using invalidateCache().

  \sa QwtCompass, QwtAnalogClock, QwtDialNeedle
  \note The controls and dials examples shows different types of dials.
  \note QDial is more similar to QwtKnob than to QwtDial
*/

class QWT_EXPORT QwtDial: public QwtAbstractSlider
{
    Q_OBJECT

    Q_ENUMS( Shadow Mode Direction )

    Q_PROPERTY( int lineWidth READ lineWidth WRITE setLineWidth )
    Q_PROPERTY( Shadow frameShadow READ frameShadow WRITE setFrameShadow )
    Q_PROPERTY( Mode mode READ mode WRITE setMode )
    Q_PROPERTY( double origin READ origin WRITE setOrigin )
    Q_PROPERTY( double minScaleArc READ minScaleArc WRITE setMinScaleArc )
    Q_PROPERTY( double maxScaleArc READ maxScaleArc WRITE setMaxScaleArc )

public:

    /*!
        \brief Frame shadow

         Unfortunately it is not possible to use QFrame::Shadow
         as a property of a widget that is not derived from QFrame.
         The following enum is made for the designer only. It is safe
         to use QFrame::Shadow instead.
     */
    enum Shadow
    {
        //! QFrame::Plain
        Plain = QFrame::Plain,

        //! QFrame::Raised
        Raised = QFrame::Raised,

        //! QFrame::Sunken
        Sunken = QFrame::Sunken
    };

    //! Mode controlling whether the needle or the scale is rotating
    enum Mode
    {
        //! The needle is rotating
        RotateNeedle,

        //! The needle is fixed, the scales are rotating
        RotateScale
    };

    explicit QwtDial( QWidget *parent = NULL );
    virtual ~QwtDial();

    void setFrameShadow( Shadow );
    Shadow frameShadow() const;

    void setLineWidth( int );
    int lineWidth() const;

    void setMode( Mode );
    Mode mode() const;

    void setScaleArc( double min, double max );

    void setMinScaleArc( double min );
    double minScaleArc() const;

    void setMaxScaleArc( double min );
    double maxScaleArc() const;

    virtual void setOrigin( double );
    double origin() const;

    void setNeedle( QwtDialNeedle * );
    const QwtDialNeedle *needle() const;
    QwtDialNeedle *needle();

    QRect boundingRect() const;
    QRect innerRect() const;

    virtual QRect scaleInnerRect() const;

    virtual QSize sizeHint() const;
    virtual QSize minimumSizeHint() const;

    void setScaleDraw( QwtRoundScaleDraw * );

    QwtRoundScaleDraw *scaleDraw();
    const QwtRoundScaleDraw *scaleDraw() const;

protected:
    virtual void wheelEvent( QWheelEvent * );
    virtual void paintEvent( QPaintEvent * );
    virtual void changeEvent( QEvent * );

    virtual void drawFrame( QPainter *p );
    virtual void drawContents( QPainter * ) const;
    virtual void drawFocusIndicator( QPainter * ) const;

    void invalidateCache();

    virtual void drawScale( QPainter *, 
        const QPointF &center, double radius ) const;

    virtual void drawScaleContents( QPainter *painter, 
        const QPointF &center, double radius ) const;

    virtual void drawNeedle( QPainter *, const QPointF &,
        double radius, double direction, QPalette::ColorGroup ) const;

    virtual double scrolledTo( const QPoint & ) const;
    virtual bool isScrollPosition( const QPoint & ) const;

    virtual void sliderChange();
    virtual void scaleChange();

private:
    void setAngleRange( double angle, double span );
    void drawNeedle( QPainter * ) const;

    class PrivateData;
    PrivateData *d_data;
};

#endif
