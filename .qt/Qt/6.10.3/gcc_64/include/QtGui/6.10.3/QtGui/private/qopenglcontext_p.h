// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOPENGLCONTEXT_P_H
#define QOPENGLCONTEXT_P_H

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

#include <QtGui/private/qtguiglobal_p.h>

#ifndef QT_NO_OPENGL

#include <qopengl.h>
#include "qopenglcontext.h"
#include <private/qobject_p.h>
#include <qmutex.h>

#include <QtCore/QByteArray>
#include <QtCore/QHash>
#include <QtCore/QSet>

QT_BEGIN_NAMESPACE


class QOpenGLFunctions;
class QOpenGLContext;
class QOpenGLFramebufferObject;
class QOpenGLMultiGroupSharedResource;

class Q_GUI_EXPORT QOpenGLSharedResource
{
public:
    QOpenGLSharedResource(QOpenGLContextGroup *group);
    virtual ~QOpenGLSharedResource() = 0;

    QOpenGLContextGroup *group() const { return m_group; }

    // schedule the resource for deletion at an appropriate time
    void free();

protected:
    // the resource's share group no longer exists, invalidate the resource
    virtual void invalidateResource() = 0;

    // a valid context in the group is current, free the resource
    virtual void freeResource(QOpenGLContext *context) = 0;

private:
    QOpenGLContextGroup *m_group;

    friend class QOpenGLContextGroup;
    friend class QOpenGLContextGroupPrivate;
    friend class QOpenGLMultiGroupSharedResource;

    Q_DISABLE_COPY_MOVE(QOpenGLSharedResource)
};

class Q_GUI_EXPORT QOpenGLSharedResourceGuard : public QOpenGLSharedResource
{
public:
    typedef void (*FreeResourceFunc)(QOpenGLFunctions *functions, GLuint id);
    QOpenGLSharedResourceGuard(QOpenGLContext *context, GLuint id, FreeResourceFunc func)
        : QOpenGLSharedResource(context->shareGroup())
        , m_id(id)
        , m_func(func)
    {
    }
    ~QOpenGLSharedResourceGuard() override;

    GLuint id() const { return m_id; }

protected:
    void invalidateResource() override
    {
        m_id = 0;
    }

    void freeResource(QOpenGLContext *context) override;

private:
    GLuint m_id;
    FreeResourceFunc m_func;
};

class Q_GUI_EXPORT QOpenGLContextGroupPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QOpenGLContextGroup)
public:
    QOpenGLContextGroupPrivate()
        : m_context(nullptr)
        , m_refs(0)
    {
    }
    ~QOpenGLContextGroupPrivate() override;

    void addContext(QOpenGLContext *ctx);
    void removeContext(QOpenGLContext *ctx);

    void cleanup();

    void deletePendingResources(QOpenGLContext *ctx);

    QOpenGLContext *m_context;

    QList<QOpenGLContext *> m_shares;
    QRecursiveMutex m_mutex;

    QHash<QOpenGLMultiGroupSharedResource *, QOpenGLSharedResource *> m_resources;
    QAtomicInt m_refs;

    QList<QOpenGLSharedResource *> m_sharedResources;
    QList<QOpenGLSharedResource *> m_pendingDeletion;
};

class Q_GUI_EXPORT QOpenGLMultiGroupSharedResource
{
public:
    QOpenGLMultiGroupSharedResource();
    ~QOpenGLMultiGroupSharedResource();

    void insert(QOpenGLContext *context, QOpenGLSharedResource *value);
    void cleanup(QOpenGLContextGroup *group, QOpenGLSharedResource *value);

    QOpenGLSharedResource *value(QOpenGLContext *context);

    QList<QOpenGLSharedResource *> resources() const;

    template <typename T>
    T *value(QOpenGLContext *context) {
        QOpenGLContextGroup *group = context->shareGroup();
        // Have to use our own mutex here, not the group's, since
        // m_groups has to be protected too against any concurrent access.
        QMutexLocker locker(&m_mutex);
        T *resource = static_cast<T *>(group->d_func()->m_resources.value(this, nullptr));
        if (!resource) {
            resource = new T(context);
            insert(context, resource);
        }
        return resource;
    }

private:
    QAtomicInt active;
    QList<QOpenGLContextGroup *> m_groups;
    QRecursiveMutex m_mutex;
};

class QPaintEngineEx;
class QOpenGLFunctions;
class QOpenGLTextureHelper;
class QOpenGLVertexArrayObjectHelper;

class Q_GUI_EXPORT QOpenGLContextVersionFunctionHelper
{
public:
    virtual ~QOpenGLContextVersionFunctionHelper();
};

class Q_GUI_EXPORT QOpenGLContextPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QOpenGLContext)
public:
    QOpenGLContextPrivate()
        : platformGLContext(nullptr)
        , shareContext(nullptr)
        , shareGroup(nullptr)
        , screen(nullptr)
        , surface(nullptr)
        , functions(nullptr)
        , textureFunctions(nullptr)
        , versionFunctions(nullptr)
        , vaoHelper(nullptr)
        , vaoHelperDestroyCallback(nullptr)
        , max_texture_size(-1)
        , workaround_brokenFBOReadBack(false)
        , workaround_brokenTexSubImage(false)
        , workaround_missingPrecisionQualifiers(false)
        , active_engine(nullptr)
        , qgl_current_fbo_invalid(false)
        , qgl_current_fbo(nullptr)
        , defaultFboRedirect(0)
    {
        requestedFormat = QSurfaceFormat::defaultFormat();
    }

    ~QOpenGLContextPrivate() override;

    void adopt(QPlatformOpenGLContext *);

    QSurfaceFormat requestedFormat;
    QPlatformOpenGLContext *platformGLContext;
    QOpenGLContext *shareContext;
    QOpenGLContextGroup *shareGroup;
    QScreen *screen;
    QSurface *surface;
    QOpenGLFunctions *functions;
    mutable QSet<QByteArray> extensionNames;
    QOpenGLTextureHelper* textureFunctions;
    std::function<void()> textureFunctionsDestroyCallback;
    QOpenGLContextVersionFunctionHelper *versionFunctions;
    QOpenGLVertexArrayObjectHelper *vaoHelper;
    using QOpenGLVertexArrayObjectHelperDestroyCallback_t = void (*)(QOpenGLVertexArrayObjectHelper *);
    QOpenGLVertexArrayObjectHelperDestroyCallback_t vaoHelperDestroyCallback;

    GLint max_texture_size;

    bool workaround_brokenFBOReadBack;
    bool workaround_brokenTexSubImage;
    bool workaround_missingPrecisionQualifiers;

    QPaintEngineEx *active_engine;

    bool qgl_current_fbo_invalid;

    // Set and unset in QOpenGLFramebufferObject::bind()/unbind().
    // (Only meaningful for QOGLFBO since an FBO might be bound by other means)
    // Saves us from querying the driver for the current FBO in most paths.
    QOpenGLFramebufferObject *qgl_current_fbo;

    GLuint defaultFboRedirect;

    static QOpenGLContext *setCurrentContext(QOpenGLContext *context);

    int maxTextureSize();

    static QOpenGLContextPrivate *get(QOpenGLContext *context)
    {
        return context ? context->d_func() : nullptr;
    }

#if !defined(QT_NO_DEBUG)
    static bool toggleMakeCurrentTracker(QOpenGLContext *context, bool value)
    {
        QMutexLocker locker(&makeCurrentTrackerMutex);
        bool old = makeCurrentTracker.value(context, false);
        makeCurrentTracker.insert(context, value);
        return old;
    }
    static void cleanMakeCurrentTracker(QOpenGLContext *context)
    {
        QMutexLocker locker(&makeCurrentTrackerMutex);
        makeCurrentTracker.remove(context);
    }
    static QHash<QOpenGLContext *, bool> makeCurrentTracker;
    static QMutex makeCurrentTrackerMutex;
#endif

    void _q_screenDestroyed(QObject *object);
};

Q_GUI_EXPORT void qt_gl_set_global_share_context(QOpenGLContext *context);
Q_GUI_EXPORT QOpenGLContext *qt_gl_global_share_context();

QT_END_NAMESPACE

#endif // QT_NO_OPENGL
#endif // QOPENGLCONTEXT_P_H
