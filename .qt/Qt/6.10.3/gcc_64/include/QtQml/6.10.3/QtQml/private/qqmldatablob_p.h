// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLDATABLOB_P_H
#define QQMLDATABLOB_P_H

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

#include <private/qqmlrefcount_p.h>
#include <private/qqmljsdiagnosticmessage_p.h>

#if QT_CONFIG(qml_network)
#include <QtNetwork/qnetworkreply.h>
#endif

#include <QtQml/qqmlprivate.h>
#include <QtQml/qqmlerror.h>
#include <QtQml/qqmlabstracturlinterceptor.h>
#include <QtQml/qqmlprivate.h>

#include <QtCore/qdatetime.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qurl.h>

QT_BEGIN_NAMESPACE

class QQmlTypeLoader;
class Q_QML_EXPORT QQmlDataBlob : public QQmlRefCounted<QQmlDataBlob>
{
public:
    using Ptr = QQmlRefPointer<QQmlDataBlob>;

    enum Status {
        Null,                    // Prior to QQmlTypeLoader::load()
        Loading,                 // Prior to data being received and dataReceived() being called
        WaitingForDependencies,  // While there are outstanding addDependency()s
        ResolvingDependencies,   // While resolving outstanding dependencies, to detect cycles
        Complete,                // Finished
        Error                    // Error
    };

    enum Type { //Matched in QQmlAbstractUrlInterceptor
        QmlFile = QQmlAbstractUrlInterceptor::QmlFile,
        JavaScriptFile = QQmlAbstractUrlInterceptor::JavaScriptFile,
        QmldirFile = QQmlAbstractUrlInterceptor::QmldirFile
    };

    QQmlDataBlob(const QUrl &, Type, QQmlTypeLoader* manager);
    virtual ~QQmlDataBlob();

    void startLoading();

    QQmlTypeLoader *typeLoader() const { return m_typeLoader; }

    Type type() const;

    Status status() const;
    bool isNull() const;
    bool isLoading() const;
    bool isWaiting() const;
    bool isComplete() const;
    bool isError() const;
    bool isCompleteOrError() const;

    bool isAsync() const;
    qreal progress() const;

    QUrl url() const;
    QString urlString() const;
    QUrl finalUrl() const;
    QString finalUrlString() const;

    QList<QQmlError> errors() const;

    class SourceCodeData {
    public:
        QString readAll(QString *error) const;
        QDateTime sourceTimeStamp() const;
        bool exists() const;
        bool isEmpty() const;
        bool isCacheable() const { return !hasInlineSourceCode; }
        bool isValid() const
        {
            return hasInlineSourceCode || !fileInfo.filePath().isEmpty();
        }

    private:
        friend class QQmlDataBlob;
        friend class QQmlTypeLoader;
        QString inlineSourceCode;
        QFileInfo fileInfo;
        bool hasInlineSourceCode = false;
    };

    template<typename Loader = QQmlTypeLoader>
    void assertTypeLoaderThreadIfRunning() const
    {
        const Loader *loader = m_typeLoader;
        Q_ASSERT(!loader || !loader->thread() || loader->thread()->isThisThread());
    }

    template<typename Loader = QQmlTypeLoader>
    void assertTypeLoaderThread() const
    {
        const Loader *loader = m_typeLoader;
        Q_ASSERT(loader && loader->thread() && loader->thread()->isThisThread());
    }

    template<typename Loader = QQmlTypeLoader>
    void assertEngineThread() const
    {
        const Loader *loader = m_typeLoader;
        Q_ASSERT(loader && loader->engine() && loader->engine()->thread()->isCurrentThread());
    }

protected:
    // Can be called from within callbacks
    void setError(const QQmlError &);
    void setError(const QList<QQmlError> &errors);
    void setError(const QQmlJS::DiagnosticMessage &error);
    void setError(const QString &description);
    void addDependency(const QQmlDataBlob::Ptr &);

    // Callbacks made in load thread
    virtual void dataReceived(const SourceCodeData &) = 0;
    virtual void initializeFromCachedUnit(const QQmlPrivate::CachedQmlUnit *) = 0;
    virtual void done();
#if QT_CONFIG(qml_network)
    virtual void networkError(QNetworkReply::NetworkError);
#endif
    virtual void dependencyError(const QQmlDataBlob::Ptr &);
    virtual void dependencyComplete(const QQmlDataBlob::Ptr &);
    virtual void allDependenciesDone();

    // Callbacks made in main thread
    virtual void downloadProgressChanged(qreal);
    virtual void completed();

protected:
    // Manager that is currently fetching data for me
    QQmlTypeLoader *m_typeLoader;

private:
    friend class QQmlTypeLoader;
    friend class QQmlTypeLoaderThread;

    void tryDone();
    void cancelAllWaitingFor();
    void notifyAllWaitingOnMe();
    void notifyComplete(const QQmlDataBlob::Ptr &);

    bool setStatus(Status status);
    bool setIsAsync(bool async) { return m_data.setIsAsync(async); }
    bool setProgress(qreal progress) { return m_data.setProgress(progress); }

    struct ThreadData {
    public:
        friend bool QQmlDataBlob::setStatus(Status);
        friend bool QQmlDataBlob::setIsAsync(bool);
        friend bool QQmlDataBlob::setProgress(qreal);

        inline ThreadData()
            : _p(0)
        {
        }

        inline QQmlDataBlob::Status status() const
        {
            return QQmlDataBlob::Status((_p.loadAcquire() & StatusMask) >> StatusShift);
        }

        inline bool isAsync() const
        {
            return _p.loadAcquire() & AsyncMask;
        }

        inline qreal progress() const
        {
            return quint8((_p.loadAcquire() & ProgressMask) >> ProgressShift) / float(0xFF);
        }

    private:
        enum {
            StatusMask = 0x0000FFFF,
            StatusShift = 0,
            ProgressMask = 0x00FF0000,
            ProgressShift = 16,
            AsyncMask = 0x80000000,
            NoMask = 0
        };


        inline bool setStatus(QQmlDataBlob::Status status)
        {
            while (true) {
                int d = _p.loadAcquire();
                int nd = (d & ~StatusMask) | ((status << StatusShift) & StatusMask);
                if (d == nd)
                    return false;
                if (_p.testAndSetRelease(d, nd))
                    return true;
            }

            Q_UNREACHABLE_RETURN(false);
        }

        inline bool setIsAsync(bool v)
        {
            while (true) {
                int d = _p.loadAcquire();
                int nd = (d & ~AsyncMask) | (v ? AsyncMask : NoMask);
                if (d == nd)
                    return false;
                if (_p.testAndSetRelease(d, nd))
                    return true;
            }

            Q_UNREACHABLE_RETURN(false);
        }

        inline bool setProgress(qreal progress)
        {
            quint8 v = 0xFF * progress;
            while (true) {
                int d = _p.loadAcquire();
                int nd = (d & ~ProgressMask) | ((v << ProgressShift) & ProgressMask);
                if (d == nd)
                    return false;
                if (_p.testAndSetRelease(d, nd))
                    return true;
            }
            Q_UNREACHABLE_RETURN(false);
        }

    private:
        QAtomicInt _p;
    };
    ThreadData m_data;

    // m_errors should *always* be written before the status is set to Error.
    // We use the status change as a memory fence around m_errors so that locking
    // isn't required.  Once the status is set to Error (or Complete), m_errors
    // cannot be changed.
    QList<QQmlError> m_errors;

    Type m_type;

    QUrl m_url;
    QUrl m_finalUrl;
    mutable QString m_urlString;
    mutable QString m_finalUrlString;

    // List of QQmlDataBlob's that are waiting for me to complete.
protected:
    QList<QQmlDataBlob *> m_waitingOnMe;
private:

    // List of QQmlDataBlob's that I am waiting for to complete.
    QVector<QQmlRefPointer<QQmlDataBlob>> m_waitingFor;

    int m_redirectCount:30;
    bool m_inCallback:1;
    bool m_isDone:1;
};

QT_END_NAMESPACE

#endif // QQMLDATABLOB_P_H
