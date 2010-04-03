/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef QWT_ABSTRACT_SCALE_DRAW_H
#define QWT_ABSTRACT_SCALE_DRAW_H

#include "qwt_global.h"
#include "qwt_scale_div.h"
#include "qwt_text.h"


#if QT_VERSION < 0x040000
class QColorGroup;
#else
class QPalette;
#endif
class QPainter;
class QFont;
class QwtScaleTransformation;
class QwtScaleMap;

/*!
  \brief A abstract base class for drawing scales

  QwtAbstractScaleDraw can be used to draw linear or logarithmic scales.

  After a scale division has been specified as a QwtScaleDiv object
  using QwtAbstractScaleDraw::setScaleDiv(const QwtScaleDiv &s),
  the scale can be drawn with the QwtAbstractScaleDraw::draw() member.
*/
class QWT_EXPORT QwtAbstractScaleDraw
{
public:

     /*!
        Components of a scale

        - Backbone
        - Ticks
        - Labels

        \sa QwtAbstractScaleDraw::enableComponent, 
            QwtAbstractScaleDraw::hasComponent
    */

    enum ScaleComponent
    { 
        Backbone = 1,
        Ticks = 2,
        Labels = 4
    };
 
    QwtAbstractScaleDraw();
    QwtAbstractScaleDraw( const QwtAbstractScaleDraw & );
    virtual ~QwtAbstractScaleDraw();

    QwtAbstractScaleDraw &operator=(const QwtAbstractScaleDraw &);
    
    void setScaleDiv(const QwtScaleDiv &s);
    const QwtScaleDiv& scaleDiv() const;

    void setTransformation(QwtScaleTransformation *);
    const QwtScaleMap &map() const;

    void enableComponent(ScaleComponent, bool enable = true);
    bool hasComponent(ScaleComponent) const;

    void setTickLength(QwtScaleDiv::TickType, int length);
    int tickLength(QwtScaleDiv::TickType) const;
    int majTickLength() const;

    void setSpacing(int margin);
    int spacing() const;
        
#if QT_VERSION < 0x040000
    virtual void draw(QPainter *, const QColorGroup &) const;
#else
    virtual void draw(QPainter *, const QPalette &) const;
#endif

    virtual QwtText label(double) const;

    /*!  
      Calculate the extent 

      The extent is the distcance from the baseline to the outermost
      pixel of the scale draw in opposite to its orientation.
      It is at least minimumExtent() pixels.
 
      \sa setMinimumExtent(), minimumExtent()
    */
    virtual int extent(const QPen &, const QFont &) const = 0;

    void setMinimumExtent(int);
    int minimumExtent() const;

    QwtScaleMap &scaleMap();

protected:
    /*!
       Draw a tick
  
       \param painter Painter
       \param value Value of the tick
       \param len Lenght of the tick

       \sa drawBackbone(), drawLabel()
    */  
    virtual void drawTick(QPainter *painter, double value, int len) const = 0;

    /*!
      Draws the baseline of the scale
      \param painter Painter

      \sa drawTick(), drawLabel()
    */
    virtual void drawBackbone(QPainter *painter) const = 0;

    /*!  
        Draws the label for a major scale tick
    
        \param painter Painter
        \param value Value

        \sa drawTick, drawBackbone
    */ 
    virtual void drawLabel(QPainter *painter, double value) const = 0;

    void invalidateCache();
    const QwtText &tickLabel(const QFont &, double value) const;

private:
    int operator==(const QwtAbstractScaleDraw &) const;
    int operator!=(const QwtAbstractScaleDraw &) const;

    class PrivateData;
    PrivateData *d_data;
};

#endif
