// Copyright (C) 2019 The Qt Company Ltd.
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

#ifndef QOPENGLSHADERCACHE_P_H
#define QOPENGLSHADERCACHE_P_H

#include <QtOpenGL/qtopenglglobal.h>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE


class QOpenGLShaderProgram;
class QOpenGLContext;

class CachedShader
{
public:
    inline CachedShader(const QByteArray &, const QByteArray &)
    {}

    inline bool isCached()
    {
        return false;
    }

    inline bool load(QOpenGLShaderProgram *, QOpenGLContext *)
    {
        return false;
    }

    inline bool store(QOpenGLShaderProgram *, QOpenGLContext *)
    {
        return false;
    }
};

QT_END_NAMESPACE

#endif
