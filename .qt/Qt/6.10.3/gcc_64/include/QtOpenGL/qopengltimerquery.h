// Copyright (C) 2013 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOPENGLTIMERQUERY_H
#define QOPENGLTIMERQUERY_H

#include <QtOpenGL/qtopenglglobal.h>

#if !defined(QT_NO_OPENGL) && !QT_CONFIG(opengles2)

#include <QtCore/QObject>
#include <QtGui/qopengl.h>

QT_BEGIN_NAMESPACE

class QOpenGLTimerQueryPrivate;

class Q_OPENGL_EXPORT QOpenGLTimerQuery : public QObject
{
    Q_OBJECT

public:
    explicit QOpenGLTimerQuery(QObject *parent = nullptr);
    ~QOpenGLTimerQuery();

    bool create();
    void destroy();
    bool isCreated() const;
    GLuint objectId() const;

    void begin();
    void end();
    GLuint64 waitForTimestamp() const;
    void recordTimestamp();
    bool isResultAvailable() const;
    GLuint64 waitForResult() const;

private:
    Q_DECLARE_PRIVATE(QOpenGLTimerQuery)
    Q_DISABLE_COPY(QOpenGLTimerQuery)
};


class QOpenGLTimeMonitorPrivate;

class Q_OPENGL_EXPORT QOpenGLTimeMonitor : public QObject
{
    Q_OBJECT

public:
    explicit QOpenGLTimeMonitor(QObject *parent = nullptr);
    ~QOpenGLTimeMonitor();

    void setSampleCount(int sampleCount);
    int sampleCount() const;

    bool create();
    void destroy();
    bool isCreated() const;
    QList<GLuint> objectIds() const;

    int recordSample();

    bool isResultAvailable() const;

    QList<GLuint64> waitForSamples() const;
    QList<GLuint64> waitForIntervals() const;

    void reset();

private:
    Q_DECLARE_PRIVATE(QOpenGLTimeMonitor)
    Q_DISABLE_COPY(QOpenGLTimeMonitor)
};

QT_END_NAMESPACE

#endif // QT_NO_OPENGL

#endif // QOPENGLTIMERQUERY_H
