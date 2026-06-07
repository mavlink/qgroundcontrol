// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCOREAPPLICATION_P_H
#define QCOREAPPLICATION_P_H

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

#include "QtCore/qcoreapplication.h"
#if QT_CONFIG(commandlineparser)
#include "QtCore/qcommandlineoption.h"
#endif
#include "QtCore/qreadwritelock.h"
#include "QtCore/qtranslator.h"
#ifndef QT_NO_QOBJECT
#include "private/qobject_p.h"
#include "private/qlocking_p.h"
#endif

#ifdef Q_OS_MACOS
#include "private/qcore_mac_p.h"
#endif

QT_BEGIN_NAMESPACE

typedef QList<QTranslator*> QTranslatorList;

class QAbstractEventDispatcher;

#ifndef QT_NO_QOBJECT
class QEvent;
#endif

class Q_CORE_EXPORT QCoreApplicationPrivate
#ifndef QT_NO_QOBJECT
    : public QObjectPrivate
#endif
{
    Q_DECLARE_PUBLIC(QCoreApplication)

public:
    enum Type : quint8 {
        Tty,
        Gui
    };

    QCoreApplicationPrivate(int &aargc,  char **aargv);

    // If not inheriting from QObjectPrivate: force this class to be polymorphic
#ifdef QT_NO_QOBJECT
    virtual
#endif
    ~QCoreApplicationPrivate();

    void init();

    QString appName() const;
    QString appVersion() const;

#ifdef Q_OS_DARWIN
    static QString infoDictionaryStringProperty(const QString &propertyName);
#endif

#ifdef Q_OS_WINDOWS
    void initDebuggingConsole();
    void cleanupDebuggingConsole();
#endif
    static void initLocale();

    static bool checkInstance(const char *method);

#if QT_CONFIG(commandlineparser)
    virtual void addQtOptions(QList<QCommandLineOption> *options);
#endif

#ifndef QT_NO_QOBJECT
    bool sendThroughApplicationEventFilters(QObject *, QEvent *);
    static bool sendThroughObjectEventFilters(QObject *, QEvent *);
    static bool notify_helper(QObject *, QEvent *);
    static inline void setEventSpontaneous(QEvent *e, bool spontaneous) { e->m_spont = spontaneous; }

    virtual void createEventDispatcher();
    virtual void eventDispatcherReady();
    virtual bool compressEvent(QEvent *event, QObject *receiver, QPostEventList *postedEvents);
    static void removePostedEvent(QEvent *);
#ifdef Q_OS_WIN
    static void removePostedTimerEvent(QObject *object, int timerId);
#endif

    QAtomicInt quitLockRef;
    void ref();
    void deref();
    virtual bool canQuitAutomatically();
    void quitAutomatically();
    virtual void quit();

    static QBasicAtomicPointer<QThread> theMainThread;
    static QBasicAtomicPointer<void> theMainThreadId;
    static QThread *mainThread();

    static void sendPostedEvents(QObject *receiver, int event_type, QThreadData *data);

    static void checkReceiverThread(QObject *receiver);
    void cleanupThreadData();

    struct QPostEventListLocker
    {
        QThreadData *threadData;
        std::unique_lock<QMutex> locker;

        void unlock() { locker.unlock(); }
    };
    static QPostEventListLocker lockThreadPostEventList(QObject *object);
#endif // QT_NO_QOBJECT

    int &argc;
    char **argv;
#if defined(Q_OS_WIN)
    // store unmodified arguments for QCoreApplication::arguments()
    int origArgc = 0;
    std::unique_ptr<char *[]> origArgv;

    bool consoleAllocated = false;
    static void *mainInstanceHandle;    // HINSTANCE without <windows.h>
#endif

    Type application_type = Tty;

#ifndef QT_NO_QOBJECT
    void execCleanup();

    bool in_exec = false;
    bool aboutToQuitEmitted = false;
    bool threadData_clean = false;

    static QAbstractEventDispatcher *eventDispatcher;
    static bool is_app_running;
    static bool is_app_closing;
#endif
#ifndef QT_NO_TRANSLATION
    QTranslatorList translators;
    QReadWriteLock translateMutex;
    static bool isTranslatorInstalled(QTranslator *translator);
#endif

    static bool setuidAllowed;
    static uint attribs;
    static inline bool testAttribute(uint flag) { return attribs & (1 << flag); }

    void processCommandLineArguments();
    QString cachedApplicationFilePath;
    QString qmljs_debug_arguments; // a string containing arguments for js/qml debugging.
    inline QString qmljsDebugArgumentsString() const { return qmljs_debug_arguments; }

#ifdef QT_NO_QOBJECT
    QCoreApplication *q_ptr = nullptr;
#endif
};

QT_END_NAMESPACE

#endif // QCOREAPPLICATION_P_H
