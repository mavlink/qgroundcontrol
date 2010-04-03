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

#include <qwidget.h>
#include <qpainter.h>
#include "qwt_paint_buffer.h"

bool QwtPaintBuffer::d_enabled = true;

//! Default constructor
QwtPaintBuffer::QwtPaintBuffer():
    d_device(0),
    d_painter(0),
    d_devicePainter(0)
{
}

/*! 
    Create an open paint buffer
    \param device Device to paint on
    \param rect Rect to paint on
    \param painter Painter to paint on device. In case of 0
                   QwtPaintBuffer uses an internal painter

    \sa open()
*/

QwtPaintBuffer::QwtPaintBuffer(QPaintDevice *device, 
        const QRect &rect, QPainter *painter):
    d_device(0),
    d_painter(0),
    d_devicePainter(0)
{
    open(device, rect, painter);
}

/*! 
    Closes the buffer
    \sa close()
*/
QwtPaintBuffer::~QwtPaintBuffer()
{
    close();
}

/*! 
  \return Depending on isEnabled() the painter
          connected to an internal pixmap buffer 
          otherwise the painter connected to the device.
*/

QPainter *QwtPaintBuffer::painter() 
{ 
    return d_painter; 
}

/*! 
  \return Device to paint on
*/
const QPaintDevice *QwtPaintBuffer::device() 
{ 
    return d_device; 
}

/*! 
    Enable/Disable double buffering. Please note that
    this is a global switch for all QwtPaintBuffers, but
    won't change opened buffers.
*/
void QwtPaintBuffer::setEnabled(bool enable) 
{ 
    d_enabled = enable; 
}

/*! 
  \return true if double buffering is enabled, false otherwise.
*/
bool QwtPaintBuffer::isEnabled() 
{ 
    return d_enabled; 
}

/*! 
    Open the buffer
    \param device Device to paint on
    \param rect Rect to paint on
    \param painter Painter to paint on device. In case of 0
                   QwtPaintBuffer uses an internal painter
*/

void QwtPaintBuffer::open(QPaintDevice *device, 
        const QRect &rect, QPainter *painter)
{
    close();

    if ( device == 0 || !rect.isValid() )
        return;

    d_device = device;
    d_devicePainter = painter;
    d_rect = rect;

    if ( isEnabled() )
    {
#ifdef Q_WS_X11
        if ( d_pixBuffer.x11Screen() != d_device->x11Screen() )
            d_pixBuffer.x11SetScreen(d_device->x11Screen());
#endif
        d_pixBuffer.resize(d_rect.size());

        d_painter = new QPainter();
        if ( d_device->devType() == QInternal::Widget )
        {
            QWidget *w = (QWidget *)d_device;
            d_pixBuffer.fill(w, d_rect.topLeft());
            d_painter->begin(&d_pixBuffer, w);
            d_painter->translate(-d_rect.x(), -d_rect.y());
        }
        else
        {
            d_painter->begin(&d_pixBuffer);
        }
    }
    else
    {
        if ( d_devicePainter )
            d_painter = d_devicePainter;
        else
            d_painter = new QPainter(d_device);

        if ( d_device->devType() == QInternal::Widget )
        {
            QWidget *w = (QWidget *)d_device;
            if ( w->testWFlags( Qt::WNoAutoErase ) )
                d_painter->eraseRect(d_rect);
        }
    }
}

/*! 
    Flush the internal pixmap buffer to the device.
*/
void QwtPaintBuffer::flush()
{
    if ( d_enabled && d_device != 0 && d_rect.isValid())
    {
        // We need a painter to find out if
        // there is a painter redirection for d_device.

        QPainter *p;
        if ( d_devicePainter == 0 )
            p = new QPainter(d_device);
        else 
            p = d_devicePainter;

        QPaintDevice *device = p->device();
        if ( device->isExtDev() )
            d_devicePainter->drawPixmap(d_rect.topLeft(), d_pixBuffer);
        else
            bitBlt(device, d_rect.topLeft(), &d_pixBuffer );

        if ( d_devicePainter == 0 )
            delete p;
    }
}

/*! 
    Flush the internal pixmap buffer to the device and close the buffer.
*/
void QwtPaintBuffer::close()
{
    flush();

    if ( d_painter )
    {
        if ( d_painter->isActive() )
            d_painter->end();

        if ( d_painter != d_devicePainter )
            delete d_painter;
    }

    if ( !d_pixBuffer.isNull() )
        d_pixBuffer = QPixmap();

    d_device = 0;
    d_painter = 0;
    d_devicePainter = 0;
} 

#endif // QT_VERSION < 0x040000
