/*
    Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies). <qt-info@nokia.com>
    Copyright (C) 2011-2013 Collabora Ltd. <info@collabora.com>

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License version 2.1
    as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
 * @file
 *   @brief Extracted from QtGstreamer to avoid overly complex dependency
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#include "basedelegate.h"

#include <QCoreApplication>

BaseDelegate::BaseDelegate(GstElement * sink, QObject * parent)
    : QObject(parent)
    , m_colorsDirty(true)
    , m_brightness(0)
    , m_contrast(0)
    , m_hue(0)
    , m_saturation(0)
    , m_pixelAspectRatio(1, 1)
    , m_forceAspectRatioDirty(true)
    , m_forceAspectRatio(false)
    , m_formatDirty(true)
    , m_isActive(false)
    , m_buffer(NULL)
    , m_sink(sink)
{
}

BaseDelegate::~BaseDelegate()
{
    Q_ASSERT(!isActive());
}

//-------------------------------------

bool BaseDelegate::isActive() const
{
    QReadLocker l(&m_isActiveLock);
    return m_isActive;
}

void BaseDelegate::setActive(bool active)
{
    GST_INFO_OBJECT(m_sink, active ? "Activating" : "Deactivating");

    QWriteLocker l(&m_isActiveLock);
    m_isActive = active;
    if (!active) {
        QCoreApplication::postEvent(this, new DeactivateEvent());
    }
}

//-------------------------------------

int BaseDelegate::brightness() const
{
    QReadLocker l(&m_colorsLock);
    return m_brightness;
}

void BaseDelegate::setBrightness(int brightness)
{
    QWriteLocker l(&m_colorsLock);
    m_brightness = qBound(-100, brightness, 100);
    m_colorsDirty = true;
}

int BaseDelegate::contrast() const
{
    QReadLocker l(&m_colorsLock);
    return m_contrast;
}

void BaseDelegate::setContrast(int contrast)
{
    QWriteLocker l(&m_colorsLock);
    m_contrast = qBound(-100, contrast, 100);
    m_colorsDirty = true;
}

int BaseDelegate::hue() const
{
    QReadLocker l(&m_colorsLock);
    return m_hue;
}

void BaseDelegate::setHue(int hue)
{
    QWriteLocker l(&m_colorsLock);
    m_hue = qBound(-100, hue, 100);
    m_colorsDirty = true;
}

int BaseDelegate::saturation() const
{
    QReadLocker l(&m_colorsLock);
    return m_saturation;
}

void BaseDelegate::setSaturation(int saturation)
{
    QWriteLocker l(&m_colorsLock);
    m_saturation = qBound(-100, saturation, 100);
    m_colorsDirty = true;
}

//-------------------------------------

Fraction BaseDelegate::pixelAspectRatio() const
{
    QReadLocker l(&m_pixelAspectRatioLock);
    return m_pixelAspectRatio;
}

void BaseDelegate::setPixelAspectRatio(const Fraction & f)
{
    QWriteLocker l(&m_pixelAspectRatioLock);
    m_pixelAspectRatio = f;
}

//-------------------------------------

bool BaseDelegate::forceAspectRatio() const
{
    QReadLocker l(&m_forceAspectRatioLock);
    return m_forceAspectRatio;
}

void BaseDelegate::setForceAspectRatio(bool force)
{
    QWriteLocker l(&m_forceAspectRatioLock);
    if (m_forceAspectRatio != force) {
        m_forceAspectRatio = force;
        m_forceAspectRatioDirty = true;
    }
}

//-------------------------------------

bool BaseDelegate::event(QEvent *event)
{
    switch((int) event->type()) {
    case BufferEventType:
    {
        BufferEvent *bufEvent = dynamic_cast<BufferEvent*>(event);
        Q_ASSERT(bufEvent);

        GST_TRACE_OBJECT(m_sink, "Received buffer %" GST_PTR_FORMAT, bufEvent->buffer);

        if (isActive()) {
            gst_buffer_replace (&m_buffer, bufEvent->buffer);
            update();
        }

        return true;
    }
    case BufferFormatEventType:
    {
        BufferFormatEvent *bufFmtEvent = dynamic_cast<BufferFormatEvent*>(event);
        Q_ASSERT(bufFmtEvent);

        GST_TRACE_OBJECT (m_sink, "Received buffer format event. New format: %s",
                          gst_video_format_to_string(bufFmtEvent->format.videoFormat()));

        m_formatDirty = true;
        m_bufferFormat = bufFmtEvent->format;

        return true;
    }
    case DeactivateEventType:
    {
        GST_LOG_OBJECT(m_sink, "Received deactivate event");

        gst_buffer_replace (&m_buffer, NULL);
        update();

        return true;
    }
    default:
        return QObject::event(event);
    }
}

void BaseDelegate::update()
{
    g_signal_emit_by_name(m_sink, "update");
}
