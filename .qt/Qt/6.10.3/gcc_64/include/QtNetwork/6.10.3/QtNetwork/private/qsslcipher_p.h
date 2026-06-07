// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QSSLCIPHER_P_H
#define QSSLCIPHER_P_H

#include <QtNetwork/private/qtnetworkglobal_p.h>
#include "qsslcipher.h"

QT_BEGIN_NAMESPACE

//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the QLibrary class.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

class QSslCipherPrivate
{
public:
    QSslCipherPrivate()
        : isNull(true), supportedBits(0), bits(0),
          exportable(false), protocol(QSsl::UnknownProtocol)
    {
    }

    bool isNull;
    QString name;
    int supportedBits;
    int bits;
    QString keyExchangeMethod;
    QString authenticationMethod;
    QString encryptionMethod;
    bool exportable;
    QString protocolString;
    QSsl::SslProtocol protocol;
};

QT_END_NAMESPACE

#endif // QSSLCIPHER_P_H
