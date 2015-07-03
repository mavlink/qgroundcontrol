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

#ifndef BASEDELEGATE_H
#define BASEDELEGATE_H

#include <gst/gst.h>

#include "../gstqtvideosinkplugin.h" //for debug category
#include "../utils/bufferformat.h"
#include "../utils/utils.h"

#include <QObject>
#include <QEvent>
#include <QReadWriteLock>

class BaseDelegate : public QObject
{
    Q_OBJECT
public:
    enum EventType {
        BufferEventType = QEvent::User,
        BufferFormatEventType,
        DeactivateEventType
    };

    //-------------------------------------

    class BufferEvent : public QEvent
    {
    public:
        inline BufferEvent(GstBuffer *buf)
            : QEvent(static_cast<QEvent::Type>(BufferEventType)),
              buffer(gst_buffer_ref(buf))
        {}

        virtual ~BufferEvent() {
            gst_buffer_unref(buffer);
        }

        GstBuffer *buffer;
    };

    class BufferFormatEvent : public QEvent
    {
    public:
        inline BufferFormatEvent(const BufferFormat &format)
            : QEvent(static_cast<QEvent::Type>(BufferFormatEventType)),
            format(format)
        {}

        BufferFormat format;
    };

    class DeactivateEvent : public QEvent
    {
    public:
        inline DeactivateEvent()
            : QEvent(static_cast<QEvent::Type>(DeactivateEventType))
        {
        }
    };

    //-------------------------------------

    explicit BaseDelegate(GstElement *sink, QObject *parent = 0);
    virtual ~BaseDelegate();

    bool isActive() const;
    void setActive(bool playing);

    // GstColorBalance interface

    int brightness() const;
    void setBrightness(int brightness);

    int contrast() const;
    void setContrast(int contrast);

    int hue() const;
    void setHue(int hue);

    int saturation() const;
    void setSaturation(int saturation);

    // pixel-aspect-ratio property
    Fraction pixelAspectRatio() const;
    void setPixelAspectRatio(const Fraction & f);

    // force-aspect-ratio property
    bool forceAspectRatio() const;
    void setForceAspectRatio(bool force);

protected:
    // internal event handling
    virtual bool event(QEvent *event);

    // tells the surface to repaint itself
    virtual void update();

protected:
    // colorbalance interface properties
    mutable QReadWriteLock m_colorsLock;
    bool m_colorsDirty;
    int m_brightness;
    int m_contrast;
    int m_hue;
    int m_saturation;

    // pixel-aspect-ratio property
    mutable QReadWriteLock m_pixelAspectRatioLock;
    Fraction m_pixelAspectRatio;

    // force-aspect-ratio property
    mutable QReadWriteLock m_forceAspectRatioLock;
    bool m_forceAspectRatioDirty;
    bool m_forceAspectRatio;

    // format caching
    bool m_formatDirty;
    BufferFormat m_bufferFormat;
    PaintAreas m_areas;

    // whether the sink is active (PAUSED or PLAYING)
    mutable QReadWriteLock m_isActiveLock;
    bool m_isActive;

    // the buffer to be drawn next
    GstBuffer *m_buffer;

    // the video sink element
    GstElement * const m_sink;
};

#endif // BASEDELEGATE_H
