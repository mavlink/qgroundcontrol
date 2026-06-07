// Copyright (C) 2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Sean Harmer <sean.harmer@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOPENGLVERTEXARRAYOBJECT_P_H
#define QOPENGLVERTEXARRAYOBJECT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the Qt OpenGL classes.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtOpenGL/qtopenglglobal.h>

#include <QtGui/qopengl.h>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class QOpenGLContext;

class QOpenGLVertexArrayObjectHelper
{
    Q_DISABLE_COPY(QOpenGLVertexArrayObjectHelper)

private:
    explicit inline QOpenGLVertexArrayObjectHelper(QOpenGLContext *context)
        : GenVertexArrays(nullptr)
        , DeleteVertexArrays(nullptr)
        , BindVertexArray(nullptr)
        , IsVertexArray(nullptr)
    {
        initializeFromContext(context);
    }

    void Q_OPENGL_EXPORT initializeFromContext(QOpenGLContext *context);

public:
    static Q_OPENGL_EXPORT QOpenGLVertexArrayObjectHelper *vertexArrayObjectHelperForContext(QOpenGLContext *context);

    inline bool isValid() const
    {
        return GenVertexArrays && DeleteVertexArrays && BindVertexArray && IsVertexArray;
    }

    inline void glGenVertexArrays(GLsizei n, GLuint *arrays) const
    {
        GenVertexArrays(n, arrays);
    }

    inline void glDeleteVertexArrays(GLsizei n, const GLuint *arrays) const
    {
        DeleteVertexArrays(n, arrays);
    }

    inline void glBindVertexArray(GLuint array) const
    {
        BindVertexArray(array);
    }

    inline GLboolean glIsVertexArray(GLuint array) const
    {
        return IsVertexArray(array);
    }

    // Function signatures are equivalent between desktop core, ARB, APPLE, ES 3 and ES 2 extensions
    typedef void (QOPENGLF_APIENTRYP qt_GenVertexArrays_t)(GLsizei n, GLuint *arrays);
    typedef void (QOPENGLF_APIENTRYP qt_DeleteVertexArrays_t)(GLsizei n, const GLuint *arrays);
    typedef void (QOPENGLF_APIENTRYP qt_BindVertexArray_t)(GLuint array);
    typedef GLboolean (QOPENGLF_APIENTRYP qt_IsVertexArray_t)(GLuint array);

    qt_GenVertexArrays_t GenVertexArrays;
    qt_DeleteVertexArrays_t DeleteVertexArrays;
    qt_BindVertexArray_t BindVertexArray;
    qt_IsVertexArray_t IsVertexArray;
};

QT_END_NAMESPACE

#endif // QOPENGLVERTEXARRAYOBJECT_P_H
