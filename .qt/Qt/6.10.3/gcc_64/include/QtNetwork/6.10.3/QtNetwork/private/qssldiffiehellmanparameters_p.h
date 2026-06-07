// Copyright (C) 2015 Mikkel Krautz <mikkel@krautz.dk>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default


#ifndef QSSLDIFFIEHELLMANPARAMETERS_P_H
#define QSSLDIFFIEHELLMANPARAMETERS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of qssldiffiehellmanparameters.cpp.  This header file may change from version to version
// without notice, or even be removed.
//
// We mean it.
//

#include <QtNetwork/private/qtnetworkglobal_p.h>

#include "qssldiffiehellmanparameters.h"

#include <QSharedData>

QT_BEGIN_NAMESPACE

class QSslDiffieHellmanParametersPrivate : public QSharedData
{
public:
    void initFromDer(const QByteArray &der);
    void initFromPem(const QByteArray &pem);

    QSslDiffieHellmanParameters::Error error = QSslDiffieHellmanParameters::NoError;
    QByteArray derData;
};

QT_END_NAMESPACE

#endif // QSSLDIFFIEHELLMANPARAMETERS_P_H
