// Copyright (C) 2013 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOPENGLQUERYHELPER_P_H
#define QOPENGLQUERYHELPER_P_H

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

#if !QT_CONFIG(opengles2)

#include <QtGui/QOpenGLContext>

QT_BEGIN_NAMESPACE

// Helper class used by QOpenGLTimerQuery and later will be used by
// QOpenGLOcclusionQuery
class QOpenGLQueryHelper
{
public:
    QOpenGLQueryHelper(QOpenGLContext *context)
        : GetQueryObjectuiv(nullptr),
          GetQueryObjectiv(nullptr),
          GetQueryiv(nullptr),
          EndQuery(nullptr),
          BeginQuery(nullptr),
          IsQuery(nullptr),
          DeleteQueries(nullptr),
          GenQueries(nullptr),
          GetInteger64v(nullptr),
          GetQueryObjectui64v(nullptr),
          GetQueryObjecti64v(nullptr),
          QueryCounter(nullptr)
    {
        Q_ASSERT(context);

        // Core in OpenGL >=1.5
        GetQueryObjectuiv = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLuint , GLenum , GLuint *)>(context->getProcAddress("glGetQueryObjectuiv"));
        GetQueryObjectiv = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLuint , GLenum , GLint *)>(context->getProcAddress("glGetQueryObjectiv"));
        GetQueryiv = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum , GLenum , GLint *)>(context->getProcAddress("glGetQueryiv"));
        EndQuery = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum )>(context->getProcAddress("glEndQuery"));
        BeginQuery = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum , GLuint )>(context->getProcAddress("glBeginQuery"));
        IsQuery = reinterpret_cast<GLboolean (QOPENGLF_APIENTRYP)(GLuint )>(context->getProcAddress("glIsQuery"));
        DeleteQueries = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLsizei , const GLuint *)>(context->getProcAddress("glDeleteQueries"));
        GenQueries = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLsizei , GLuint *)>(context->getProcAddress("glGenQueries"));

        // Core in OpenGL >=3.2
        GetInteger64v = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum , GLint64 *)>(context->getProcAddress("glGetInteger64v"));

        // Core in OpenGL >=3.3 / ARB_timer_query
        GetQueryObjectui64v = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLuint , GLenum , GLuint64 *)>(context->getProcAddress("glGetQueryObjectui64v"));
        GetQueryObjecti64v = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLuint , GLenum , GLint64 *)>(context->getProcAddress("glGetQueryObjecti64v"));
        QueryCounter = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLuint , GLenum )>(context->getProcAddress("glQueryCounter"));
    }

    inline void glGetQueryObjectuiv(GLuint id, GLenum pname, GLuint *params)
    {
        GetQueryObjectuiv(id, pname, params);
    }

    inline void glGetQueryObjectiv(GLuint id, GLenum pname, GLint *params)
    {
        GetQueryObjectiv(id, pname, params);
    }

    inline void glGetQueryiv(GLenum target, GLenum pname, GLint *params)
    {
        GetQueryiv(target, pname, params);
    }

    inline void glEndQuery(GLenum target)
    {
        EndQuery(target);
    }

    inline void glBeginQuery(GLenum target, GLuint id)
    {
        BeginQuery(target, id);
    }

    inline GLboolean glIsQuery(GLuint id)
    {
        return IsQuery(id);
    }

    inline void glDeleteQueries(GLsizei n, const GLuint *ids)
    {
        DeleteQueries(n, ids);
    }

    inline void glGenQueries(GLsizei n, GLuint *ids)
    {
        GenQueries(n, ids);
    }

    inline void glGetInteger64v(GLenum pname, GLint64 *params)
    {
        GetInteger64v(pname, params);
    }

    inline void glGetQueryObjectui64v(GLuint id, GLenum pname, GLuint64 *params)
    {
        GetQueryObjectui64v(id, pname, params);
    }

    inline void glGetQueryObjecti64v(GLuint id, GLenum pname, GLint64 *params)
    {
        GetQueryObjecti64v(id, pname, params);
    }

    inline void glQueryCounter(GLuint id, GLenum target)
    {
        QueryCounter(id, target);
    }

private:
    // Core in OpenGL >=1.5
    void (QOPENGLF_APIENTRYP GetQueryObjectuiv)(GLuint id, GLenum pname, GLuint *params);
    void (QOPENGLF_APIENTRYP GetQueryObjectiv)(GLuint id, GLenum pname, GLint *params);
    void (QOPENGLF_APIENTRYP GetQueryiv)(GLenum target, GLenum pname, GLint *params);
    void (QOPENGLF_APIENTRYP EndQuery)(GLenum target);
    void (QOPENGLF_APIENTRYP BeginQuery)(GLenum target, GLuint id);
    GLboolean (QOPENGLF_APIENTRYP IsQuery)(GLuint id);
    void (QOPENGLF_APIENTRYP DeleteQueries)(GLsizei n, const GLuint *ids);
    void (QOPENGLF_APIENTRYP GenQueries)(GLsizei n, GLuint *ids);

    // Core in OpenGL >=3.2
    void (QOPENGLF_APIENTRYP GetInteger64v)(GLenum pname, GLint64 *params);

    // Core in OpenGL >=3.3 and provided by ARB_timer_query
    void (QOPENGLF_APIENTRYP GetQueryObjectui64v)(GLuint id, GLenum pname, GLuint64 *params);
    void (QOPENGLF_APIENTRYP GetQueryObjecti64v)(GLuint id, GLenum pname, GLint64 *params);
    void (QOPENGLF_APIENTRYP QueryCounter)(GLuint id, GLenum target);
};

QT_END_NAMESPACE

#endif

#endif // QOPENGLQUERYHELPER_P_H
