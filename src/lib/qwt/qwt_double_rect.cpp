/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include <qglobal.h>

#if QT_VERSION < 0x040000

#include "qwt_math.h"
#include "qwt_double_rect.h"

/*! 
    Constructs a null point.

    \sa QwtDoublePoint::isNull
*/
QwtDoublePoint::QwtDoublePoint():
    d_x(0.0),
    d_y(0.0)
{
}

//! Constructs a point with coordinates specified by x and y.
QwtDoublePoint::QwtDoublePoint(double x, double y ):
    d_x(x),
    d_y(y)
{
}

/*! 
    Copy constructor. 

    Constructs a point using the values of the point specified.
*/
QwtDoublePoint::QwtDoublePoint(const QPoint &p):
    d_x(double(p.x())),
    d_y(double(p.y()))
{
}

/*! 
    Returns true if point1 is equal to point2; otherwise returns false.

    Two points are equal to each other if both x-coordinates and 
    both y-coordinates are the same.
*/
bool QwtDoublePoint::operator==(const QwtDoublePoint &other) const
{
    return (d_x == other.d_x) && (d_y == other.d_y);
}

//! Returns true if point1 is not equal to point2; otherwise returns false.
bool QwtDoublePoint::operator!=(const QwtDoublePoint &other) const
{
    return !operator==(other);
}

/*! 
    Negates the coordinates of the point, and returns a point with the 
    new coordinates. (Inversion).
*/
const QwtDoublePoint QwtDoublePoint::operator-() const
{
    return QwtDoublePoint(-d_x, -d_y);
}

/*! 
    Adds the coordinates of the point to the corresponding coordinates of 
    the other point, and returns a point with the new coordinates. 
    (Vector addition.)
*/
const QwtDoublePoint QwtDoublePoint::operator+(
    const QwtDoublePoint &other) const
{
    return QwtDoublePoint(d_x + other.d_x, d_y + other.d_y);
}

/*! 
    Subtracts the coordinates of the other point from the corresponding 
    coordinates of the given point, and returns a point with the new 
    coordinates. (Vector subtraction.)
*/
const QwtDoublePoint QwtDoublePoint::operator-(
    const QwtDoublePoint &other) const
{
    return QwtDoublePoint(d_x - other.d_x, d_y - other.d_y);
}

/*!
    Multiplies the coordinates of the point by the given scale factor, 
    and returns a point with the new coordinates. 
    (Scalar multiplication of a vector.)
*/
const QwtDoublePoint QwtDoublePoint::operator*(double factor) const
{
    return QwtDoublePoint(d_x * factor, d_y * factor);
}

/*!
    Divides the coordinates of the point by the given scale factor, 
    and returns a point with the new coordinates. 
    (Scalar division of a vector.)
*/
const QwtDoublePoint QwtDoublePoint::operator/(double factor) const
{
    return QwtDoublePoint(d_x / factor, d_y / factor);
}

/*!
    Adds the coordinates of this point to the corresponding coordinates 
    of the other point, and returns a reference to this point with the 
    new coordinates. This is equivalent to vector addition.
*/
QwtDoublePoint &QwtDoublePoint::operator+=(const QwtDoublePoint &other)
{
    d_x += other.d_x;
    d_y += other.d_y;
    return *this;
}

/*!
    Subtracts the coordinates of the other point from the corresponding 
    coordinates of this point, and returns a reference to this point with 
    the new coordinates. This is equivalent to vector subtraction.
*/
QwtDoublePoint &QwtDoublePoint::operator-=(const QwtDoublePoint &other)
{
    d_x -= other.d_x;
    d_y -= other.d_y;
    return *this;
}

/*!
    Multiplies the coordinates of this point by the given scale factor, 
    and returns a reference to this point with the new coordinates. 
    This is equivalent to scalar multiplication of a vector.
*/
QwtDoublePoint &QwtDoublePoint::operator*=(double factor)
{
    d_x *= factor;
    d_y *= factor;
    return *this;
}

/*!
    Divides the coordinates of this point by the given scale factor, 
    and returns a references to this point with the new coordinates. 
    This is equivalent to scalar division of a vector.
*/
QwtDoublePoint &QwtDoublePoint::operator/=(double factor)
{
    d_x /= factor;
    d_y /= factor;
    return *this;
}

//! Constructs an invalid size.
QwtDoubleSize::QwtDoubleSize():
    d_width(-1.0),
    d_height(-1.0)
{   
}   

//! Constructs a size with width width and height height.
QwtDoubleSize::QwtDoubleSize( double width, double height ):
    d_width(width),
    d_height(height)
{   
}   

//! Constructs a size with floating point accuracy from the given size.
QwtDoubleSize::QwtDoubleSize(const QSize &sz):
    d_width(double(sz.width())),
    d_height(double(sz.height()))
{   
}   

//! Swaps the width and height values.
void QwtDoubleSize::transpose()
{   
    double tmp = d_width;
    d_width = d_height;
    d_height = tmp;
}

/*! 
    Returns a size with the maximum width and height of this 
    size and other.
*/
QwtDoubleSize QwtDoubleSize::expandedTo(
    const QwtDoubleSize &other) const
{   
    return QwtDoubleSize(
        qwtMax(d_width, other.d_width),
        qwtMax(d_height, other.d_height)
    );  
}   

/*!
    Returns a size with the minimum width and height of this size and other.
*/
QwtDoubleSize QwtDoubleSize::boundedTo(
    const QwtDoubleSize &other) const
{   
    return QwtDoubleSize(
        qwtMin(d_width, other.d_width),
        qwtMin(d_height, other.d_height)
    );  
}   

//! Returns true if s1 and s2 are equal; otherwise returns false.
bool QwtDoubleSize::operator==(const QwtDoubleSize &other) const
{ 
    return d_width == other.d_width && d_height == other.d_height;
}   

//! Returns true if s1 and s2 are different; otherwise returns false.
bool QwtDoubleSize::operator!=(const QwtDoubleSize &other) const
{ 
    return !operator==(other);
}   

/*! 
  Returns the size formed by adding both components by
  the components of other. Each component is added separately.
*/
const QwtDoubleSize QwtDoubleSize::operator+(
    const QwtDoubleSize &other) const
{   
    return QwtDoubleSize(d_width + other.d_width,
        d_height + other.d_height); 
}       

/*! 
  Returns the size formed by subtracting both components by
  the components of other. Each component is subtracted separately.
*/  
const QwtDoubleSize QwtDoubleSize::operator-(
    const QwtDoubleSize &other) const
{   
    return QwtDoubleSize(d_width - other.d_width,
        d_height - other.d_height); 
}       

//! Returns the size formed by multiplying both components by c.
const QwtDoubleSize QwtDoubleSize::operator*(double c) const
{ 
    return QwtDoubleSize(d_width * c, d_height * c);
}   

//! Returns the size formed by dividing both components by c.
const QwtDoubleSize QwtDoubleSize::operator/(double c) const
{ 
    return QwtDoubleSize(d_width / c, d_height / c);
}   

//! Adds size other to this size and returns a reference to this size.
QwtDoubleSize &QwtDoubleSize::operator+=(const QwtDoubleSize &other)
{   
    d_width += other.d_width; 
    d_height += other.d_height;
    return *this;
}

//! Subtracts size other from this size and returns a reference to this size.
QwtDoubleSize &QwtDoubleSize::operator-=(const QwtDoubleSize &other)
{   
    d_width -= other.d_width; 
    d_height -= other.d_height;
    return *this;
}

/* 
  Multiplies this size's width and height by c, 
  and returns a reference to this size.
*/
QwtDoubleSize &QwtDoubleSize::operator*=(double c)
{   
    d_width *= c; 
    d_height *= c;
    return *this;
}

/* 
  Devides this size's width and height by c, 
  and returns a reference to this size.
*/
QwtDoubleSize &QwtDoubleSize::operator/=(double c)
{
    d_width /= c;
    d_height /= c;
    return *this;
}   

//! Constructs an rectangle with all components set to 0.0 
QwtDoubleRect::QwtDoubleRect():
    d_left(0.0),
    d_right(0.0),
    d_top(0.0),
    d_bottom(0.0)
{
}

/*! 
  Constructs an rectangle with x1 to x2 as x-range and,
  y1 to y2 as y-range.
*/
QwtDoubleRect::QwtDoubleRect(double left, double top,
        double width, double height):
    d_left(left),
    d_right(left + width),
    d_top(top),
    d_bottom(top + height)
{
}

/*! 
  Constructs a rectangle with topLeft as the top-left corner and 
  size as the rectangle size.
*/
QwtDoubleRect::QwtDoubleRect(
        const QwtDoublePoint &p, const QwtDoubleSize &size):
    d_left(p.x()),
    d_right(p.x() + size.width()),
    d_top(p.y()),
    d_bottom(p.y() + size.height())
{
}

QwtDoubleRect::QwtDoubleRect(const QRect &rect):
    d_left(rect.left()),
    d_right(rect.right()),
    d_top(rect.top()),
    d_bottom(rect.bottom())
{
}

QRect QwtDoubleRect::toRect() const
{
    return QRect(qRound(x()), qRound(y()), qRound(width()), qRound(height()));
}

/*! 
  Set the x-range from x1 to x2 and the y-range from y1 to y2.
*/
void QwtDoubleRect::setRect(double left, double top, 
    double width, double height)
{
    d_left = left;
    d_right = left + width;
    d_top = top;
    d_bottom = top + height;
}

/*!
  Sets the size of the rectangle to size. 
  Changes x2 and y2 only.
*/
void QwtDoubleRect::setSize(const QwtDoubleSize &size)
{
    setWidth(size.width());
    setHeight(size.height());
}

/*!
  Returns a normalized rectangle, i.e. a rectangle that has a non-negative 
  width and height. 

  It swaps x1 and x2 if x1() > x2(), and swaps y1 and y2 if y1() > y2(). 
*/
QwtDoubleRect QwtDoubleRect::normalized() const
{
    QwtDoubleRect r;
    if ( d_right < d_left ) 
    {
        r.d_left = d_right;
        r.d_right = d_left;
    } 
    else 
    {
        r.d_left = d_left;
        r.d_right = d_right; 
    }
    if ( d_bottom < d_top ) 
    { 
        r.d_top = d_bottom; 
        r.d_bottom = d_top;
    } 
    else 
    {
        r.d_top = d_top;
        r.d_bottom = d_bottom;
    }
    return r;
}

/*!
  Returns the bounding rectangle of this rectangle and rectangle other. 
  r.unite(s) is equivalent to r|s. 
*/
QwtDoubleRect QwtDoubleRect::unite(const QwtDoubleRect &other) const
{
    return *this | other;
}

/*!
  Returns the intersection of this rectangle and rectangle other. 
  r.intersect(s) is equivalent to r&s. 
*/
QwtDoubleRect QwtDoubleRect::intersect(const QwtDoubleRect &other) const
{
    return *this & other;
}

/*!
  Returns true if this rectangle intersects with rectangle other; 
  otherwise returns false. 
*/
bool QwtDoubleRect::intersects(const QwtDoubleRect &other) const
{
    return ( qwtMax(d_left, other.d_left) <= qwtMin(d_right, other.d_right) ) &&
         ( qwtMax(d_top, other.d_top ) <= qwtMin(d_bottom, other.d_bottom) );
}

//! Returns true if this rect and other are equal; otherwise returns false. 
bool QwtDoubleRect::operator==(const QwtDoubleRect &other) const
{
    return d_left == other.d_left && d_right == other.d_right && 
        d_top == other.d_top && d_bottom == other.d_bottom;
}

//! Returns true if this rect and other are different; otherwise returns false. 
bool QwtDoubleRect::operator!=(const QwtDoubleRect &other) const
{
    return !operator==(other);
}

/*!
  Returns the bounding rectangle of this rectangle and rectangle other. 
  The bounding rectangle of a nonempty rectangle and an empty or 
  invalid rectangle is defined to be the nonempty rectangle. 
*/
QwtDoubleRect QwtDoubleRect::operator|(const QwtDoubleRect &other) const
{
    if ( isEmpty() ) 
        return other;

    if ( other.isEmpty() ) 
        return *this;
        
    const double minX = qwtMin(d_left, other.d_left);
    const double maxX = qwtMax(d_right, other.d_right);
    const double minY = qwtMin(d_top, other.d_top);
    const double maxY = qwtMax(d_bottom, other.d_bottom);

    return QwtDoubleRect(minX, minY, maxX - minX, maxY - minY);
}

/*!
  Returns the intersection of this rectangle and rectangle other. 
  Returns an empty rectangle if there is no intersection. 
*/
QwtDoubleRect QwtDoubleRect::operator&(const QwtDoubleRect &other) const
{
    if (isNull() || other.isNull())
        return QwtDoubleRect();

    const QwtDoubleRect r1 = normalized();
    const QwtDoubleRect r2 = other.normalized();

    const double minX = qwtMax(r1.left(), r2.left());
    const double maxX = qwtMin(r1.right(), r2.right());
    const double minY = qwtMax(r1.top(), r2.top());
    const double maxY = qwtMin(r1.bottom(), r2.bottom());

    return QwtDoubleRect(minX, minY, maxX - minX, maxY - minY);
}

//! Unites this rectangle with rectangle other. 
QwtDoubleRect &QwtDoubleRect::operator|=(const QwtDoubleRect &other)
{
    *this = *this | other;
    return *this;
}

//! Intersects this rectangle with rectangle other. 
QwtDoubleRect &QwtDoubleRect::operator&=(const QwtDoubleRect &other)
{
    *this = *this & other;
    return *this;
}

//! Returns the center point of the rectangle. 
QwtDoublePoint QwtDoubleRect::center() const
{
    return QwtDoublePoint(d_left + (d_right - d_left) / 2.0, 
        d_top + (d_bottom - d_top) / 2.0);
}

/*!
  Returns true if the point (x, y) is inside or on the edge of the rectangle; 
  otherwise returns false. 

  If proper is true, this function returns true only if p is inside 
  (not on the edge). 
*/
bool QwtDoubleRect::contains(double x, double y, bool proper) const
{
    if ( proper )
        return x > d_left && x < d_right && y > d_top && y < d_bottom;
    else
        return x >= d_left && x <= d_right && y >= d_top && y <= d_bottom;
}

/*!
  Returns true if the point p is inside or on the edge of the rectangle; 
  otherwise returns false. 

  If proper is true, this function returns true only if p is inside 
  (not on the edge). 
*/
bool QwtDoubleRect::contains(const QwtDoublePoint &p, bool proper) const
{
    return contains(p.x(), p.y(), proper);
}

/*!
  Returns true if the rectangle other is inside this rectangle; 
  otherwise returns false. 

  If proper is true, this function returns true only if other is entirely 
  inside (not on the edge). 
*/
bool QwtDoubleRect::contains(const QwtDoubleRect &other, bool proper) const
{
    return contains(other.d_left, other.d_top, proper) && 
        contains(other.d_right, other.d_bottom, proper);
}

//! moves x1() to x, leaving the size unchanged
void QwtDoubleRect::moveLeft(double x)
{
    const double w = width();
    d_left = x;
    d_right = d_left + w;
}

//! moves x1() to x, leaving the size unchanged
void QwtDoubleRect::moveRight(double x)
{
    const double w = width();
    d_right = x;
    d_left = d_right - w;
}

//! moves y1() to y, leaving the size unchanged
void QwtDoubleRect::moveTop(double y)
{
    const double h = height();
    d_top = y;
    d_bottom = d_top + h;
}

//! moves y1() to y, leaving the size unchanged
void QwtDoubleRect::moveBottom(double y)
{
    const double h = height();
    d_bottom = y;
    d_top = d_bottom - h;
}

//! moves left() to x and top() to y, leaving the size unchanged
void QwtDoubleRect::moveTo(double x, double y)
{
    moveLeft(x);
    moveTop(y);
}

//! moves x1() by dx and y1() by dy. leaving the size unchanged
void QwtDoubleRect::moveBy(double dx, double dy)
{
    d_left += dx;
    d_right += dx;
    d_top += dy;
    d_bottom += dy;
}

//! moves the center to pos, leaving the size unchanged
void QwtDoubleRect::moveCenter(const QwtDoublePoint &pos)
{
    moveCenter(pos.x(), pos.y());
}

//! moves the center to (x, y), leaving the size unchanged
void QwtDoubleRect::moveCenter(double x, double y)
{
    moveTo(x - width() / 2.0, y - height() / 2.0);
}

#endif // QT_VERSION < 0x040000
