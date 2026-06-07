// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

#ifndef QOPENGLTEXTURECACHE_P_H
#define QOPENGLTEXTURECACHE_P_H

#include <QtOpenGL/qtopenglglobal.h>
#include <QHash>
#include <QObject>
#include <QCache>
#include <private/qopenglcontext_p.h>
#include <private/qopengltextureuploader_p.h>
#include <QtCore/qmutex.h>

QT_BEGIN_NAMESPACE

class QOpenGLCachedTexture;

class Q_OPENGL_EXPORT QOpenGLTextureCache : public QOpenGLSharedResource
{
public:
    static QOpenGLTextureCache *cacheForContext(QOpenGLContext *context);

    QOpenGLTextureCache(QOpenGLContext *);
    ~QOpenGLTextureCache();

    enum class BindResultFlag : quint8 {
        NewTexture = 0x01
    };
    Q_DECLARE_FLAGS(BindResultFlags, BindResultFlag)

    struct BindResult {
        GLuint id;
        BindResultFlags flags;
    };

    BindResult bindTexture(QOpenGLContext *context, const QPixmap &pixmap,
                           QOpenGLTextureUploader::BindOptions options = QOpenGLTextureUploader::PremultipliedAlphaBindOption);
    BindResult bindTexture(QOpenGLContext *context, const QImage &image,
                           QOpenGLTextureUploader::BindOptions options = QOpenGLTextureUploader::PremultipliedAlphaBindOption);

    void invalidate(qint64 key);

    void invalidateResource() override;
    void freeResource(QOpenGLContext *ctx) override;

private:
    BindResult bindTexture(QOpenGLContext *context, qint64 key, const QImage &image, QOpenGLTextureUploader::BindOptions options);

    QMutex m_mutex;
    QCache<quint64, QOpenGLCachedTexture> m_cache;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QOpenGLTextureCache::BindResultFlags)

class QOpenGLCachedTexture
{
public:
    QOpenGLCachedTexture(GLuint id, QOpenGLTextureUploader::BindOptions options, QOpenGLContext *context);
    ~QOpenGLCachedTexture() { m_resource->free(); }

    GLuint id() const { return m_resource->id(); }
    QOpenGLTextureUploader::BindOptions options() const { return m_options; }

private:
    QOpenGLSharedResourceGuard *m_resource;
    QOpenGLTextureUploader::BindOptions m_options;
};

QT_END_NAMESPACE

#endif

