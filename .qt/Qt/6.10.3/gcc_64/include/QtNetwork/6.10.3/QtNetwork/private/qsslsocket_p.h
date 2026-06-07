// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default


#ifndef QSSLSOCKET_P_H
#define QSSLSOCKET_P_H

#include "qsslsocket.h"

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtNetwork/private/qtnetworkglobal_p.h>

#include <private/qtcpsocket_p.h>

#include "qocspresponse.h"
#include "qsslconfiguration_p.h"
#include "qsslkey.h"
#include "qtlsbackend_p.h"

#include <QtCore/qlist.h>
#include <QtCore/qmutex.h>
#include <QtCore/qstringlist.h>

#include <memory>

QT_BEGIN_NAMESPACE

class QSslContext;
class QTlsBackend;

class Q_NETWORK_EXPORT QSslSocketPrivate : public QTcpSocketPrivate
{
    Q_DECLARE_PUBLIC(QSslSocket)
public:
    QSslSocketPrivate();
    virtual ~QSslSocketPrivate();

    void init();
    bool verifyProtocolSupported(const char *where);
    bool initialized;

    QSslSocket::SslMode mode;
    bool autoStartHandshake;
    bool connectionEncrypted;
    bool ignoreAllSslErrors;
    QList<QSslError> ignoreErrorsList;
    bool* readyReadEmittedPointer;

    QSslConfigurationPrivate configuration;

    // if set, this hostname is used for certificate validation instead of the hostname
    // that was used for connecting to.
    QString verificationPeerName;

    bool allowRootCertOnDemandLoading;

    static bool s_loadRootCertsOnDemand;

    static bool supportsSsl();
    static void ensureInitialized();

    static QList<QSslCipher> defaultCiphers();
    static QList<QSslCipher> defaultDtlsCiphers();
    static QList<QSslCipher> supportedCiphers();
    static void setDefaultCiphers(const QList<QSslCipher> &ciphers);
    static void setDefaultDtlsCiphers(const QList<QSslCipher> &ciphers);
    static void setDefaultSupportedCiphers(const QList<QSslCipher> &ciphers);

    static QList<QSslEllipticCurve> supportedEllipticCurves();
    static void setDefaultSupportedEllipticCurves(const QList<QSslEllipticCurve> &curves);
    static void resetDefaultEllipticCurves();

    static QList<QSslCertificate> defaultCaCertificates();
    static QList<QSslCertificate> systemCaCertificates();
    static void setDefaultCaCertificates(const QList<QSslCertificate> &certs);
    static void addDefaultCaCertificate(const QSslCertificate &cert);
    static void addDefaultCaCertificates(const QList<QSslCertificate> &certs);
    static bool isMatchingHostname(const QSslCertificate &cert, const QString &peerName);
    static bool isMatchingHostname(const QString &cn, const QString &hostname);

    // The socket itself, including private slots.
    QTcpSocket *plainSocket = nullptr;
    void createPlainSocket(QIODevice::OpenMode openMode);
    static void pauseSocketNotifiers(QSslSocket*);
    static void resumeSocketNotifiers(QSslSocket*);
    // ### The 2 methods below should be made member methods once the QSslContext class is made public
    static void checkSettingSslContext(QSslSocket*, std::shared_ptr<QSslContext>);
    static std::shared_ptr<QSslContext> sslContext(QSslSocket *socket);
    bool isPaused() const;
    void setPaused(bool p);
    bool bind(const QHostAddress &address, quint16, QAbstractSocket::BindMode, const QNetworkInterface *iface = nullptr) override;
    void _q_connectedSlot();
    void _q_hostFoundSlot();
    void _q_disconnectedSlot();
    void _q_stateChangedSlot(QAbstractSocket::SocketState);
    void _q_errorSlot(QAbstractSocket::SocketError);
    void _q_readyReadSlot();
    void _q_channelReadyReadSlot(int);
    void _q_bytesWrittenSlot(qint64);
    void _q_channelBytesWrittenSlot(int, qint64);
    void _q_readChannelFinishedSlot();
    void _q_flushWriteBuffer();
    void _q_flushReadBuffer();
    void _q_resumeImplementation();

    static QList<QByteArray> unixRootCertDirectories(); // used also by QSslContext

    qint64 peek(char *data, qint64 maxSize) override;
    QByteArray peek(qint64 maxSize) override;
    bool flush() override;

    void startClientEncryption();
    void startServerEncryption();
    void transmit();
    void disconnectFromHost();
    void disconnected();
    QSslCipher sessionCipher() const;
    QSsl::SslProtocol sessionProtocol() const;
    void continueHandshake();

    static bool rootCertOnDemandLoadingSupported();
    static void setRootCertOnDemandLoadingSupported(bool supported);

    static QTlsBackend *tlsBackendInUse();

    // Needed by TlsCryptograph:
    QSslSocket::SslMode tlsMode() const;
    bool isRootsOnDemandAllowed() const;
    QString verificationName() const;
    QString tlsHostName() const;
    QTcpSocket *plainTcpSocket() const;
    bool verifyErrorsHaveBeenIgnored();
    bool isAutoStartingHandshake() const;
    bool isPendingClose() const;
    void setPendingClose(bool pc);
    qint64 maxReadBufferSize() const;
    void setMaxReadBufferSize(qint64 maxSize);
    void setEncrypted(bool enc);
    QRingBufferRef &tlsWriteBuffer();
    QRingBufferRef &tlsBuffer();
    bool &tlsEmittedBytesWritten();
    bool *readyReadPointer();

protected:

    bool hasUndecryptedData() const;
    bool paused;
    bool flushTriggered;

    static inline QMutex backendMutex;
    static inline QString activeBackendName;
    static inline QTlsBackend *tlsBackend = nullptr;

    std::unique_ptr<QTlsPrivate::TlsCryptograph> backend;
};

QT_END_NAMESPACE

#endif
