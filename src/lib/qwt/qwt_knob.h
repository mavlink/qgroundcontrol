/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef QWT_KNOB_H
#define QWT_KNOB_H

#include "qwt_global.h"
#include "qwt_abstract_slider.h"
#include "qwt_abstract_scale.h"

class QwtRoundScaleDraw;

/*!
  \brief The Knob Widget

  The QwtKnob widget imitates look and behaviour of a volume knob on a radio.
  It contains a scale around the knob which is set up automatically or can
  be configured manually (see QwtAbstractScale).
  Automatic scrolling is enabled when the user presses a mouse
  button on the scale. For a description of signals, slots and other
  members, see QwtAbstractSlider.

  \image html knob.png
  \sa   QwtAbstractSlider and QwtAbstractScale for the descriptions
    of the inherited members.
*/

class QWT_EXPORT QwtKnob : public QwtAbstractSlider, public QwtAbstractScale
{
    Q_OBJECT 
    Q_ENUMS (Symbol)
    Q_PROPERTY( int knobWidth READ knobWidth WRITE setKnobWidth )
    Q_PROPERTY( int borderWidth READ borderWidth WRITE setBorderWidth )
    Q_PROPERTY( double totalAngle READ totalAngle WRITE setTotalAngle )
    Q_PROPERTY( Symbol symbol READ symbol WRITE setSymbol )

public:
    /*!
        Symbol
        \sa QwtKnob::QwtKnob()
    */

    enum Symbol { Line, Dot };

    explicit QwtKnob(QWidget* parent = NULL);
#if QT_VERSION < 0x040000
    explicit QwtKnob(QWidget* parent, const char *name);
#endif
    virtual ~QwtKnob();

    void setKnobWidth(int w);
    int knobWidth() const;

    void setTotalAngle (double angle);
    double totalAngle() const;

    void setBorderWidth(int bw);
    int borderWidth() const;

    void setSymbol(Symbol);
    Symbol symbol() const;

    virtual QSize sizeHint() const;
    virtual QSize minimumSizeHint() const;
    
    void setScaleDraw(QwtRoundScaleDraw *);
    const QwtRoundScaleDraw *scaleDraw() const;
    QwtRoundScaleDraw *scaleDraw();

protected:
    virtual void paintEvent(QPaintEvent *e);
    virtual void resizeEvent(QResizeEvent *e);

    void draw(QPainter *p, const QRect& ur);
    void drawKnob(QPainter *p, const QRect &r);
    void drawMarker(QPainter *p, double arc, const QColor &c);

private:
    void initKnob();
    void layoutKnob( bool update = true );
    double getValue(const QPoint &p);
    void getScrollMode( const QPoint &p, int &scrollMode, int &direction );
    void recalcAngle();
    
    virtual void valueChange();
    virtual void rangeChange();
    virtual void scaleChange();
    virtual void fontChange(const QFont &oldFont);

    class PrivateData;
    PrivateData *d_data;
};

#endif
