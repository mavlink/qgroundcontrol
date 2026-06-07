// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QNETWORKREPLY_P_H
#define QNETWORKREPLY_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the Network Access API.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtNetwork/private/qtnetworkglobal_p.h>
#include "qnetworkrequest.h"
#include "qnetworkrequest_p.h"
#include "qnetworkreply.h"
#include "QtCore/qpointer.h"
#include <QtCore/QElapsedTimer>
#include "private/qiodevice_p.h"

QT_BEGIN_NAMESPACE

class QNetworkReplyPrivate: public QIODevicePrivate, public QNetworkHeadersPrivate
{
public:
    enum State {
        Idle,               // The reply is idle.
        Buffering,          // The reply is buffering outgoing data.
        Working,            // The reply is uploading/downloading data.
        Finished,           // The reply has finished.
        Aborted,            // The reply has been aborted.
    };

    QNetworkReplyPrivate();
    QNetworkRequest request;
    QNetworkRequest originalRequest;
    QUrl url;
    QPointer<QNetworkAccessManager> manager;
    qint64 readBufferMaxSize;
    QElapsedTimer downloadProgressSignalChoke;
    QElapsedTimer uploadProgressSignalChoke;
    bool emitAllUploadProgressSignals;
    const static int progressSignalInterval;
    QNetworkAccessManager::Operation operation;
    QNetworkReply::NetworkError errorCode;
    bool isFinished;

    static inline void setManager(QNetworkReply *reply, QNetworkAccessManager *manager)
    { reply->d_func()->manager = manager; }

    Q_DECLARE_PUBLIC(QNetworkReply)
};

QT_END_NAMESPACE

#endif
