// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QV4INCLUDE_P_H
#define QV4INCLUDE_P_H

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

#include <QtCore/qobject.h>
#include <QtCore/qurl.h>
#include <QtCore/qpointer.h>

#include <private/qv4value_p.h>
#include <private/qv4context_p.h>
#include <private/qv4persistent_p.h>

QT_BEGIN_NAMESPACE

class QJSEngine;
class QJSValue;
#if QT_CONFIG(qml_network)
class QNetworkAccessManager;
#endif
class QNetworkReply;
class QV4Include : public QObject
{
    Q_OBJECT
public:
    enum Status {
        Ok = 0,
        Loading = 1,
        NetworkError = 2,
        Exception = 3
    };

    static QJSValue method_include(QV4::ExecutionEngine *engine, const QUrl &url,
                                   const QJSValue &callbackFunction);

private Q_SLOTS:
    void finished();

private:
    QV4Include(const QUrl &url, QV4::ExecutionEngine *engine, QV4::QmlContext *qmlContext,
               const QV4::Value &callback);
    ~QV4Include();

    QV4::ReturnedValue result();

    static QV4::ReturnedValue resultValue(QV4::ExecutionEngine *v4, Status status = Loading,
                                          const QString &statusText = QString());
    static void callback(const QV4::Value &callback, const QV4::Value &status);

    QV4::ExecutionEngine *v4;
    QUrl m_url;

#if QT_CONFIG(qml_network)
    QNetworkAccessManager *m_network;
    QPointer<QNetworkReply> m_reply;
#endif

    QV4::PersistentValue m_callbackFunction;
    QV4::PersistentValue m_resultObject;
    QV4::PersistentValue m_qmlContext;
};

QT_END_NAMESPACE

#endif

