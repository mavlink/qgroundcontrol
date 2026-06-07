// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QNETWORKACCESSFILEBACKEND_P_H
#define QNETWORKACCESSFILEBACKEND_P_H

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
#include "qnetworkaccessbackend_p.h"
#include "qnetworkrequest.h"
#include "qnetworkreply.h"
#include "QtCore/qfile.h"

QT_BEGIN_NAMESPACE

class QNetworkAccessFileBackend: public QNetworkAccessBackend
{
    Q_OBJECT
public:
    QNetworkAccessFileBackend();
    virtual ~QNetworkAccessFileBackend();

    void open() override;
    void close() override;

    qint64 bytesAvailable() const override;
    qint64 read(char *data, qint64 maxlen) override;

public slots:
    void uploadReadyReadSlot();
private:
    QFile file;
    qint64 totalBytes;
    bool hasUploadFinished;

    bool loadFileInfo();
};

class QNetworkAccessFileBackendFactory: public QNetworkAccessBackendFactory
{
public:
    virtual QStringList supportedSchemes() const override;
    virtual QNetworkAccessBackend *create(QNetworkAccessManager::Operation op,
                                          const QNetworkRequest &request) const override;
};

QT_END_NAMESPACE

#endif
