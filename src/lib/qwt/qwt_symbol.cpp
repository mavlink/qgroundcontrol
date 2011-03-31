/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include <qpainter.h>
#include <qapplication.h>
#include "qwt_painter.h"
#include "qwt_polygon.h"
#include "qwt_symbol.h"

/*!
  Default Constructor

  The symbol is constructed with gray interior,
  black outline with zero width, no size and style 'NoSymbol'.
*/
QwtSymbol::QwtSymbol():
    d_brush(Qt::gray),
    d_pen(Qt::black),
    d_size(0,0),
    d_style(QwtSymbol::NoSymbol)
{
}

/*!
  \brief Constructor
  \param style Symbol Style
  \param brush brush to fill the interior
  \param pen outline pen
  \param size size
*/
QwtSymbol::QwtSymbol(QwtSymbol::Style style, const QBrush &brush,
                     const QPen &pen, const QSize &size):
    d_brush(brush),
    d_pen(pen),
    d_size(size),
    d_style(style)
{
}

//! Destructor
QwtSymbol::~QwtSymbol()
{
}

QwtSymbol *QwtSymbol::clone() const
{
    QwtSymbol *other = new QwtSymbol;
    *other = *this;

    return other;
}

/*!
  \brief Specify the symbol's size

  If the 'h' parameter is left out or less than 0,
  and the 'w' parameter is greater than or equal to 0,
  the symbol size will be set to (w,w).
  \param w width
  \param h height (defaults to -1)
*/
void QwtSymbol::setSize(int w, int h)
{
    if ((w >= 0) && (h < 0))
        h = w;
    d_size = QSize(w,h);
}

//! Set the symbol's size
void QwtSymbol::setSize(const QSize &s)
{
    if (s.isValid())
        d_size = s;
}

/*!
  \brief Assign a brush

  The brush is used to draw the interior of the symbol.
  \param br brush
*/
void QwtSymbol::setBrush(const QBrush &br)
{
    d_brush = br;
}

/*!
  \brief Assign a pen

  The pen is used to draw the symbol's outline.

  \param pn pen
*/
void QwtSymbol::setPen(const QPen &pn)
{
    d_pen = pn;
}

/*!
  \brief Draw the symbol at a point (x,y).
*/
void QwtSymbol::draw(QPainter *painter, int x, int y) const
{
    draw(painter, QPoint(x, y));
}


/*!
  \brief Draw the symbol into a bounding rectangle.

  This function assumes that the painter has been initialized with
  brush and pen before. This allows a much more performant implementation
  when painting many symbols with the same brush and pen like in curves.

  \param painter Painter
  \param r Bounding rectangle
*/
void QwtSymbol::draw(QPainter *painter, const QRect& r) const
{
    switch(d_style) {
    case QwtSymbol::Ellipse:
        QwtPainter::drawEllipse(painter, r);
        break;
    case QwtSymbol::Rect:
        QwtPainter::drawRect(painter, r);
        break;
    case QwtSymbol::Diamond: {
        const int w2 = r.width() / 2;
        const int h2 = r.height() / 2;

        QwtPolygon pa(4);
        pa.setPoint(0, r.x() + w2, r.y());
        pa.setPoint(1, r.right(), r.y() + h2);
        pa.setPoint(2, r.x() + w2, r.bottom());
        pa.setPoint(3, r.x(), r.y() + h2);
        QwtPainter::drawPolygon(painter, pa);
        break;
    }
    case QwtSymbol::Cross: {
        const int w2 = r.width() / 2;
        const int h2 = r.height() / 2;

        QwtPainter::drawLine(painter, r.x() + w2, r.y(),
                             r.x() + w2, r.bottom());
        QwtPainter::drawLine(painter, r.x(), r.y() + h2,
                             r.right(), r.y() + h2);
        break;
    }
    case QwtSymbol::XCross: {
        QwtPainter::drawLine(painter, r.left(), r.top(),
                             r.right(), r.bottom());
        QwtPainter::drawLine(painter, r.left(), r.bottom(),
                             r.right(), r.top());
        break;
    }
    case QwtSymbol::Triangle:
    case QwtSymbol::UTriangle: {
        const int w2 = r.width() / 2;

        QwtPolygon pa(3);
        pa.setPoint(0, r.x() + w2, r.y());
        pa.setPoint(1, r.right(), r.bottom());
        pa.setPoint(2, r.x(), r.bottom());
        QwtPainter::drawPolygon(painter, pa);
        break;
    }
    case QwtSymbol::DTriangle: {
        const int w2 = r.width() / 2;

        QwtPolygon pa(3);
        pa.setPoint(0, r.x(), r.y());
        pa.setPoint(1, r.right(), r.y());
        pa.setPoint(2, r.x() + w2, r.bottom());
        QwtPainter::drawPolygon(painter, pa);
        break;
    }
    case QwtSymbol::RTriangle: {
        const int h2 = r.height() / 2;

        QwtPolygon pa(3);
        pa.setPoint(0, r.x(), r.y());
        pa.setPoint(1, r.right(), r.y() + h2);
        pa.setPoint(2, r.x(), r.bottom());
        QwtPainter::drawPolygon(painter, pa);
        break;
    }
    case QwtSymbol::LTriangle: {
        const int h2 = r.height() / 2;

        QwtPolygon pa(3);
        pa.setPoint(0, r.right(), r.y());
        pa.setPoint(1, r.x(), r.y() + h2);
        pa.setPoint(2, r.right(), r.bottom());
        QwtPainter::drawPolygon(painter, pa);
        break;
    }
    case QwtSymbol::HLine: {
        const int h2 = r.height() / 2;
        QwtPainter::drawLine(painter, r.left(), r.top() + h2,
                             r.right(), r.top() + h2);
        break;
    }
    case QwtSymbol::VLine: {
        const int w2 = r.width() / 2;
        QwtPainter::drawLine(painter, r.left() + w2, r.top(),
                             r.left() + w2, r.bottom());
        break;
    }
    case QwtSymbol::Star1: {
        const double sqrt1_2 = 0.70710678118654752440; /* 1/sqrt(2) */

        const int w2 = r.width() / 2;
        const int h2 = r.height() / 2;
        const int d1  = (int)( (double)w2 * (1.0 - sqrt1_2) );

        QwtPainter::drawLine(painter, r.left() + d1, r.top() + d1,
                             r.right() - d1, r.bottom() - d1);
        QwtPainter::drawLine(painter, r.left() + d1, r.bottom() - d1,
                             r.right() - d1, r.top() + d1);
        QwtPainter::drawLine(painter, r.left() + w2, r.top(),
                             r.left() + w2, r.bottom());
        QwtPainter::drawLine(painter, r.left(), r.top() + h2,
                             r.right(), r.top() + h2);
        break;
    }
    case QwtSymbol::Star2: {
        const int w = r.width();
        const int side = (int)(((double)r.width() * (1.0 - 0.866025)) /
                               2.0);  // 0.866025 = cos(30°)
        const int h4 = r.height() / 4;
        const int h2 = r.height() / 2;
        const int h34 = (r.height() * 3) / 4;

        QwtPolygon pa(12);
        pa.setPoint(0, r.left() + (w / 2), r.top());
        pa.setPoint(1, r.right() - (side + (w - 2 * side) / 3),
                    r.top() + h4 );
        pa.setPoint(2, r.right() - side, r.top() + h4);
        pa.setPoint(3, r.right() - (side + (w / 2 - side) / 3),
                    r.top() + h2 );
        pa.setPoint(4, r.right() - side, r.top() + h34);
        pa.setPoint(5, r.right() - (side + (w - 2 * side) / 3),
                    r.top() + h34 );
        pa.setPoint(6, r.left() + (w / 2), r.bottom());
        pa.setPoint(7, r.left() + (side + (w - 2 * side) / 3),
                    r.top() + h34 );
        pa.setPoint(8, r.left() + side, r.top() + h34);
        pa.setPoint(9, r.left() + (side + (w / 2 - side) / 3),
                    r.top() + h2 );
        pa.setPoint(10, r.left() + side, r.top() + h4);
        pa.setPoint(11, r.left() + (side + (w - 2 * side) / 3),
                    r.top() + h4 );
        QwtPainter::drawPolygon(painter, pa);
        break;
    }
    case QwtSymbol::Hexagon: {
        const int w2 = r.width() / 2;
        const int side = (int)(((double)r.width() * (1.0 - 0.866025)) /
                               2.0);  // 0.866025 = cos(30°)
        const int h4 = r.height() / 4;
        const int h34 = (r.height() * 3) / 4;

        QwtPolygon pa(6);
        pa.setPoint(0, r.left() + w2, r.top());
        pa.setPoint(1, r.right() - side, r.top() + h4);
        pa.setPoint(2, r.right() - side, r.top() + h34);
        pa.setPoint(3, r.left() + w2, r.bottom());
        pa.setPoint(4, r.left() + side, r.top() + h34);
        pa.setPoint(5, r.left() + side, r.top() + h4);
        QwtPainter::drawPolygon(painter, pa);
        break;
    }
    default:
        ;
    }
}

/*!
  \brief Draw the symbol at a specified point

  \param painter Painter
  \param pos Center of the symbol
*/
void QwtSymbol::draw(QPainter *painter, const QPoint &pos) const
{
    QRect rect;
    rect.setSize(QwtPainter::metricsMap().screenToLayout(d_size));
    rect.moveCenter(pos);

    painter->setBrush(d_brush);
    painter->setPen(d_pen);

    draw(painter, rect);
}

/*!
  \brief Specify the symbol style

  The following styles are defined:<dl>
  <dt>NoSymbol<dd>No Style. The symbol cannot be drawn.
  <dt>Ellipse<dd>Ellipse or circle
  <dt>Rect<dd>Rectangle
  <dt>Diamond<dd>Diamond
  <dt>Triangle<dd>Triangle pointing upwards
  <dt>DTriangle<dd>Triangle pointing downwards
  <dt>UTriangle<dd>Triangle pointing upwards
  <dt>LTriangle<dd>Triangle pointing left
  <dt>RTriangle<dd>Triangle pointing right
  <dt>Cross<dd>Cross (+)
  <dt>XCross<dd>Diagonal cross (X)
  <dt>HLine<dd>Horizontal line
  <dt>VLine<dd>Vertical line
  <dt>Star1<dd>X combined with +
  <dt>Star2<dd>Six-pointed star
  <dt>Hexagon<dd>Hexagon</dl>

  \param s style
*/
void QwtSymbol::setStyle(QwtSymbol::Style s)
{
    d_style = s;
}

//! == operator
bool QwtSymbol::operator==(const QwtSymbol &other) const
{
    return brush() == other.brush() && pen() == other.pen()
           && style() == other.style() && size() == other.size();
}

//! != operator
bool QwtSymbol::operator!=(const QwtSymbol &other) const
{
    return !(*this == other);
}
