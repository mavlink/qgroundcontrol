// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default


#ifndef QSSLKEY_OPENSSL_P_H
#define QSSLKEY_OPENSSL_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of qsslcertificate.cpp.  This header file may change from version to version
// without notice, or even be removed.
//
// We mean it.
//

#include <QtNetwork/private/qtnetworkglobal_p.h>

#include "qsslkey.h"
#include "qssl_p.h"

#include <memory>

QT_BEGIN_NAMESPACE

namespace QTlsPrivate {
class TlsKey;
}

class QSslKeyPrivate
{
public:
    QSslKeyPrivate();
    ~QSslKeyPrivate();

    using Cipher = QTlsPrivate::Cipher;

    Q_NETWORK_EXPORT static QByteArray decrypt(Cipher cipher, const QByteArray &data, const QByteArray &key, const QByteArray &iv);
    Q_NETWORK_EXPORT static QByteArray encrypt(Cipher cipher, const QByteArray &data, const QByteArray &key, const QByteArray &iv);

    std::unique_ptr<QTlsPrivate::TlsKey> backend;
    QAtomicInt ref;

private:
    Q_DISABLE_COPY_MOVE(QSslKeyPrivate)
};

QT_END_NAMESPACE

#endif // QSSLKEY_OPENSSL_P_H
