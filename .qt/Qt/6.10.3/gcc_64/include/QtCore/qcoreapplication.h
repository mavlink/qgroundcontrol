// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCOREAPPLICATION_H
#define QCOREAPPLICATION_H

#include <QtCore/qglobal.h>
#include <QtCore/qstring.h>
#ifndef QT_NO_QOBJECT
#include <QtCore/qcoreevent.h>
#include <QtCore/qdeadlinetimer.h>
#include <QtCore/qeventloop.h>
#include <QtCore/qobject.h>
#else
#include <QtCore/qscopedpointer.h>
#endif
#include <QtCore/qnativeinterface.h>

#ifndef QT_NO_QOBJECT
#if defined(Q_OS_WIN) && !defined(tagMSG)
typedef struct tagMSG MSG;
#endif
#endif

QT_BEGIN_NAMESPACE

class QAbstractEventDispatcher;
class QAbstractNativeEventFilter;
class QDebug;
class QEventLoopLocker;
class QPermission;
#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
class QPostEventList;
#endif
class QTranslator;

#define qApp QCoreApplication::instance()

class QCoreApplicationPrivate;
class Q_CORE_EXPORT QCoreApplication
#ifndef QT_NO_QOBJECT
    : public QObject
#endif
{
#ifndef QT_NO_QOBJECT
    Q_OBJECT
    Q_PROPERTY(QString applicationName READ applicationName WRITE setApplicationName
               NOTIFY applicationNameChanged)
    Q_PROPERTY(QString applicationVersion READ applicationVersion WRITE setApplicationVersion
               NOTIFY applicationVersionChanged)
    Q_PROPERTY(QString organizationName READ organizationName WRITE setOrganizationName
               NOTIFY organizationNameChanged)
    Q_PROPERTY(QString organizationDomain READ organizationDomain WRITE setOrganizationDomain
               NOTIFY organizationDomainChanged)
    Q_PROPERTY(bool quitLockEnabled READ isQuitLockEnabled WRITE setQuitLockEnabled)
#endif

    Q_DECLARE_PRIVATE(QCoreApplication)
    friend class QEventLoopLocker;
#if QT_CONFIG(permissions)
    using RequestPermissionPrototype = void(*)(QPermission);
#endif

public:
    enum { ApplicationFlags = QT_VERSION
    };

    QCoreApplication(int &argc, char **argv
#ifndef Q_QDOC
                     , int = ApplicationFlags
#endif
            );

    ~QCoreApplication();

    static QStringList arguments();

    static void setAttribute(Qt::ApplicationAttribute attribute, bool on = true);
    static bool testAttribute(Qt::ApplicationAttribute attribute);

    static void setOrganizationDomain(const QString &orgDomain);
    static QString organizationDomain();
    static void setOrganizationName(const QString &orgName);
    static QString organizationName();
    static void setApplicationName(const QString &application);
    static QString applicationName();
    static void setApplicationVersion(const QString &version);
    static QString applicationVersion();

    static void setSetuidAllowed(bool allow);
    static bool isSetuidAllowed();

#if QT_VERSION >= QT_VERSION_CHECK(7, 0, 0)
    static QCoreApplication *instance() noexcept { return self.loadRelaxed(); }
    static bool instanceExists() noexcept { return instance() != nullptr; }
#else
    static QCoreApplication *instance() noexcept { return self; }
    static bool instanceExists() noexcept;
#endif

#ifndef QT_NO_QOBJECT
    static int exec();
    static void processEvents(QEventLoop::ProcessEventsFlags flags = QEventLoop::AllEvents);
    static void processEvents(QEventLoop::ProcessEventsFlags flags, int maxtime);
    static void processEvents(QEventLoop::ProcessEventsFlags flags, QDeadlineTimer deadline);

    static bool sendEvent(QObject *receiver, QEvent *event);
    static void postEvent(QObject *receiver, QEvent *event, int priority = Qt::NormalEventPriority);
    static void sendPostedEvents(QObject *receiver = nullptr, int event_type = 0);
    static void removePostedEvents(QObject *receiver, int eventType = 0);
    static QAbstractEventDispatcher *eventDispatcher();
    static void setEventDispatcher(QAbstractEventDispatcher *eventDispatcher);

    virtual bool notify(QObject *, QEvent *);

    static bool startingUp();
    static bool closingDown();
#endif

    static QString applicationDirPath();
    static QString applicationFilePath();
    Q_DECL_CONST_FUNCTION static qint64 applicationPid() noexcept;

#if QT_CONFIG(permissions) || defined(Q_QDOC)
    Qt::PermissionStatus checkPermission(const QPermission &permission);

# ifdef Q_QDOC
    template <typename Functor>
    void requestPermission(const QPermission &permission, const QObject *context, Functor functor);
# else
    // requestPermission with context or receiver object; need to require here that receiver is the
    // right type to avoid ambiguity with the private implementation function.
    template <typename Functor,
              std::enable_if_t<
                    QtPrivate::AreFunctionsCompatible<RequestPermissionPrototype, Functor>::value,
                    bool> = true>
    void requestPermission(const QPermission &permission,
                           const typename QtPrivate::ContextTypeForFunctor<Functor>::ContextType *receiver,
                           Functor &&func)
    {
        requestPermissionImpl(permission,
                          QtPrivate::makeCallableObject<RequestPermissionPrototype>(std::forward<Functor>(func)),
                          receiver);
    }
# endif // Q_QDOC

#ifndef QT_NO_CONTEXTLESS_CONNECT
    #ifdef Q_QDOC
    template <typename Functor>
    #else
    // requestPermission to a functor or function pointer (without context)
    template <typename Functor,
              std::enable_if_t<
                    QtPrivate::AreFunctionsCompatible<RequestPermissionPrototype, Functor>::value,
                    bool> = true>
    #endif
    void requestPermission(const QPermission &permission, Functor &&func)
    {
        requestPermission(permission, nullptr, std::forward<Functor>(func));
    }
#endif // QT_NO_CONTEXTLESS_CONNECT

#if QT_CORE_REMOVED_SINCE(6, 10)
private:
    void requestPermission(const QPermission &permission,
        QtPrivate::QSlotObjectBase *slotObj, const QObject *context);
public:
#endif

#endif // QT_CONFIG(permission)

#if QT_CONFIG(library)
    static void setLibraryPaths(const QStringList &);
    static QStringList libraryPaths();
    static void addLibraryPath(const QString &);
    static void removeLibraryPath(const QString &);
#endif // QT_CONFIG(library)

#ifndef QT_NO_TRANSLATION
    static bool installTranslator(QTranslator * messageFile);
    static bool removeTranslator(QTranslator * messageFile);
#endif

    static QString translate(const char * context,
                             const char * key,
                             const char * disambiguation = nullptr,
                             int n = -1);

    QT_DECLARE_NATIVE_INTERFACE_ACCESSOR(QCoreApplication)

#ifndef QT_NO_QOBJECT
    void installNativeEventFilter(QAbstractNativeEventFilter *filterObj);
    void removeNativeEventFilter(QAbstractNativeEventFilter *filterObj);

    static bool isQuitLockEnabled();
    static void setQuitLockEnabled(bool enabled);

public Q_SLOTS:
    static void quit();
    static void exit(int retcode = 0);

Q_SIGNALS:
    void aboutToQuit(QPrivateSignal);

    void organizationNameChanged();
    void organizationDomainChanged();
    void applicationNameChanged();
    void applicationVersionChanged();

protected:
    bool event(QEvent *) override;

#  if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
    QT_DEPRECATED_VERSION_X_6_10("This feature will be removed in Qt 7")
    virtual bool compressEvent(QEvent *, QObject *receiver, QPostEventList *);
#  endif
#endif // QT_NO_QOBJECT

protected:
    QCoreApplication(QCoreApplicationPrivate &p);

#ifdef QT_NO_QOBJECT
    std::unique_ptr<QCoreApplicationPrivate> d_ptr;
#endif

private:
#ifndef QT_NO_QOBJECT
    static bool sendSpontaneousEvent(QObject *receiver, QEvent *event);
    static bool notifyInternal2(QObject *receiver, QEvent *);
    static bool forwardEvent(QObject *receiver, QEvent *event, QEvent *originatingEvent = nullptr);

    void requestPermissionImpl(const QPermission &permission, QtPrivate::QSlotObjectBase *slotObj,
                               const QObject *context);
#endif

#if QT_VERSION >= QT_VERSION_CHECK(7, 0, 0)
    static QBasicAtomicPointer<QCoreApplication> self;
#else
    static QCoreApplication *self;
#endif

    Q_DISABLE_COPY(QCoreApplication)

    friend class QApplication;
    friend class QApplicationPrivate;
    friend class QGuiApplication;
    friend class QGuiApplicationPrivate;
    friend class QWidget;
    friend class QWidgetWindow;
    friend class QWidgetPrivate;
    friend class QWindowPrivate;
#ifndef QT_NO_QOBJECT
    friend class QEventDispatcherUNIXPrivate;
    friend class QCocoaEventDispatcherPrivate;
    friend bool qt_sendSpontaneousEvent(QObject *, QEvent *);
#endif
    friend Q_CORE_EXPORT QString qAppName();
    friend class QCommandLineParserPrivate;
};

#define Q_DECLARE_TR_FUNCTIONS(context) \
public: \
    static inline QString tr(const char *sourceText, const char *disambiguation = nullptr, int n = -1) \
        { return QCoreApplication::translate(#context, sourceText, disambiguation, n); } \
private:

typedef void (*QtStartUpFunction)();
typedef void (*QtCleanUpFunction)();

Q_CORE_EXPORT void qAddPreRoutine(QtStartUpFunction);
Q_CORE_EXPORT void qAddPostRoutine(QtCleanUpFunction);
Q_CORE_EXPORT void qRemovePostRoutine(QtCleanUpFunction);
Q_CORE_EXPORT QString qAppName();                // get application name

#define Q_COREAPP_STARTUP_FUNCTION(AFUNC) \
    static void AFUNC ## _ctor_function() {  \
        qAddPreRoutine(AFUNC);        \
    }                                 \
    Q_CONSTRUCTOR_FUNCTION(AFUNC ## _ctor_function)

#ifndef QT_NO_QOBJECT
#if defined(Q_OS_WIN) && !defined(QT_NO_DEBUG_STREAM)
Q_CORE_EXPORT QString decodeMSG(const MSG &);
Q_CORE_EXPORT QDebug operator<<(QDebug, const MSG &);
#endif
#endif

QT_END_NAMESPACE

#include <QtCore/qcoreapplication_platform.h>

#endif // QCOREAPPLICATION_H
