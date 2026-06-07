// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2014 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

/****************************************************************************
**
** In addition, as a special exception, the copyright holders listed above give
** permission to link the code of its release of Qt with the OpenSSL project's
** "OpenSSL" library (or modified versions of the "OpenSSL" library that use the
** same license as the original version), and distribute the linked executables.
**
** You must comply with the GNU General Public License version 2 in all
** respects for all of the code used other than the "OpenSSL" code.  If you
** modify this file, you may extend this exception to your version of the file,
** but you are not obligated to do so.  If you do not wish to do so, delete
** this exception statement from your version of this file.
**
****************************************************************************/

#ifndef QSSLCONFIGURATION_P_H
#define QSSLCONFIGURATION_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the QSslSocket API.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qmap.h>
#include <QtNetwork/private/qtnetworkglobal_p.h>
#include "qsslconfiguration.h"
#include "qlist.h"
#include "qsslcertificate.h"
#include "qsslcipher.h"
#include "qsslkey.h"
#include "qsslellipticcurve.h"
#include "qssldiffiehellmanparameters.h"

QT_BEGIN_NAMESPACE

class QSslConfigurationPrivate: public QSharedData
{
public:
    QSslConfigurationPrivate()
        : sessionProtocol(QSsl::UnknownProtocol),
          protocol(QSsl::SecureProtocols),
          peerVerifyMode(QSslSocket::AutoVerifyPeer),
          peerVerifyDepth(0),
          allowRootCertOnDemandLoading(true),
          peerSessionShared(false),
          sslOptions(QSslConfigurationPrivate::defaultSslOptions),
          dhParams(QSslDiffieHellmanParameters::defaultParameters()),
          sslSessionTicketLifeTimeHint(-1),
          ephemeralServerKey(),
          preSharedKeyIdentityHint(),
          nextProtocolNegotiationStatus(QSslConfiguration::NextProtocolNegotiationNone)
    { }

    QSslCertificate peerCertificate;
    QList<QSslCertificate> peerCertificateChain;

    QList<QSslCertificate> localCertificateChain;

    QSslKey privateKey;
    QSslCipher sessionCipher;
    QSsl::SslProtocol sessionProtocol;
    QList<QSslCipher> ciphers;
    QList<QSslCertificate> caCertificates;

    QSsl::SslProtocol protocol;
    QSslSocket::PeerVerifyMode peerVerifyMode;
    int peerVerifyDepth;
    bool allowRootCertOnDemandLoading;
    bool peerSessionShared;

    Q_AUTOTEST_EXPORT static bool peerSessionWasShared(const QSslConfiguration &configuration);

    QSsl::SslOptions sslOptions;

    static const QSsl::SslOptions defaultSslOptions;

    QList<QSslEllipticCurve> ellipticCurves;

    QSslDiffieHellmanParameters dhParams;

    QMap<QByteArray, QVariant> backendConfig;

    QByteArray sslSession;
    int sslSessionTicketLifeTimeHint;

    QSslKey ephemeralServerKey;

    QByteArray preSharedKeyIdentityHint;

    QList<QByteArray> nextAllowedProtocols;
    QByteArray nextNegotiatedProtocol;
    QSslConfiguration::NextProtocolNegotiationStatus nextProtocolNegotiationStatus;

#if QT_CONFIG(dtls)
    bool dtlsCookieEnabled = true;
#else
    const bool dtlsCookieEnabled = false;
#endif // dtls

#if QT_CONFIG(ocsp)
    bool ocspStaplingEnabled = false;
#else
    const bool ocspStaplingEnabled = false;
#endif

#if QT_CONFIG(openssl)
    bool reportFromCallback = false;
    bool missingCertIsFatal = false;
#else
    const bool reportFromCallback = false;
    const bool missingCertIsFatal = false;
#endif // openssl

    // in qsslsocket.cpp:
    static QSslConfiguration defaultConfiguration();
    static void setDefaultConfiguration(const QSslConfiguration &configuration);
    static void deepCopyDefaultConfiguration(QSslConfigurationPrivate *config);

    static QSslConfiguration defaultDtlsConfiguration();
    static void setDefaultDtlsConfiguration(const QSslConfiguration &configuration);
};

// implemented here for inlining purposes
inline QSslConfiguration::QSslConfiguration(QSslConfigurationPrivate *dd)
    : d(dd)
{
}

QT_END_NAMESPACE

#endif
