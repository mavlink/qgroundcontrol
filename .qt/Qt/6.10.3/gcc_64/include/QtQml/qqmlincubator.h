// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLINCUBATOR_H
#define QQMLINCUBATOR_H

#include <QtQml/qtqmlglobal.h>
#include <QtQml/qqmlerror.h>
#include <atomic>

QT_BEGIN_NAMESPACE


class QQmlEngine;
class QQmlPropertyData;
class QVariant;

class QQmlIncubatorPrivate;
class Q_QML_EXPORT QQmlIncubator
{
    Q_DISABLE_COPY(QQmlIncubator)
public:
    enum IncubationMode {
        Asynchronous,
        AsynchronousIfNested,
        Synchronous
    };
    enum Status {
        Null,
        Ready,
        Loading,
        Error
    };

    QQmlIncubator(IncubationMode = Asynchronous);
    virtual ~QQmlIncubator();

    void clear();
    void forceCompletion();

    bool isNull() const;
    bool isReady() const;
    bool isError() const;
    bool isLoading() const;

    QList<QQmlError> errors() const;

    IncubationMode incubationMode() const;

    Status status() const;

    QObject *object() const;

    void setInitialProperties(const QVariantMap &initialProperties);

protected:
    virtual void statusChanged(Status);
    virtual void setInitialState(QObject *);

private:
    friend class QQmlComponent;
    friend class QQmlEnginePrivate;
    friend class QQmlIncubatorPrivate;
    QQmlIncubatorPrivate *d;
};

class QQmlEnginePrivate;
class Q_QML_EXPORT QQmlIncubationController
{
    Q_DISABLE_COPY(QQmlIncubationController)
public:
    QQmlIncubationController();
    virtual ~QQmlIncubationController();

    QQmlEngine *engine() const;
    int incubatingObjectCount() const;

    void incubateFor(int msecs);
    void incubateWhile(std::atomic<bool> *flag, int msecs = 0);

protected:
    virtual void incubatingObjectCountChanged(int);

private:
    friend class QQmlEngine;
    friend class QQmlEnginePrivate;
    friend class QQmlIncubatorPrivate;
    QQmlEnginePrivate *d;
};

QT_END_NAMESPACE

#endif // QQMLINCUBATOR_H
