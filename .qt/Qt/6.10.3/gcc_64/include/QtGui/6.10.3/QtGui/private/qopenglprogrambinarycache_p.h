// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOPENGLPROGRAMBINARYCACHE_P_H
#define QOPENGLPROGRAMBINARYCACHE_P_H

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

#include <QtGui/qtguiglobal.h>
#include <QtCore/qcache.h>
#include <QtCore/qmutex.h>
#include <QtCore/QLoggingCategory>
#include <QtGui/private/qopenglcontext_p.h>
#include <rhi/qshader.h>

QT_BEGIN_NAMESPACE

// These classes are also used by the OpenGL backend of QRhi. They must
// therefore stay independent from QOpenGLShader(Program). Must rely only on
// QOpenGLContext/Functions.

QT_DECLARE_EXPORTED_QT_LOGGING_CATEGORY(lcOpenGLProgramDiskCache, Q_GUI_EXPORT)

class Q_GUI_EXPORT QOpenGLProgramBinaryCache
{
public:
    struct Q_GUI_EXPORT ShaderDesc {
        ShaderDesc() { }
        ShaderDesc(QShader::Stage stage, const QByteArray &source = QByteArray())
          : stage(stage), source(source)
        { }
        QShader::Stage stage;
        QByteArray source;
    };
    struct Q_GUI_EXPORT ProgramDesc {
        QList<ShaderDesc> shaders;
        QByteArray cacheKey() const;
    };

    QOpenGLProgramBinaryCache();

    bool load(const QByteArray &cacheKey, uint programId);
    void save(const QByteArray &cacheKey, uint programId);

private:
    QString cacheFileName(const QByteArray &cacheKey) const;
    bool verifyHeader(const QByteArray &buf) const;
    bool setProgramBinary(uint programId, uint blobFormat, const void *p, uint blobSize);

    QString m_globalCacheDir;
    QString m_localCacheDir;
    QString m_currentCacheDir;
    bool m_cacheWritable;
    struct MemCacheEntry {
        MemCacheEntry(const void *p, int size, uint format)
          : blob(reinterpret_cast<const char *>(p), size),
            format(format)
        { }
        QByteArray blob;
        uint format;
    };
    QCache<QByteArray, MemCacheEntry> m_memCache;
#if QT_CONFIG(opengles2)
    void (QOPENGLF_APIENTRYP programBinaryOES)(GLuint program, GLenum binaryFormat, const GLvoid *binary, GLsizei length);
    void (QOPENGLF_APIENTRYP getProgramBinaryOES)(GLuint program, GLsizei bufSize, GLsizei *length, GLenum *binaryFormat, GLvoid *binary);
    void initializeProgramBinaryOES(QOpenGLContext *context);
    bool m_programBinaryOESInitialized = false;
#endif
    QMutex m_mutex;
};

// While unlikely, one application can in theory use contexts with different versions
// or profiles. Therefore any version- or extension-specific checks must be done on a
// per-context basis, not just once per process. QOpenGLSharedResource enables this,
// although it's once-per-sharing-context-group, not per-context. Still, this should
// be good enough in practice.
class Q_GUI_EXPORT QOpenGLProgramBinarySupportCheck : public QOpenGLSharedResource
{
public:
    QOpenGLProgramBinarySupportCheck(QOpenGLContext *context);
    void invalidateResource() override { }
    void freeResource(QOpenGLContext *) override { }

    bool isSupported() const { return m_supported; }

private:
    bool m_supported;
};

class QOpenGLProgramBinarySupportCheckWrapper
{
public:
    QOpenGLProgramBinarySupportCheck *get(QOpenGLContext *context)
    {
        return m_resource.value<QOpenGLProgramBinarySupportCheck>(context);
    }

private:
    QOpenGLMultiGroupSharedResource m_resource;
};

QT_END_NAMESPACE

#endif
