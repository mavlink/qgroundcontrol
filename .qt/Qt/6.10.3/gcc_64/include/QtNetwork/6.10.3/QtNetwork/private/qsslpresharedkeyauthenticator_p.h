// Copyright (C) 2014 Governikus GmbH & Co. KG.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QSSLPRESHAREDKEYAUTHENTICATOR_P_H
#define QSSLPRESHAREDKEYAUTHENTICATOR_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtNetwork/private/qtnetworkglobal_p.h>
#include <QSharedData>

QT_BEGIN_NAMESPACE

class QSslPreSharedKeyAuthenticatorPrivate : public QSharedData
{
public:
    QSslPreSharedKeyAuthenticatorPrivate();

    QByteArray identityHint;

    QByteArray identity;
    int maximumIdentityLength;

    QByteArray preSharedKey;
    int maximumPreSharedKeyLength;
};

QT_END_NAMESPACE

#endif // QSSLPRESHAREDKEYAUTHENTICATOR_P_H
