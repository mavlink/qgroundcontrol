// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QSTYLEANIMATION_P_H
#define QSTYLEANIMATION_P_H

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include "qabstractanimation.h"
#include "qdatetime.h"
#include "qimage.h"

QT_REQUIRE_CONFIG(animation);

QT_BEGIN_NAMESPACE

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists for the convenience of
// qcommonstyle.cpp.  This header file may change from version to version
// without notice, or even be removed.
//
// We mean it.
//

class Q_WIDGETS_EXPORT QStyleAnimation : public QAbstractAnimation
{
    Q_OBJECT

public:
    QStyleAnimation(QObject *target);
    virtual ~QStyleAnimation();

    QObject *target() const;

    int duration() const override;
    void setDuration(int duration);

    int delay() const;
    void setDelay(int delay);

    QTime startTime() const;
    void setStartTime(QTime time);

    enum FrameRate {
        DefaultFps,
        SixtyFps,
        ThirtyFps,
        TwentyFps,
        FifteenFps
    };

    FrameRate frameRate() const;
    void setFrameRate(FrameRate fps);

    void updateTarget();

public Q_SLOTS:
    void start();

protected:
    virtual bool isUpdateNeeded() const;
    virtual void updateCurrentTime(int time) override;

private:
    int _delay;
    int _duration;
    QTime _startTime;
    FrameRate _fps;
    int _skip;
};

class Q_WIDGETS_EXPORT QProgressStyleAnimation : public QStyleAnimation
{
    Q_OBJECT

public:
    QProgressStyleAnimation(int speed, QObject *target);

    int animationStep() const;
    int progressStep(int width) const;

    int speed() const;
    void setSpeed(int speed);

protected:
    bool isUpdateNeeded() const override;

private:
    int _speed;
    mutable int _step;
};

class Q_WIDGETS_EXPORT QNumberStyleAnimation : public QStyleAnimation
{
    Q_OBJECT

public:
    QNumberStyleAnimation(QObject *target);

    qreal startValue() const;
    void setStartValue(qreal value);

    qreal endValue() const;
    void setEndValue(qreal value);

    qreal currentValue() const;

protected:
    bool isUpdateNeeded() const override;

private:
    qreal _start;
    qreal _end;
    mutable qreal _prev;
};

class Q_WIDGETS_EXPORT QBlendStyleAnimation : public QStyleAnimation
{
    Q_OBJECT

public:
    enum Type { Transition, Pulse };

    QBlendStyleAnimation(Type type, QObject *target);

    QImage startImage() const;
    void setStartImage(const QImage& image);

    QImage endImage() const;
    void setEndImage(const QImage& image);

    QImage currentImage() const;

protected:
    virtual void updateCurrentTime(int time) override;

private:
    Type _type;
    QImage _start;
    QImage _end;
    QImage _current;
};

class Q_WIDGETS_EXPORT QScrollbarStyleAnimation : public QNumberStyleAnimation
{
    Q_OBJECT

public:
    enum Mode { Activating, Deactivating };

    QScrollbarStyleAnimation(Mode mode, QObject *target);

    Mode mode() const;

    bool wasActive() const;
    void setActive(bool active);

private slots:
    void updateCurrentTime(int time) override;

private:
    Mode _mode;
    bool _active;
};

QT_END_NAMESPACE

#endif // QSTYLEANIMATION_P_H
