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

#include "qtvideosinkdelegate.h"
#include "../painters/genericsurfacepainter.h"
#include "../painters/openglsurfacepainter.h"

#include <QStack>
#include <QPainter>

#include "glutils.h"

QtVideoSinkDelegate::QtVideoSinkDelegate(GstElement *sink, QObject *parent)
    : BaseDelegate(sink, parent)
    , m_painter(0)
    , m_supportedPainters(Generic)
#ifndef GST_QT_VIDEO_SINK_NO_OPENGL
    , m_glContext(0)
#endif
{
}

QtVideoSinkDelegate::~QtVideoSinkDelegate()
{
    destroyPainter();
}

void QtVideoSinkDelegate::paint(QPainter *painter, const QRectF & targetArea)
{
    GST_TRACE_OBJECT(m_sink, "paint called");

#ifndef GST_QT_VIDEO_SINK_NO_OPENGL
    if (m_glContext) {
        Q_ASSERT_X(m_glContext == QGLContext::currentContext(),
            "qtvideosink - paint",
            "Please use a QPainter that is initialized to paint on the "
            "GL surface that has the same context as the one given on the glcontext property"
        );
    }
#endif

    if (!m_buffer) {
        painter->fillRect(targetArea, Qt::black);
    } else {
        //recalculate the video area if needed
        QReadLocker forceAspectRatioLocker(&m_forceAspectRatioLock);
        if (targetArea != m_areas.targetArea || m_formatDirty
             || m_forceAspectRatioDirty)
        {
            m_forceAspectRatioDirty = false;

            QReadLocker pixelAspectRatioLocker(&m_pixelAspectRatioLock);
            Qt::AspectRatioMode aspectRatioMode = m_forceAspectRatio ?
                    Qt::KeepAspectRatio : Qt::IgnoreAspectRatio;
            m_areas.calculate(targetArea, m_bufferFormat.frameSize(),
                    m_bufferFormat.pixelAspectRatio(), m_pixelAspectRatio,
                    aspectRatioMode);
            pixelAspectRatioLocker.unlock();

            GST_LOG_OBJECT(m_sink,
                "Recalculated paint areas: "
                "Frame size: " QSIZE_FORMAT ", "
                "target area: " QRECTF_FORMAT ", "
                "video area: " QRECTF_FORMAT ", "
                "black1: " QRECTF_FORMAT ", "
                "black2: " QRECTF_FORMAT,
                QSIZE_FORMAT_ARGS(m_bufferFormat.frameSize()),
                QRECTF_FORMAT_ARGS(m_areas.targetArea),
                QRECTF_FORMAT_ARGS(m_areas.videoArea),
                QRECTF_FORMAT_ARGS(m_areas.blackArea1),
                QRECTF_FORMAT_ARGS(m_areas.blackArea2)
            );
        }
        forceAspectRatioLocker.unlock();

        //if either pixelFormat or frameSize have changed, we need to reset the painter
        //and/or change painter, in case the current one does not handle the requested format
        if ((m_formatDirty) || !m_painter)
        {
            changePainter(m_bufferFormat);

            m_formatDirty = false;

            //make sure to update the colors after changing painter
            m_colorsDirty = true;
        }

        if (G_LIKELY(m_painter)) {
            QReadLocker colorsLocker(&m_colorsLock);
            if (m_colorsDirty) {
                m_painter->updateColors(m_brightness, m_contrast, m_hue, m_saturation);
                m_colorsDirty = false;
            }
            colorsLocker.unlock();

            GstMapInfo mem_info;
            if (gst_buffer_map(m_buffer, &mem_info, GST_MAP_READ)) {
                m_painter->paint(mem_info.data, m_bufferFormat, painter, m_areas);
                gst_buffer_unmap(m_buffer, &mem_info);
            }
        }
    }
}

#ifndef GST_QT_VIDEO_SINK_NO_OPENGL

QGLContext *QtVideoSinkDelegate::glContext() const
{
    return m_glContext;
}

void QtVideoSinkDelegate::setGLContext(QGLContext *context)
{
    if (m_glContext == context)
        return;

    m_glContext = context;
    m_supportedPainters = Generic;

    if (m_glContext) {
        m_glContext->makeCurrent();
        QOpenGLFunctionsDef *funcs = getQOpenGLFunctions();
        if (funcs) {
            const QByteArray extensions(reinterpret_cast<const char *>(funcs->glGetString(GL_EXTENSIONS)));
            GST_LOG_OBJECT(m_sink, "Available GL extensions: %s", extensions.constData());

#ifndef QT_OPENGL_ES
            if (extensions.contains("ARB_fragment_program"))
                m_supportedPainters |= ArbFp;
#endif

#ifndef QT_OPENGL_ES_2
            if (QGLShaderProgram::hasOpenGLShaderPrograms(m_glContext)
                && extensions.contains("ARB_shader_objects"))
#endif
                m_supportedPainters |= Glsl;
        }
    }

    GST_LOG_OBJECT(m_sink, "Done setting GL context. m_supportedPainters=%x", (int) m_supportedPainters);
}

#endif

void QtVideoSinkDelegate::changePainter(const BufferFormat & format)
{
    if (m_painter) {
        m_painter->cleanup();
        if (G_UNLIKELY(!m_painter->supportsFormat(format.videoFormat()))) {
            destroyPainter();
        }
    }

    QStack<PainterType> possiblePainters;
    if (GenericSurfacePainter::supportedPixelFormats().contains(format.videoFormat())) {
        possiblePainters.push(Generic);
    }

#ifndef GST_QT_VIDEO_SINK_NO_OPENGL
    if (OpenGLSurfacePainter::supportedPixelFormats().contains(format.videoFormat())) {
        if (m_supportedPainters & ArbFp) {
            possiblePainters.push(ArbFp);
        }

        if (m_supportedPainters & Glsl) {
            possiblePainters.push(Glsl);
        }
    }
#endif

    while (!possiblePainters.isEmpty()) {
        if (!m_painter) {
            PainterType type = possiblePainters.pop();
            switch(type) {
#ifndef GST_QT_VIDEO_SINK_NO_OPENGL
            case Glsl:
                GST_LOG_OBJECT(m_sink, "Creating GLSL painter");
                m_painter = new GlslSurfacePainter;
                break;
# ifndef QT_OPENGL_ES
            case ArbFp:
                GST_LOG_OBJECT(m_sink, "Creating ARB Fragment Shader painter");
                m_painter = new ArbFpSurfacePainter;
                break;
# endif
#endif
            case Generic:
                GST_LOG_OBJECT(m_sink, "Creating Generic painter");
                m_painter = new GenericSurfacePainter;
                break;
            default:
                Q_ASSERT(false);
            }
        }

        try {
            m_painter->init(format);
            return;
        } catch (const QString & error) {
            GST_ELEMENT_WARNING(m_sink, RESOURCE, FAILED,
                    ("Failed to start painter"), ("%s", error.toUtf8().constData()));
            delete m_painter;
            m_painter = 0;
        }
    }

    GST_ELEMENT_ERROR(m_sink, RESOURCE, FAILED,
            ("Failed to create a painter for the given format"), (NULL));
}

void QtVideoSinkDelegate::destroyPainter()
{
    GST_LOG_OBJECT(m_sink, "Destroying painter");
    delete m_painter;
    m_painter = 0;
}

bool QtVideoSinkDelegate::event(QEvent *event)
{
    if (event->type() == (QEvent::Type)DeactivateEventType) {
        if (m_painter) {
            m_painter->cleanup();
            destroyPainter();
        }
    }
    return BaseDelegate::event(event);
}
