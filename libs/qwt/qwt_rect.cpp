/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include "qwt_clipper.h"
#include "qwt_rect.h"

//! Constructor
QwtRect::QwtRect():
    QRect()
{
}

//! Copy constructor
QwtRect::QwtRect(const QRect &r):
    QRect(r)
{
}

//! Sutherland-Hodgman polygon clipping
QwtPolygon QwtRect::clip(const QwtPolygon &pa) const
{
    return QwtClipper::clipPolygon(*this, pa);
}
