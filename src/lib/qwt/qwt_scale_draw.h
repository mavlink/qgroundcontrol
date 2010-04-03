/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef QWT_SCALE_DRAW_H
#define QWT_SCALE_DRAW_H

#include <qpoint.h>
#include "qwt_global.h"
#include "qwt_abstract_scale_draw.h"

/*!
  \brief A class for drawing scales

  QwtScaleDraw can be used to draw linear or logarithmic scales.
  A scale has a position, an alignment and a length, which can be specified .
  The labels can be rotated and aligned
  to the ticks using setLabelRotation() and setLabelAlignment().

  After a scale division has been specified as a QwtScaleDiv object
  using QwtAbstractScaleDraw::setScaleDiv(const QwtScaleDiv &s),
  the scale can be drawn with the QwtAbstractScaleDraw::draw() member.
*/

class QWT_EXPORT QwtScaleDraw: public QwtAbstractScaleDraw
{
public:
    /*! 
        Alignment of the scale draw
        \sa setAlignment(), alignment()
     */
    enum Alignment { BottomScale, TopScale, LeftScale, RightScale };

    QwtScaleDraw();
    QwtScaleDraw(const QwtScaleDraw &);

    virtual ~QwtScaleDraw();

    QwtScaleDraw &operator=(const QwtScaleDraw &other);

    void getBorderDistHint(const QFont &, int &start, int &end) const;
    int minLabelDist(const QFont &) const;

    int minLength(const QPen &, const QFont &) const;
    virtual int extent(const QPen &, const QFont &) const;

    void move(int x, int y);
    void move(const QPoint &);
    void setLength(int length);

    Alignment alignment() const;
    void setAlignment(Alignment);

    Qt::Orientation orientation() const;

    QPoint pos() const;
    int length() const;

#if QT_VERSION < 0x040000
    void setLabelAlignment(int);
    int labelAlignment() const;
#else
    void setLabelAlignment(Qt::Alignment);
    Qt::Alignment labelAlignment() const;
#endif

    void setLabelRotation(double rotation);
    double labelRotation() const;

    int maxLabelHeight(const QFont &) const;
    int maxLabelWidth(const QFont &) const;

    QPoint labelPosition(double val) const;

    QRect labelRect(const QFont &, double val) const;
    QSize labelSize(const QFont &, double val) const;

    QRect boundingLabelRect(const QFont &, double val) const;

protected:

#if QT_VERSION < 0x040000
    QWMatrix labelMatrix(const QPoint &, const QSize &) const;
#else   
    QMatrix labelMatrix(const QPoint &, const QSize &) const;
#endif  

    virtual void drawTick(QPainter *p, double val, int len) const;
    virtual void drawBackbone(QPainter *p) const;
    virtual void drawLabel(QPainter *p, double val) const;

private:
    void updateMap();

    class PrivateData;
    PrivateData *d_data;
};

inline void QwtScaleDraw::move(int x, int y)
{
    move(QPoint(x, y));
}

#endif
