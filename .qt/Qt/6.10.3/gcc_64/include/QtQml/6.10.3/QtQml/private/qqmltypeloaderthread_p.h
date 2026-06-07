// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLTYPELOADERTHREAD_P_H
#define QQMLTYPELOADERTHREAD_P_H

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

#include <private/qqmlthread_p.h>
#include <private/qv4compileddata_p.h>
#include <private/qqmldatablob_p.h>

#include <QtQml/qtqmlglobal.h>

#if QT_CONFIG(qml_network)
#include <private/qqmltypeloadernetworkreplyproxy_p.h>
#include <QtNetwork/qnetworkaccessmanager.h>
#endif

QT_BEGIN_NAMESPACE

class QQmlTypeLoader;
class QQmlEngineExtensionInterface;
class QQmlExtensionInterface;

namespace QQmlPrivate {
struct CachedQmlUnit;
}

class QQmlTypeLoaderThread : public QQmlThread
{
    typedef QQmlTypeLoaderThread This;

public:
    QQmlTypeLoaderThread(QQmlTypeLoader *loader);
    ~QQmlTypeLoaderThread();
#if QT_CONFIG(qml_network)
    QNetworkAccessManager *networkAccessManager() const;
    QQmlTypeLoaderNetworkReplyProxy *networkReplyProxy() const;
#endif // qml_network
    void load(const QQmlDataBlob::Ptr &b);
    void loadAsync(const QQmlDataBlob::Ptr &b);
    void loadWithStaticData(const QQmlDataBlob::Ptr &b, const QByteArray &);
    void loadWithStaticDataAsync(const QQmlDataBlob::Ptr &b, const QByteArray &);
    void loadWithCachedUnit(const QQmlDataBlob::Ptr &b, const QQmlPrivate::CachedQmlUnit *unit);
    void loadWithCachedUnitAsync(const QQmlDataBlob::Ptr &b, const QQmlPrivate::CachedQmlUnit *unit);
    void callCompleted(const QQmlDataBlob::Ptr &b);
    void callDownloadProgressChanged(const QQmlDataBlob::Ptr &b, qreal p);
    void initializeEngine(QQmlExtensionInterface *, const char *);
    void initializeEngine(QQmlEngineExtensionInterface *, const char *);
    void drop(const QQmlDataBlob::Ptr &b);

private:
    void loadThread(const QQmlDataBlob::Ptr &b);
    void loadWithStaticDataThread(const QQmlDataBlob::Ptr &b, const QByteArray &);
    void loadWithCachedUnitThread(const QQmlDataBlob::Ptr &b, const QQmlPrivate::CachedQmlUnit *unit);
    void callCompletedMain(const QQmlDataBlob::Ptr &b);
    void callDownloadProgressChangedMain(const QQmlDataBlob::Ptr &b, qreal p);
    void initializeExtensionMain(QQmlExtensionInterface *iface, const char *uri);
    void initializeEngineExtensionMain(QQmlEngineExtensionInterface *iface, const char *uri);
    void dropThread(const QQmlDataBlob::Ptr &b);

    QQmlTypeLoader *m_loader;
#if QT_CONFIG(qml_network)
    mutable QNetworkAccessManager *m_networkAccessManager = nullptr;
    mutable QQmlTypeLoaderNetworkReplyProxy *m_networkReplyProxy = nullptr;
#endif // qml_network
};

QT_END_NAMESPACE

#endif // QQMLTYPELOADERTHREAD_P_H
