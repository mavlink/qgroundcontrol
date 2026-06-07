// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOPENGL_CUSTOM_SHADER_STAGE_H
#define QOPENGL_CUSTOM_SHADER_STAGE_H

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

#include <QtOpenGL/qtopenglglobal.h>
#include <QOpenGLShaderProgram>
#include <private/qglobal_p.h>

QT_BEGIN_NAMESPACE


class QPainter;
class QOpenGLCustomShaderStagePrivate;
class Q_OPENGL_EXPORT QOpenGLCustomShaderStage
{
    Q_DECLARE_PRIVATE(QOpenGLCustomShaderStage)
public:
    QOpenGLCustomShaderStage();
    virtual ~QOpenGLCustomShaderStage();
    virtual void setUniforms(QOpenGLShaderProgram*) {}

    void setUniformsDirty();

    bool setOnPainter(QPainter*);
    void removeFromPainter(QPainter*);
    QByteArray source() const;

    void setInactive();
protected:
    void setSource(const QByteArray&);

private:
    QOpenGLCustomShaderStagePrivate* d_ptr;

    Q_DISABLE_COPY_MOVE(QOpenGLCustomShaderStage)
};


QT_END_NAMESPACE


#endif
