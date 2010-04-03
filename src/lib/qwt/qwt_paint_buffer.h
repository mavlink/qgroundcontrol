/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef QWT_PAINT_BUFFER_H
#define QWT_PAINT_BUFFER_H 1

#include <qglobal.h>
#if QT_VERSION < 0x040000

#include <qpixmap.h>
#include "qwt_global.h"

class QPainter;

/*!
  \brief Paint buffer for Qwt widgets

  QwtPaintBuffer offers a simple way to en/disable double buffering.
  Double buffering is enabled as default and in general there will be
  no reason to change this. 
*/

class QWT_EXPORT QwtPaintBuffer
{
public:
    explicit QwtPaintBuffer();
    explicit QwtPaintBuffer(QPaintDevice *, const QRect &, QPainter *p = NULL);

    virtual ~QwtPaintBuffer();

    void open(QPaintDevice *, const QRect &, QPainter *p = NULL);
    void close();

    QPainter *painter();
    const QPaintDevice *device();
    
    static void setEnabled(bool enable);
    static bool isEnabled();

    //! Return Buffer used for double buffering
    const QPixmap &buffer() const { return d_pixBuffer; }

protected:
    void flush();

private:
    QPixmap d_pixBuffer;
    QRect d_rect;

    QPaintDevice *d_device; // use QGuardedPtr
    QPainter *d_painter; // use QGuardedPtr
    QPainter *d_devicePainter; // use QGuardedPtr

    static bool d_enabled;
};

#endif // QT_VERSION < 0x040000

#endif
