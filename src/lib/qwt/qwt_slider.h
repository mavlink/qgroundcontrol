/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

// vim: expandtab

#ifndef QWT_SLIDER_H
#define QWT_SLIDER_H

#include "qwt_global.h"
#include "qwt_abstract_scale.h"
#include "qwt_abstract_slider.h"

class QwtScaleDraw;

/*!
  \brief The Slider Widget

  QwtSlider is a slider widget which operates on an interval
  of type double. QwtSlider supports different layouts as
  well as a scale.

  \image html sliders.png

  \sa QwtAbstractSlider and QwtAbstractScale for the descriptions
      of the inherited members.
*/

class QWT_EXPORT QwtSlider : public QwtAbstractSlider, public QwtAbstractScale
{
    Q_OBJECT
    Q_ENUMS( ScalePos )
    Q_ENUMS( BGSTYLE )
    Q_PROPERTY( ScalePos scalePosition READ scalePosition
        WRITE setScalePosition )
    Q_PROPERTY( BGSTYLE bgStyle READ bgStyle WRITE setBgStyle )
    Q_PROPERTY( int thumbLength READ thumbLength WRITE setThumbLength )
    Q_PROPERTY( int thumbWidth READ thumbWidth WRITE setThumbWidth )
    Q_PROPERTY( int borderWidth READ borderWidth WRITE setBorderWidth )
 
public:

    /*! 
      Scale position. QwtSlider tries to enforce valid combinations of its
      orientation and scale position:
      - Qt::Horizonal combines with NoScale, TopScale and BottomScale
      - Qt::Vertical combines with NoScale, LeftScale and RightScale

      \sa QwtSlider::QwtSlider
     */
    enum ScalePos 
    { 
        NoScale, 

        LeftScale, 
        RightScale, 
        TopScale, 
        BottomScale 
    };

    /*! 
      Background style.
      \sa QwtSlider::QwtSlider
     */
    enum BGSTYLE 
    { 
        BgTrough = 0x1, 
        BgSlot = 0x2, 
        BgBoth = BgTrough | BgSlot
    };

    explicit QwtSlider(QWidget *parent,
          Qt::Orientation = Qt::Horizontal,
          ScalePos = NoScale, BGSTYLE bgStyle = BgTrough);
#if QT_VERSION < 0x040000
    explicit QwtSlider(QWidget *parent, const char *name);
#endif
    
    virtual ~QwtSlider();

    virtual void setOrientation(Qt::Orientation); 

    void setBgStyle(BGSTYLE);
    BGSTYLE bgStyle() const;
    
    void setScalePosition(ScalePos s);
    ScalePos scalePosition() const;

    int thumbLength() const;
    int thumbWidth() const;
    int borderWidth() const;

    void setThumbLength(int l);
    void setThumbWidth(int w);
    void setBorderWidth(int bw);
    void setMargins(int x, int y);

    virtual QSize sizeHint() const;
    virtual QSize minimumSizeHint() const;
    
    void setScaleDraw(QwtScaleDraw *);
    const QwtScaleDraw *scaleDraw() const;

protected:
    virtual double getValue(const QPoint &p);
    virtual void getScrollMode(const QPoint &p, 
        int &scrollMode, int &direction);

    void draw(QPainter *p, const QRect& update_rect);
    virtual void drawSlider (QPainter *p, const QRect &r);
    virtual void drawThumb(QPainter *p, const QRect &, int pos);

    virtual void resizeEvent(QResizeEvent *e);
    virtual void paintEvent (QPaintEvent *e);

    virtual void valueChange();
    virtual void rangeChange();
    virtual void scaleChange();
    virtual void fontChange(const QFont &oldFont);

    void layoutSlider( bool update = true );
    int xyPosition(double v) const;

    QwtScaleDraw *scaleDraw();

private:
    void initSlider(Qt::Orientation, ScalePos scalePos, BGSTYLE bgStyle);

    class PrivateData;
    PrivateData *d_data;
};

#endif
