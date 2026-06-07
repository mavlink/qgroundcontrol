// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOPENGLGRADIENTCACHE_P_H
#define QOPENGLGRADIENTCACHE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QMultiHash>
#include <QObject>
#include <private/qopenglcontext_p.h>
#include <QtCore/qmutex.h>
#include <QGradient>
#include <qrgba64.h>

QT_BEGIN_NAMESPACE

class QOpenGL2GradientCache : public QOpenGLSharedResource
{
    struct CacheInfo
    {
        inline CacheInfo(QGradientStops s, qreal op, QGradient::InterpolationMode mode) :
            stops(std::move(s)), opacity(op), interpolationMode(mode) {}

        GLuint texId;
        QGradientStops stops;
        qreal opacity;
        QGradient::InterpolationMode interpolationMode;
    };

    typedef QMultiHash<quint64, CacheInfo> QOpenGLGradientColorTableHash;

public:
    static QOpenGL2GradientCache *cacheForContext(QOpenGLContext *context);

    QOpenGL2GradientCache(QOpenGLContext *);
    ~QOpenGL2GradientCache();

    GLuint getBuffer(const QGradient &gradient, qreal opacity);
    inline int paletteSize() const { return 1024; }

    void invalidateResource() override;
    void freeResource(QOpenGLContext *ctx) override;

private:
    inline int maxCacheSize() const { return 60; }
    inline void generateGradientColorTable(const QGradient& gradient,
                                           QRgba64 *colorTable,
                                           int size, qreal opacity) const;
    inline void generateGradientColorTable(const QGradient& gradient,
                                           uint *colorTable,
                                           int size, qreal opacity) const;
    GLuint addCacheElement(quint64 hash_val, const QGradient &gradient, qreal opacity);
    void cleanCache();

    QOpenGLGradientColorTableHash cache;
    QMutex m_mutex;
};

QT_END_NAMESPACE

#endif // QOPENGLGRADIENTCACHE_P_H
