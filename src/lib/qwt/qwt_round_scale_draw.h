/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef QWT_ROUND_SCALE_DRAW_H
#define QWT_ROUND_SCALE_DRAW_H

#include <qpoint.h>
#include "qwt_global.h"
#include "qwt_abstract_scale_draw.h"

class QPen;

/*!
  \brief A class for drawing round scales

  QwtRoundScaleDraw can be used to draw round scales.
  The circle segment can be adjusted by QwtRoundScaleDraw::setAngleRange().
  The geometry of the scale can be specified with 
  QwtRoundScaleDraw::moveCenter() and QwtRoundScaleDraw::setRadius().

  After a scale division has been specified as a QwtScaleDiv object
  using QwtAbstractScaleDraw::setScaleDiv(const QwtScaleDiv &s),
  the scale can be drawn with the QwtAbstractScaleDraw::draw() member.
*/

class QWT_EXPORT QwtRoundScaleDraw: public QwtAbstractScaleDraw
{
public:
    QwtRoundScaleDraw();
    QwtRoundScaleDraw(const QwtRoundScaleDraw &);

    virtual ~QwtRoundScaleDraw();

    QwtRoundScaleDraw &operator=(const QwtRoundScaleDraw &other);

    void setRadius(int radius);
    int radius() const;

    void moveCenter(int x, int y);
    void moveCenter(const QPoint &);
    QPoint center() const;

    void setAngleRange(double angle1, double angle2);

    virtual int extent(const QPen &, const QFont &) const;

protected:
    virtual void drawTick(QPainter *p, double val, int len) const;
    virtual void drawBackbone(QPainter *p) const;
    virtual void drawLabel(QPainter *p, double val) const;

private:
    class PrivateData;
    PrivateData *d_data;
};

inline void QwtRoundScaleDraw::moveCenter(int x, int y)
{
    moveCenter(QPoint(x, y));
}

#endif
