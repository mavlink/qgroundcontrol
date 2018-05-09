/*
    Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies). <qt-info@nokia.com>
    Copyright (C) 2011 Collabora Ltd. <info@collabora.com>

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

#ifndef QT_VIDEO_SINK_DELEGATE_H
#define QT_VIDEO_SINK_DELEGATE_H

#include "basedelegate.h"
#include "../painters/abstractsurfacepainter.h"

class QGLContext;

class QtVideoSinkDelegate : public BaseDelegate
{
    Q_OBJECT
public:
    enum PainterType {
        Generic = 0x00,
        ArbFp = 0x01,
        Glsl = 0x02
    };
    Q_DECLARE_FLAGS(PainterTypes, PainterType);

    explicit QtVideoSinkDelegate(GstElement *sink, QObject *parent = 0);
    virtual ~QtVideoSinkDelegate();

    PainterTypes supportedPainterTypes() const { return m_supportedPainters; }

#ifndef GST_QT_VIDEO_SINK_NO_OPENGL
    // glcontext property
    QGLContext *glContext() const;
    void setGLContext(QGLContext *context);
#endif

    // paint action
    void paint(QPainter *painter, const QRectF & targetArea);

protected:
    // internal event handling
    virtual bool event(QEvent *event);

private:
    void changePainter(const BufferFormat & format);
    void destroyPainter();

    AbstractSurfacePainter *m_painter;
    PainterTypes m_supportedPainters;

#ifndef GST_QT_VIDEO_SINK_NO_OPENGL
    QGLContext *m_glContext;
#endif
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QtVideoSinkDelegate::PainterTypes)

#endif // QT_VIDEO_SINK_DELEGATE_H
