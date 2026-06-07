// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:execute-external-code

#ifndef QPROCESS_P_H
#define QPROCESS_P_H

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

#include "QtCore/qprocess.h"
#include "QtCore/qstringlist.h"
#include "QtCore/qhash.h"
#include "QtCore/qmap.h"
#include "QtCore/qshareddata.h"
#include "QtCore/qdeadlinetimer.h"
#include "private/qiodevice_p.h"

QT_REQUIRE_CONFIG(processenvironment);

#ifdef Q_OS_UNIX
#include <QtCore/private/qorderedmutexlocker_p.h>
#endif

#ifdef Q_OS_WIN
#include "QtCore/qt_windows.h"
typedef HANDLE Q_PIPE;
#define INVALID_Q_PIPE INVALID_HANDLE_VALUE
#else
typedef int Q_PIPE;
#define INVALID_Q_PIPE -1
#endif

QT_BEGIN_NAMESPACE

class QSocketNotifier;
class QWindowsPipeReader;
class QWindowsPipeWriter;
class QWinEventNotifier;

#ifdef Q_OS_WIN
class QProcEnvKey : public QString
{
public:
    QProcEnvKey() {}
    explicit QProcEnvKey(const QString &other) : QString(other) {}
    QProcEnvKey(const QProcEnvKey &other) : QString(other) {}
    bool operator==(const QProcEnvKey &other) const { return !compare(other, Qt::CaseInsensitive); }
};

inline bool operator<(const QProcEnvKey &a, const QProcEnvKey &b)
{
    // On windows use case-insensitive ordering because that is how Windows needs the environment
    // block sorted (https://msdn.microsoft.com/en-us/library/windows/desktop/ms682009(v=vs.85).aspx)
    return a.compare(b, Qt::CaseInsensitive) < 0;
}

Q_DECLARE_TYPEINFO(QProcEnvKey, Q_RELOCATABLE_TYPE);

typedef QString QProcEnvValue;
#else
using QProcEnvKey = QByteArray;

class QProcEnvValue
{
public:
    QProcEnvValue() = default;
    explicit QProcEnvValue(const QString &value) : stringValue(value) {}
    explicit QProcEnvValue(const QByteArray &value) : byteValue(value) {}
    bool operator==(const QProcEnvValue &other) const
    {
        return byteValue.isEmpty() && other.byteValue.isEmpty()
                ? stringValue == other.stringValue
                : bytes() == other.bytes();
    }
    QByteArray bytes() const
    {
        if (byteValue.isEmpty() && !stringValue.isEmpty())
            byteValue = stringValue.toLocal8Bit();
        return byteValue;
    }
    QString string() const
    {
        if (stringValue.isEmpty() && !byteValue.isEmpty())
            stringValue = QString::fromLocal8Bit(byteValue);
        return stringValue;
    }

    mutable QByteArray byteValue;
    mutable QString stringValue;
};
Q_DECLARE_TYPEINFO(QProcEnvValue, Q_RELOCATABLE_TYPE);
#endif

class QProcessEnvironmentPrivate: public QSharedData
{
public:
    typedef QProcEnvKey Key;
    typedef QProcEnvValue Value;
#ifdef Q_OS_WIN
    inline Key prepareName(const QString &name) const { return Key(name); }
    inline QString nameToString(const Key &name) const { return name; }
    inline Value prepareValue(const QString &value) const { return value; }
    inline QString valueToString(const Value &value) const { return value; }
    struct MutexLocker {
        MutexLocker(const QProcessEnvironmentPrivate *) {}
    };
    struct OrderedMutexLocker {
        OrderedMutexLocker(const QProcessEnvironmentPrivate *,
                           const QProcessEnvironmentPrivate *) {}
    };
#else
    inline Key prepareName(const QString &name) const
    {
        Key &ent = nameMap[name];
        if (ent.isEmpty())
            ent = name.toLocal8Bit();
        return ent;
    }
    inline QString nameToString(const Key &name) const
    {
        const QString sname = QString::fromLocal8Bit(name);
        nameMap[sname] = name;
        return sname;
    }
    inline Value prepareValue(const QString &value) const { return Value(value); }
    inline QString valueToString(const Value &value) const { return value.string(); }

    struct MutexLocker : public QMutexLocker<QMutex>
    {
        MutexLocker(const QProcessEnvironmentPrivate *d) : QMutexLocker(&d->mutex) {}
    };
    struct OrderedMutexLocker : public QOrderedMutexLocker
    {
        OrderedMutexLocker(const QProcessEnvironmentPrivate *d1,
                           const QProcessEnvironmentPrivate *d2) :
            QOrderedMutexLocker(&d1->mutex, &d2->mutex)
        {}
    };

    QProcessEnvironmentPrivate() : QSharedData() {}
    QProcessEnvironmentPrivate(const QProcessEnvironmentPrivate &other) :
        QSharedData()
    {
        // This being locked ensures that the functions that only assign
        // d pointers don't need explicit locking.
        // We don't need to lock our own mutex, as this object is new and
        // consequently not shared. For the same reason, non-const methods
        // do not need a lock, as they detach objects (however, we need to
        // ensure that they really detach before using prepareName()).
        MutexLocker locker(&other);
        vars = other.vars;
        nameMap = other.nameMap;
        // We need to detach our members, so that our mutex can protect them.
        // As we are being detached, they likely would be detached a moment later anyway.
        vars.detach();
        nameMap.detach();
    }
#endif

    using Map = QMap<Key, Value>;
    Map vars;

#ifdef Q_OS_UNIX
    typedef QHash<QString, Key> NameHash;
    mutable NameHash nameMap;

    mutable QMutex mutex;
#endif

    static QProcessEnvironment fromList(const QStringList &list);
    QStringList toList() const;
    QStringList keys() const;
    void insert(const QProcessEnvironmentPrivate &other);
};

template<> Q_INLINE_TEMPLATE void QSharedDataPointer<QProcessEnvironmentPrivate>::detach()
{
    if (d && d->ref.loadRelaxed() == 1)
        return;
    QProcessEnvironmentPrivate *x = (d ? new QProcessEnvironmentPrivate(*d)
                                     : new QProcessEnvironmentPrivate);
    x->ref.ref();
    if (d && !d->ref.deref())
        delete d.get();
    d.reset(x);
}

#if QT_CONFIG(process)

class QProcessPrivate : public QIODevicePrivate
{
public:
    Q_DECLARE_PUBLIC(QProcess)

    struct Channel {
        enum ProcessChannelType : char {
            Normal = 0,
            PipeSource = 1,
            PipeSink = 2,
            Redirect = 3
        };

        void clear();

        Channel &operator=(const QString &fileName)
        {
            clear();
            file = fileName;
            type = fileName.isEmpty() ? Normal : Redirect;
            return *this;
        }

        void pipeTo(QProcessPrivate *other)
        {
            clear();
            process = other;
            type = PipeSource;
        }

        void pipeFrom(QProcessPrivate *other)
        {
            clear();
            process = other;
            type = PipeSink;
        }

        QString file;
        QProcessPrivate *process = nullptr;
#ifdef Q_OS_UNIX
        QSocketNotifier *notifier = nullptr;
#else
        union {
            QWindowsPipeReader *reader = nullptr;
            QWindowsPipeWriter *writer;
        };
#endif
        Q_PIPE pipe[2] = {INVALID_Q_PIPE, INVALID_Q_PIPE};

        ProcessChannelType type = Normal;
        bool closed = false;
        bool append = false;
    };

    QProcessPrivate();
    virtual ~QProcessPrivate();

    // private slots
    bool _q_canReadStandardOutput();
    bool _q_canReadStandardError();
#ifdef Q_OS_WIN
    qint64 pipeWriterBytesToWrite() const;
    void _q_bytesWritten(qint64 bytes);
    void _q_writeFailed();
#else
    bool _q_canWrite();
    bool writeToStdin();
#endif
    bool _q_startupNotification();
    void _q_processDied();

    Channel stdinChannel;
    Channel stdoutChannel;
    Channel stderrChannel;
    bool openChannels();
    bool openChannelsForDetached();
    bool openChannel(Channel &channel);
    void closeChannel(Channel *channel);
    void closeWriteChannel();
    void closeChannels();
    bool tryReadFromChannel(Channel *channel); // obviously, only stdout and stderr

    QString program;
    QStringList arguments;
    QString workingDirectory;
    QProcessEnvironment environment = QProcessEnvironment::InheritFromParent;
#if defined(Q_OS_WIN)
    QString nativeArguments;
    QProcess::CreateProcessArgumentModifier modifyCreateProcessArgs;
    QWinEventNotifier *processFinishedNotifier = nullptr;
    Q_PROCESS_INFORMATION *pid = nullptr;
#else
    struct UnixExtras {
        std::function<void(void)> childProcessModifier;
        QProcess::UnixProcessParameters processParameters;
    };
    std::unique_ptr<UnixExtras> unixExtras;
    QSocketNotifier *stateNotifier = nullptr;
    Q_PIPE childStartedPipe[2] = {INVALID_Q_PIPE, INVALID_Q_PIPE};
    pid_t pid = 0;
    int forkfd = -1;
#endif

    int exitCode = 0;
    quint8 processState = QProcess::NotRunning;
    quint8 exitStatus = QProcess::NormalExit;
    quint8 processError = QProcess::UnknownError;
    quint8 processChannelMode = QProcess::SeparateChannels;
    quint8 inputChannelMode = QProcess::ManagedInputChannel;
    bool emittedReadyRead = false;
    bool emittedBytesWritten = false;

    void start(QIODevice::OpenMode mode);
    void startProcess();
#if defined(Q_OS_UNIX)
    void commitChannels() const;
#endif
    bool processStarted(QString *errorMessage = nullptr);
    void processFinished();
    void terminateProcess();
    void killProcess();
#ifdef Q_OS_UNIX
    void waitForDeadChild();
#else
    void findExitCode();
#endif
#ifdef Q_OS_WIN
    STARTUPINFOW createStartupInfo();
    bool callCreateProcess(QProcess::CreateProcessArguments *cpargs);
    bool drainOutputPipes();
#endif

    bool startDetached(qint64 *pPid);

    bool waitForStarted(const QDeadlineTimer &deadline);
    bool waitForReadyRead(const QDeadlineTimer &deadline);
    bool waitForBytesWritten(const QDeadlineTimer &deadline);
    bool waitForFinished(const QDeadlineTimer &deadline);

    qint64 bytesAvailableInChannel(const Channel *channel) const;
    qint64 readFromChannel(const Channel *channel, char *data, qint64 maxlen);

    void destroyPipe(Q_PIPE pipe[2]);
    void cleanup();
    void setError(QProcess::ProcessError error, const QString &description = QString());
    void setErrorAndEmit(QProcess::ProcessError error, const QString &description = QString());

    const QProcessEnvironmentPrivate *environmentPrivate() const
    { return environment.d.constData(); }
};

#endif // QT_CONFIG(process)

QT_END_NAMESPACE

#endif // QPROCESS_P_H
