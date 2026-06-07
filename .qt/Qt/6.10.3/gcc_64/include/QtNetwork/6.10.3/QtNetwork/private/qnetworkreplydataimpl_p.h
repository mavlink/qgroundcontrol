// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QNETWORKREPLYDATAIMPL_H
#define QNETWORKREPLYDATAIMPL_H

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
#include "qnetworkreply.h"
#include "qnetworkreply_p.h"
#include "qnetworkaccessmanager.h"
#include <QBuffer>

QT_BEGIN_NAMESPACE


class QNetworkReplyDataImplPrivate;
class QNetworkReplyDataImpl: public QNetworkReply
{
    Q_OBJECT
public:
    QNetworkReplyDataImpl(QObject *parent, const QNetworkRequest &req, const QNetworkAccessManager::Operation op);
    ~QNetworkReplyDataImpl();
    virtual void abort() override;

    // reimplemented from QNetworkReply
    virtual void close() override;
    virtual qint64 bytesAvailable() const override;
    virtual bool isSequential () const override;
    qint64 size() const override;

    virtual qint64 readData(char *data, qint64 maxlen) override;

    Q_DECLARE_PRIVATE(QNetworkReplyDataImpl)
};

class QNetworkReplyDataImplPrivate: public QNetworkReplyPrivate
{
public:
    QNetworkReplyDataImplPrivate();
    ~QNetworkReplyDataImplPrivate();

    QBuffer decodedData;

    Q_DECLARE_PUBLIC(QNetworkReplyDataImpl)
};

QT_END_NAMESPACE

#endif // QNETWORKREPLYDATAIMPL_H
