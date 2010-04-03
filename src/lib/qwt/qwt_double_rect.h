/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

/*! \file */
#ifndef QWT_DOUBLE_RECT_H
#define QWT_DOUBLE_RECT_H 1

#include "qwt_global.h"
#include "qwt_array.h"

#if QT_VERSION >= 0x040000

#include <QPointF>
#include <QSizeF>
#include <QRectF>

/*! 
  \typedef QPointF QwtDoublePoint
  \brief This is a typedef, see Trolltech Documentation for QPointF
         in QT assistant 4.x. As soon as Qt3 compatibility is dropped
         this typedef will disappear.
*/
typedef QPointF QwtDoublePoint;

/*! 
   \typedef QSizeF QwtDoubleSize
   \brief This is a typedef, see Trolltech Documentation for QSizeF
          in QT assistant 4.x. As soon as Qt3 compatibility is dropped
         this typedef will disappear.
*/
typedef QSizeF QwtDoubleSize;

/*! 
   \typedef QRectF QwtDoubleRect
   \brief This is a typedef, see Trolltech Documentation for QRectF
          in QT assistant 4.x. As soon as Qt3 compatibility is dropped
         this typedef will disappear.
*/
typedef QRectF QwtDoubleRect;

#else

#include <qpoint.h>
#include <qsize.h>
#include <qrect.h>

/*!
  \brief The QwtDoublePoint class defines a point in double coordinates
*/

class QWT_EXPORT QwtDoublePoint
{
public:
    QwtDoublePoint();
    QwtDoublePoint(double x, double y);
    QwtDoublePoint(const QPoint &);

    QPoint toPoint() const;

    bool isNull()    const;

    double x() const;
    double y() const;

    double &rx();
    double &ry();

    void setX(double x);
    void setY(double y);

    bool operator==(const QwtDoublePoint &) const;
    bool operator!=(const QwtDoublePoint &) const;

    const QwtDoublePoint operator-() const;
    const QwtDoublePoint operator+(const QwtDoublePoint &) const;
    const QwtDoublePoint operator-(const QwtDoublePoint &) const;
    const QwtDoublePoint operator*(double) const;
    const QwtDoublePoint operator/(double) const;

    QwtDoublePoint &operator+=(const QwtDoublePoint &);
    QwtDoublePoint &operator-=(const QwtDoublePoint &);
    QwtDoublePoint &operator*=(double);
    QwtDoublePoint &operator/=(double);

private:
    double d_x;
    double d_y;
};

/*!
  The QwtDoubleSize class defines a size in double coordinates
*/

class QWT_EXPORT QwtDoubleSize
{
public:
    QwtDoubleSize();
    QwtDoubleSize(double width, double height);
    QwtDoubleSize(const QSize &);

    bool isNull() const;
    bool isEmpty() const;
    bool isValid() const;

    double width() const;
    double height() const;
    void setWidth( double w );
    void setHeight( double h );
    void transpose();

    QwtDoubleSize expandedTo(const QwtDoubleSize &) const;
    QwtDoubleSize boundedTo(const QwtDoubleSize &) const;

    bool operator==(const QwtDoubleSize &) const;
    bool operator!=(const QwtDoubleSize &) const;

    const QwtDoubleSize operator+(const QwtDoubleSize &) const;
    const QwtDoubleSize operator-(const QwtDoubleSize &) const;
    const QwtDoubleSize operator*(double) const;
    const QwtDoubleSize operator/(double) const;

    QwtDoubleSize &operator+=(const QwtDoubleSize &);
    QwtDoubleSize &operator-=(const QwtDoubleSize &);
    QwtDoubleSize &operator*=(double c);
    QwtDoubleSize &operator/=(double c);

private:
    double d_width;
    double d_height;
};

/*!
  The QwtDoubleRect class defines a size in double coordinates.
*/

class QWT_EXPORT QwtDoubleRect  
{
public:
    QwtDoubleRect();
    QwtDoubleRect(double left, double top, double width, double height);
    QwtDoubleRect(const QwtDoublePoint&, const QwtDoubleSize &);

    QwtDoubleRect(const QRect &);
    QRect toRect() const;

    bool isNull()    const;
    bool isEmpty()   const;
    bool isValid()   const;

    QwtDoubleRect normalized() const;

    double x()  const;
    double y()  const;

    double left()  const;
    double right()  const;
    double top()  const;
    double bottom()  const;

    void setX(double);
    void setY(double);

    void setLeft(double);
    void setRight(double);
    void setTop(double);
    void setBottom(double);

    QwtDoublePoint center()  const;

    void moveLeft(double x);
    void moveRight(double x);
    void moveTop(double y );
    void moveBottom(double y );
    void moveTo(double x, double y);
    void moveTo(const QwtDoublePoint &);
    void moveBy(double dx, double dy);
    void moveCenter(const QwtDoublePoint &);
    void moveCenter(double dx, double dy);

    void setRect(double x1, double x2, double width, double height);

    double width()   const;
    double height()  const;
    QwtDoubleSize size() const;

    void setWidth(double w );
    void setHeight(double h );
    void setSize(const QwtDoubleSize &);

    QwtDoubleRect  operator|(const QwtDoubleRect &r) const;
    QwtDoubleRect  operator&(const QwtDoubleRect &r) const;
    QwtDoubleRect &operator|=(const QwtDoubleRect &r);
    QwtDoubleRect &operator&=(const QwtDoubleRect &r);
    bool operator==( const QwtDoubleRect &) const;
    bool operator!=( const QwtDoubleRect &) const;

    bool contains(const QwtDoublePoint &p, bool proper = false) const;
    bool contains(double x, double y, bool proper = false) const; 
    bool contains(const QwtDoubleRect &r, bool proper=false) const;

    QwtDoubleRect unite(const QwtDoubleRect &) const;
    QwtDoubleRect intersect(const QwtDoubleRect &) const;
    bool intersects(const QwtDoubleRect &) const;

    QwtDoublePoint bottomRight() const;
    QwtDoublePoint topRight() const;
    QwtDoublePoint topLeft() const;
    QwtDoublePoint bottomLeft() const;

private:
    double d_left;
    double d_right;
    double d_top;
    double d_bottom;
};

/*! 
    Returns true if the point is null; otherwise returns false.

    A point is considered to be null if both the x- and y-coordinates 
    are equal to zero.
*/
inline bool QwtDoublePoint::isNull() const
{ 
    return d_x == 0.0 && d_y == 0.0; 
}

//! Returns the x-coordinate of the point.
inline double QwtDoublePoint::x() const
{ 
    return d_x; 
}

//! Returns the y-coordinate of the point.
inline double QwtDoublePoint::y() const
{   
    return d_y; 
}

//! Returns a reference to the x-coordinate of the point.
inline double &QwtDoublePoint::rx()
{
    return d_x;
}

//! Returns a reference to the y-coordinate of the point.
inline double &QwtDoublePoint::ry()
{
    return d_y;
}

//! Sets the x-coordinate of the point to the value specified by x.
inline void QwtDoublePoint::setX(double x)
{ 
    d_x = x; 
}

//! Sets the y-coordinate of the point to the value specified by y.
inline void QwtDoublePoint::setY(double y)
{ 
    d_y = y; 
}

/*!
   Rounds the coordinates of this point to the nearest integer and 
   returns a QPoint with these rounded coordinates.
*/
inline QPoint QwtDoublePoint::toPoint() const
{
    return QPoint(qRound(d_x), qRound(d_y));
}

/*!
  Returns true if the width is 0 and the height is 0; 
  otherwise returns false.
*/
inline bool QwtDoubleSize::isNull() const
{ 
    return d_width == 0.0 && d_height == 0.0; 
}

/*! 
  Returns true if the width is <= 0.0 or the height is <= 0.0, 
  otherwise false. 
*/
inline bool QwtDoubleSize::isEmpty() const
{ 
    return d_width <= 0.0 || d_height <= 0.0; 
}

/*!
  Returns true if the width is equal to or greater than 0.0 and the height 
  is equal to or greater than 0.0; otherwise returns false.
*/
inline bool QwtDoubleSize::isValid() const
{ 
    return d_width >= 0.0 && d_height >= 0.0; 
}

//! Returns the width. 
inline double QwtDoubleSize::width() const
{ 
    return d_width; 
}

//! Returns the height. 
inline double QwtDoubleSize::height() const
{ 
    return d_height; 
}

//! Sets the width to width. 
inline void QwtDoubleSize::setWidth(double width)
{ 
    d_width = width; 
}

//! Sets the height to height. 
inline void QwtDoubleSize::setHeight(double height)
{ 
    d_height = height; 
}

/*!
    Returns true if the rectangle is a null rectangle; otherwise returns false.
    A null rectangle has both the width and the height set to 0.
    A null rectangle is also empty and invalid.

    \sa QwtDoubleRect::isEmpty, QwtDoubleRect::isValid
*/
inline bool QwtDoubleRect::isNull() const
{ 
    return d_right == d_left && d_bottom == d_top;
}

/*!
    Returns true if the rectangle is empty; otherwise returns false.
    An empty rectangle has a width() <= 0 or height() <= 0.
    An empty rectangle is not valid. isEmpty() == !isValid()

    \sa QwtDoubleRect::isNull, QwtDoubleRect::isValid
*/
inline bool QwtDoubleRect::isEmpty() const
{ 
    return d_left >= d_right || d_top >= d_bottom; 
}

/*!
    Returns true if the rectangle is valid; otherwise returns false.
    A valid rectangle has a width() > 0 and height() > 0.
    Note that non-trivial operations like intersections are not defined 
    for invalid rectangles.  isValid() == !isEmpty()

    \sa isNull(), isEmpty(), and normalized().
*/
inline bool QwtDoubleRect::isValid() const
{ 
    return d_left < d_right && d_top < d_bottom; 
}

//! Returns x
inline double QwtDoubleRect::x() const
{ 
    return d_left; 
}

//! Returns y
inline double QwtDoubleRect::y() const
{ 
    return d_top; 
}

//! Returns left
inline double QwtDoubleRect::left() const
{ 
    return d_left; 
}

//! Returns right
inline double QwtDoubleRect::right() const
{ 
    return d_right; 
}

//! Returns top
inline double QwtDoubleRect::top() const
{ 
    return d_top; 
}

//! Returns bottom
inline double QwtDoubleRect::bottom() const
{ 
    return d_bottom; 
}

//! Set left  
inline void QwtDoubleRect::setX(double x)
{ 
    d_left = x;
}

//! Set left  
inline void QwtDoubleRect::setY(double y)
{ 
    d_top = y;
}

//! Set left  
inline void QwtDoubleRect::setLeft(double x)
{ 
    d_left = x;
}

//! Set right  
inline void QwtDoubleRect::setRight(double x)
{ 
    d_right = x;
}

//! Set top  
inline void QwtDoubleRect::setTop(double y)
{ 
    d_top = y;
}

//! Set bottom  
inline void QwtDoubleRect::setBottom(double y)
{ 
    d_bottom = y;
}

//! Returns the width
inline double QwtDoubleRect::width() const
{ 
    return  d_right - d_left; 
}

//! Returns the height
inline double QwtDoubleRect::height() const
{ 
    return  d_bottom - d_top; 
}

//! Returns the size
inline QwtDoubleSize QwtDoubleRect::size() const
{ 
    return QwtDoubleSize(width(), height());
}

//! Set the width, by right = left + w;
inline void QwtDoubleRect::setWidth(double w)
{
    d_right = d_left + w;
}

//! Set the height, by bottom = top + h;
inline void QwtDoubleRect::setHeight(double h)
{
    d_bottom = d_top + h;
}

/*! 
    Moves the top left corner of the rectangle to p, 
    without changing the rectangles size.
*/
inline void QwtDoubleRect::moveTo(const QwtDoublePoint &p)
{
    moveTo(p.x(), p.y());
}

inline QwtDoublePoint QwtDoubleRect::bottomRight() const
{
    return QwtDoublePoint(bottom(), right());
}

inline QwtDoublePoint QwtDoubleRect::topRight() const
{
    return QwtDoublePoint(top(), right());
}

inline QwtDoublePoint QwtDoubleRect::topLeft() const
{
    return QwtDoublePoint(top(), left());
}

inline QwtDoublePoint QwtDoubleRect::bottomLeft() const
{
    return QwtDoublePoint(bottom(), left());
}


#endif // QT_VERSION < 0x040000

#endif // QWT_DOUBLE_RECT_H
