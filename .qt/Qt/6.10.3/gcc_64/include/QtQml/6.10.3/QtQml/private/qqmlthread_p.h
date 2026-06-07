// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLTHREAD_P_H
#define QQMLTHREAD_P_H

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


#include <QtCore/qglobal.h>

#include <private/qintrusivelist_p.h>

QT_BEGIN_NAMESPACE

class QThread;
class QMutex;

class QQmlThreadPrivate;
class QQmlThread
{
public:
    QQmlThread();
    virtual ~QQmlThread();

    void lock();
    void unlock();
    void wakeOne();
    void wait();

    bool isThisThread() const;

    // Synchronously invoke a method in the thread
    template<typename Method, typename ...Args>
    void callMethodInThread(Method &&method, Args &&...args);

    // Synchronously invoke a method in the main thread.  If the main thread is
    // blocked in a callMethodInThread() call, the call is made from within that
    // call.
    template<typename Method, typename ...Args>
    void callMethodInMain(Method &&method, Args &&...args);

    // Asynchronously invoke a method in the thread.
    template<typename Method, typename ...Args>
    void postMethodToThread(Method &&method, Args &&...args);

    // Asynchronously invoke a method in the main thread.
    template<typename Method, typename ...Args>
    void postMethodToMain(Method &&method, Args &&...args);

    void waitForNextMessage();
    void discardMessages();

    void startup();
    void shutdown();

protected:
    QThread *thread() const;
    QObject *threadObject() const;

private:
    friend class QQmlThreadPrivate;

    struct Message {
        Message() : next(nullptr) {}
        virtual ~Message() {}
        Message *next;
        virtual void call(QQmlThread *) = 0;
    };
    template<typename Method, typename ...Args>
    Message *createMessageFromMethod(Method &&method, Args &&...args);
    void internalCallMethodInThread(Message *);
    void internalCallMethodInMain(Message *);
    void internalPostMethodToThread(Message *);
    void internalPostMethodToMain(Message *);
    QQmlThreadPrivate *d;
};

namespace QtPrivate {
template <typename> struct member_function_traits;

template <typename Return, typename Object, typename... Args>
struct member_function_traits<Return (Object::*)(Args...)>
{
    using class_type = Object;
};
}

template<typename Method, typename ...Args>
QQmlThread::Message *QQmlThread::createMessageFromMethod(Method &&method, Args &&...args)
{
    struct I : public Message {
        Method m;
        std::tuple<std::decay_t<Args>...> arguments;
        I(Method &&method, Args&& ...args) : m(std::forward<Method>(method)), arguments(std::forward<Args>(args)...) {}
        void call(QQmlThread *thread) override {
            using class_type = typename QtPrivate::member_function_traits<Method>::class_type;
            class_type *me = static_cast<class_type *>(thread);
            std::apply(m, std::tuple_cat(std::make_tuple(me), arguments));
        }
    };
    return new I(std::forward<Method>(method), std::forward<Args>(args)...);
}

template<typename Method, typename ...Args>
void QQmlThread::callMethodInMain(Method &&method, Args&& ...args)
{
    Message *m = createMessageFromMethod(std::forward<Method>(method), std::forward<Args>(args)...);
    internalCallMethodInMain(m);
}

template<typename Method, typename ...Args>
void QQmlThread::callMethodInThread(Method &&method, Args&& ...args)
{
    Message *m = createMessageFromMethod(std::forward<Method>(method), std::forward<Args>(args)...);
    internalCallMethodInThread(m);
}

template<typename Method, typename ...Args>
void QQmlThread::postMethodToThread(Method &&method, Args&& ...args)
{
    Message *m = createMessageFromMethod(std::forward<Method>(method), std::forward<Args>(args)...);
    internalPostMethodToThread(m);
}

template<typename Method, typename ...Args>
void QQmlThread::postMethodToMain(Method &&method, Args&& ...args)
{
    Message *m = createMessageFromMethod(std::forward<Method>(method), std::forward<Args>(args)...);
    internalPostMethodToMain(m);
}

QT_END_NAMESPACE

#endif // QQMLTHREAD_P_H
