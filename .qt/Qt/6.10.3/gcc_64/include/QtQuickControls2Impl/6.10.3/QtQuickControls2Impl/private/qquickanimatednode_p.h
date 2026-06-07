// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKANIMATEDNODE_P_H
#define QQUICKANIMATEDNODE_P_H

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

#include <QtCore/qobject.h>
#include <QtQuick/qsgnode.h>
#include <QtCore/qelapsedtimer.h>
#include <QtQuickControls2Impl/private/qtquickcontrols2implglobal_p.h>

QT_BEGIN_NAMESPACE

class QQuickItem;
class QQuickWindow;

class Q_QUICKCONTROLS2IMPL_EXPORT QQuickAnimatedNode : public QObject, public QSGTransformNode
{
    Q_OBJECT

public:
    explicit QQuickAnimatedNode(QQuickItem *target);

    bool isRunning() const;

    int currentTime() const;
    void setCurrentTime(int time);

    int duration() const;
    void setDuration(int duration);

    enum LoopCount { Infinite = -1 };

    int loopCount() const;
    void setLoopCount(int count);

    virtual void sync(QQuickItem *target);

    QQuickWindow *window() const;

    // must be called from sync() or updatePaintNode()
    void start(int duration = 0);
    void restart();
    void stop();

Q_SIGNALS:
    void started();
    void stopped();

protected:
    virtual void updateCurrentTime(int time);

private Q_SLOTS:
    void advance();
    void update();

private:
    bool m_running = false;
    int m_duration = 0;
    int m_loopCount = 1;
    int m_currentTime = 0;
    int m_currentLoop = 0;
    QElapsedTimer m_timer;
    QQuickWindow *m_window = nullptr;
};

QT_END_NAMESPACE

#endif // QQUICKANIMATEDNODE_P_H
