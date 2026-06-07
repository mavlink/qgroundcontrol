// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QAUTHENTICATOR_P_H
#define QAUTHENTICATOR_P_H

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

#include <QtNetwork/private/qtnetworkglobal_p.h>
#include <qhash.h>
#include <qbytearray.h>
#include <qscopedpointer.h>
#include <qstring.h>
#include <qauthenticator.h>
#include <qvariant.h>

QT_BEGIN_NAMESPACE

class QHttpResponseHeader;
class QHttpHeaders;
#if QT_CONFIG(sspi) // SSPI
class QSSPIWindowsHandles;
#elif QT_CONFIG(gssapi) // GSSAPI
class QGssApiHandles;
#endif

class Q_NETWORK_EXPORT QAuthenticatorPrivate
{
public:
    enum Method { None, Basic, Negotiate, Ntlm, DigestMd5, };
    QAuthenticatorPrivate();
    ~QAuthenticatorPrivate();

    QString user;
    QString extractedUser;
    QString password;
    QVariantHash options;
    Method method;
    QString realm;
    QByteArray challenge;
#if QT_CONFIG(sspi) // SSPI
    QScopedPointer<QSSPIWindowsHandles> sspiWindowsHandles;
#elif QT_CONFIG(gssapi) // GSSAPI
    QScopedPointer<QGssApiHandles> gssApiHandles;
#endif
    bool hasFailed; //credentials have been tried but rejected by server.

    enum Phase {
        Start,
        Phase1,
        Phase2,
        Done,
        Invalid
    };
    Phase phase;

    // digest specific
    QByteArray cnonce;
    int nonceCount;

    // ntlm specific
    QString workstation;
    QString userDomain;

    QByteArray calculateResponse(QByteArrayView method, QByteArrayView path, QStringView host);

    inline static QAuthenticatorPrivate *getPrivate(QAuthenticator &auth) { return auth.d; }
    inline static const QAuthenticatorPrivate *getPrivate(const QAuthenticator &auth) { return auth.d; }

    QByteArray digestMd5Response(QByteArrayView challenge, QByteArrayView method,
                                 QByteArrayView path);
    static QHash<QByteArray, QByteArray>
    parseDigestAuthenticationChallenge(QByteArrayView challenge);

    void parseHttpResponse(const QHttpHeaders &headers, bool isProxy, QStringView host);
    void updateCredentials();

    static bool isMethodSupported(QByteArrayView method);
};


QT_END_NAMESPACE

#endif
