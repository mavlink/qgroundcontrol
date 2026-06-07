// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QNETWORKACCESSCACHEBACKEND_P_H
#define QNETWORKACCESSCACHEBACKEND_P_H

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

QT_BEGIN_NAMESPACE

class QNetworkAccessCacheBackend : public QNetworkAccessBackend
{

public:
    QNetworkAccessCacheBackend();
    ~QNetworkAccessCacheBackend();

    void open() override;
    void close() override;
    bool start() override;
    qint64 bytesAvailable() const override;
    qint64 read(char *data, qint64 maxlen) override;

private:
    bool sendCacheContents();

    QIODevice *device = nullptr;
};

QT_END_NAMESPACE

#endif // QNETWORKACCESSCACHEBACKEND_P_H
