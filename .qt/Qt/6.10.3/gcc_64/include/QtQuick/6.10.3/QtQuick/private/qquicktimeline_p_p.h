// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKTIMELINE_H
#define QQUICKTIMELINE_H

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

#include <QtCore/QObject>
#include <private/qtquickglobal_p.h>
#include "private/qabstractanimationjob_p.h"

QT_BEGIN_NAMESPACE

class QEasingCurve;
class QQuickTimeLineValue;
class QQuickTimeLineCallback;
struct QQuickTimeLinePrivate;
class QQuickTimeLineObject;
class Q_QUICK_EXPORT QQuickTimeLine : public QObject, QAbstractAnimationJob
{
Q_OBJECT
public:
    QQuickTimeLine(QObject *parent = nullptr);
    ~QQuickTimeLine();

    enum SyncMode { LocalSync, GlobalSync };
    SyncMode syncMode() const;
    void setSyncMode(SyncMode);

    void pause(QQuickTimeLineObject &, int);
    void callback(const QQuickTimeLineCallback &);
    void set(QQuickTimeLineValue &, qreal);

    int accel(QQuickTimeLineValue &, qreal velocity, qreal accel);
    int accel(QQuickTimeLineValue &, qreal velocity, qreal accel, qreal maxDistance);
    int accelDistance(QQuickTimeLineValue &, qreal velocity, qreal distance);

    void move(QQuickTimeLineValue &, qreal destination, int time = 500);
    void move(QQuickTimeLineValue &, qreal destination, const QEasingCurve &, int time = 500);
    void moveBy(QQuickTimeLineValue &, qreal change, int time = 500);
    void moveBy(QQuickTimeLineValue &, qreal change, const QEasingCurve &, int time = 500);

    void sync();
    void setSyncPoint(int);
    int syncPoint() const;

    void sync(QQuickTimeLineValue &);
    void sync(QQuickTimeLineValue &, QQuickTimeLineValue &);

    void reset(QQuickTimeLineValue &);

    void complete();
    void clear();
    bool isActive() const;

    int time() const;

    int duration() const override;
Q_SIGNALS:
    void updated();
    void completed();

protected:
    void updateCurrentTime(int) override;
    void debugAnimation(QDebug d) const override;

private:
    void remove(QQuickTimeLineObject *);
    friend class QQuickTimeLineObject;
    friend struct QQuickTimeLinePrivate;
    QQuickTimeLinePrivate *d;
};

class Q_AUTOTEST_EXPORT QQuickTimeLineObject
{
public:
    QQuickTimeLineObject();
    virtual ~QQuickTimeLineObject();

protected:
    friend class QQuickTimeLine;
    friend struct QQuickTimeLinePrivate;
    QQuickTimeLine *_t;
};

class Q_AUTOTEST_EXPORT QQuickTimeLineValue : public QQuickTimeLineObject
{
public:
    QQuickTimeLineValue(qreal v = 0.) : _v(v) {}

    virtual qreal value() const { return _v; }
    virtual void setValue(qreal v) { _v = v; }

    QQuickTimeLine *timeLine() const { return _t; }

    operator qreal() const { return _v; }
    QQuickTimeLineValue &operator=(qreal v) { setValue(v); return *this; }
private:
    friend class QQuickTimeLine;
    friend struct QQuickTimeLinePrivate;
    qreal _v;
};

class Q_AUTOTEST_EXPORT QQuickTimeLineCallback
{
public:
    typedef void (*Callback)(void *);

    QQuickTimeLineCallback();
    QQuickTimeLineCallback(QQuickTimeLineObject *b, Callback, void * = nullptr);
    QQuickTimeLineCallback(const QQuickTimeLineCallback &o);

    QQuickTimeLineCallback &operator=(const QQuickTimeLineCallback &o);
    QQuickTimeLineObject *callbackObject() const;

private:
    friend struct QQuickTimeLinePrivate;
    Callback d0;
    void *d1;
    QQuickTimeLineObject *d2;
};

template<class T>
class QQuickTimeLineValueProxy : public QQuickTimeLineValue
{
public:
    QQuickTimeLineValueProxy(T *object, void (T::*func)(qreal), qreal v = 0.)
    : QQuickTimeLineValue(v), object(object), setter(func)
    {
        Q_ASSERT(object);
    }

    void setValue(qreal v) override
    {
        QQuickTimeLineValue::setValue(v);
        (object->*setter)(v);
    }

private:
    T *object;
    void (T::*setter)(qreal);
};

QT_END_NAMESPACE

#endif
