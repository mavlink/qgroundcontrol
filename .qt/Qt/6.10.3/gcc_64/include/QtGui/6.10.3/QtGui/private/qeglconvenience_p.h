// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QEGLCONVENIENCE_H
#define QEGLCONVENIENCE_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/qsurfaceformat.h>
#include <QtCore/qlist.h>
#include <QtCore/qsize.h>

#include <QtGui/private/qt_egl_p.h>


QT_BEGIN_NAMESPACE

Q_GUI_EXPORT QList<EGLint> q_createConfigAttributesFromFormat(const QSurfaceFormat &format);

Q_GUI_EXPORT bool q_reduceConfigAttributes(QList<EGLint> *configAttributes);

Q_GUI_EXPORT EGLConfig q_configFromGLFormat(EGLDisplay display,
                                               const QSurfaceFormat &format,
                                               bool highestPixelFormat = false,
                                               int surfaceType = EGL_WINDOW_BIT);

Q_GUI_EXPORT QSurfaceFormat q_glFormatFromConfig(EGLDisplay display, const EGLConfig config,
                                                    const QSurfaceFormat &referenceFormat = {});

Q_GUI_EXPORT bool q_hasEglExtension(EGLDisplay display,const char* extensionName);

Q_GUI_EXPORT void q_printEglConfig(EGLDisplay display, EGLConfig config);

#ifdef Q_OS_UNIX
Q_GUI_EXPORT QSizeF q_physicalScreenSizeFromFb(int framebufferDevice,
                                                  const QSize &screenSize = {});

Q_GUI_EXPORT  QSize q_screenSizeFromFb(int framebufferDevice);

Q_GUI_EXPORT int q_screenDepthFromFb(int framebufferDevice);

Q_GUI_EXPORT  qreal q_refreshRateFromFb(int framebufferDevice);

#endif

class Q_GUI_EXPORT QEglConfigChooser
{
public:
    QEglConfigChooser(EGLDisplay display);
    virtual ~QEglConfigChooser();

    EGLDisplay display() const { return m_display; }

    void setSurfaceType(EGLint surfaceType) { m_surfaceType = surfaceType; }
    EGLint surfaceType() const { return m_surfaceType; }

    void setSurfaceFormat(const QSurfaceFormat &format) { m_format = format; }
    QSurfaceFormat surfaceFormat() const { return m_format; }

    void setIgnoreColorChannels(bool ignore) { m_ignore = ignore; }
    bool ignoreColorChannels() const { return m_ignore; }

    EGLConfig chooseConfig();

protected:
    virtual bool filterConfig(EGLConfig config) const;

    QSurfaceFormat m_format;
    EGLDisplay m_display;
    EGLint m_surfaceType;
    bool m_ignore;

    int m_confAttrRed;
    int m_confAttrGreen;
    int m_confAttrBlue;
    int m_confAttrAlpha;
};


QT_END_NAMESPACE

#endif //QEGLCONVENIENCE_H
