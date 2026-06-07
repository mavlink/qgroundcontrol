// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLTYPELOADERNETWORKREPLYPROXY_P_H
#define QQMLTYPELOADERNETWORKREPLYPROXY_P_H

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

#include <QtQml/qtqmlglobal.h>
#include <QtCore/qobject.h>
#include <QtCore/private/qglobal_p.h>

QT_REQUIRE_CONFIG(qml_network);

QT_BEGIN_NAMESPACE

class QNetworkReply;
class QQmlTypeLoader;

// This is a lame object that we need to ensure that slots connected to
// QNetworkReply get called in the correct thread (the loader thread).
// As QQmlTypeLoader lives in the main thread, and we can't use
// Qt::DirectConnection connections from a QNetworkReply (because then
// sender() wont work), we need to insert this object in the middle.
class QQmlTypeLoaderNetworkReplyProxy : public QObject
{
    Q_OBJECT
public:
    QQmlTypeLoaderNetworkReplyProxy(QQmlTypeLoader *l, QObject *parent);

public Q_SLOTS:
    void finished();
    void downloadProgress(qint64, qint64);
    void manualFinished(QNetworkReply*);

private:
    QQmlTypeLoader *l;
};

QT_END_NAMESPACE

#endif // QQMLTYPELOADERNETWORKREPLYPROXY_P_H
