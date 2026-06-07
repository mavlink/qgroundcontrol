// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKFRAMEANIMATION_H
#define QQUICKFRAMEANIMATION_H

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

#include <qqml.h>
#include <QtCore/qobject.h>
#include <private/qtqmlglobal_p.h>
#include <private/qtquickglobal_p.h>

QT_BEGIN_NAMESPACE

class QQuickFrameAnimationPrivate;
class Q_QUICK_EXPORT QQuickFrameAnimation : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickFrameAnimation)
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(bool running READ isRunning WRITE setRunning NOTIFY runningChanged)
    Q_PROPERTY(bool paused READ isPaused WRITE setPaused NOTIFY pausedChanged)
    Q_PROPERTY(int currentFrame READ currentFrame NOTIFY currentFrameChanged)
    Q_PROPERTY(qreal frameTime READ frameTime NOTIFY frameTimeChanged)
    Q_PROPERTY(qreal smoothFrameTime READ smoothFrameTime NOTIFY smoothFrameTimeChanged)
    Q_PROPERTY(qreal elapsedTime READ elapsedTime NOTIFY elapsedTimeChanged)
    QML_NAMED_ELEMENT(FrameAnimation)
    QML_ADDED_IN_VERSION(6, 4)

public:
    QQuickFrameAnimation(QObject *parent = nullptr);

    bool isRunning() const;
    void setRunning(bool running);

    bool isPaused() const;
    void setPaused(bool paused);

    int currentFrame() const;
    qreal frameTime() const;
    qreal smoothFrameTime() const;
    qreal elapsedTime() const;

protected:
    void classBegin() override;
    void componentComplete() override;

public Q_SLOTS:
    void start();
    void stop();
    void restart();
    void pause();
    void resume();
    void reset();

Q_SIGNALS:
    void triggered();
    void runningChanged();
    void pausedChanged();
    void currentFrameChanged();
    void frameTimeChanged();
    void smoothFrameTimeChanged();
    void elapsedTimeChanged();

private:
    void setCurrentFrame(int frame);
    void setElapsedTime(qreal elapsedTime);

};

QT_END_NAMESPACE

#endif
