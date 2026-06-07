// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLVME_P_H
#define QQMLVME_P_H

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

#include <private/qrecursionwatcher_p.h>

#include <QtCore/QStack>
#include <QtCore/QString>
#include <QtCore/qelapsedtimer.h>
#include <QtCore/qdeadlinetimer.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qtypeinfo.h>

#include <private/qqmlengine_p.h>

#include <atomic>

QT_BEGIN_NAMESPACE

class QObject;

class QQmlInstantiationInterrupt {
public:
    inline QQmlInstantiationInterrupt();
    inline QQmlInstantiationInterrupt(std::atomic<bool> *runWhile,
                                      QDeadlineTimer deadline = QDeadlineTimer::Forever);
    inline QQmlInstantiationInterrupt(QDeadlineTimer deadline);

    inline bool shouldInterrupt() const;
private:
    enum Mode { None, Time, Flag };
    Mode mode;
    QDeadlineTimer deadline;
    std::atomic<bool> *runWhile = nullptr;
};

class Q_QML_EXPORT QQmlVME
{
public:
    static void enableComponentComplete();
    static void disableComponentComplete();
    static bool componentCompleteEnabled();

private:
    static bool s_enableComponentComplete;
};

// Used to check that a QQmlVME that is interrupted mid-execution
// is still valid.  Checks all the objects and contexts have not been
// deleted.
//
// VME stands for Virtual Machine Execution. QML files used to
// be compiled to a byte code data structure that a virtual machine executed
// (for constructing the tree of QObjects and setting properties).
class QQmlVMEGuard
{
public:
    QQmlVMEGuard();
    ~QQmlVMEGuard();

    void guard(QQmlObjectCreator *);
    void clear();

    bool isOK() const;

private:
    size_t m_objectCount;
    QQmlGuard<QObject> *m_objects;
    int m_contextCount;
    QQmlGuardedContextData *m_contexts;
};

QQmlInstantiationInterrupt::QQmlInstantiationInterrupt()
    : mode(None)
{
}

QQmlInstantiationInterrupt::QQmlInstantiationInterrupt(std::atomic<bool> *runWhile, QDeadlineTimer deadline)
    : mode(Flag), deadline(deadline), runWhile(runWhile)
{
}

QQmlInstantiationInterrupt::QQmlInstantiationInterrupt(QDeadlineTimer deadline)
    : mode(Time), deadline(deadline)
{
}

bool QQmlInstantiationInterrupt::shouldInterrupt() const
{
    switch (mode) {
    case None:
        return false;
    case Time:
        return deadline.hasExpired();
    case Flag:
        return !runWhile->load(std::memory_order_acquire) || deadline.hasExpired();
    }
    Q_UNREACHABLE_RETURN(false);
}

QT_END_NAMESPACE

#endif // QQMLVME_P_H
