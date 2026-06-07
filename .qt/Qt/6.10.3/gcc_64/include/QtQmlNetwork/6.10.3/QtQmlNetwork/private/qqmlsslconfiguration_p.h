// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQMLSSLCONFIGURATION_P_H
#define QQMLSSLCONFIGURATION_P_H

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

#include <qtqmlnetworkexports.h>
#include "qqmlsslkey_p.h"

#include <QtCore/QByteArray>
#include <QtCore/QMetaType>
#include <QtQml/qqml.h>
#include <QtNetwork/qsslconfiguration.h>
#include <QtNetwork/qsslsocket.h>
#include <QtNetwork/qssl.h>

QT_BEGIN_NAMESPACE

class Q_QMLNETWORK_EXPORT QQmlSslConfiguration
{
    Q_GADGET

    Q_PROPERTY(QString ciphers READ ciphers WRITE setCiphers)
    Q_PROPERTY(QList<QSsl::SslOption> sslOptions READ sslOptions WRITE setSslOptions)
    Q_PROPERTY(QSsl::SslProtocol protocol READ protocol WRITE setProtocol)
    Q_PROPERTY(QSslSocket::PeerVerifyMode peerVerifyMode READ peerVerifyMode
                       WRITE setPeerVerifyMode)
    Q_PROPERTY(int peerVerifyDepth READ peerVerifyDepth WRITE setPeerVerifyDepth)
    Q_PROPERTY(QByteArray sessionTicket READ sessionTicket WRITE setSessionTicket)

public:
    Q_INVOKABLE void setCertificateFiles(const QStringList &certificateFiles);
    Q_INVOKABLE void setPrivateKey(const QQmlSslKey &privateKey);

    QString ciphers() const;
    QList<QSsl::SslOption> sslOptions() const;
    QSsl::SslProtocol protocol() const;
    QSslSocket::PeerVerifyMode peerVerifyMode() const;
    int peerVerifyDepth() const;
    QByteArray sessionTicket() const;
    QSslConfiguration const configuration();

    void setProtocol(QSsl::SslProtocol protocol);
    void setPeerVerifyMode(QSslSocket::PeerVerifyMode mode);
    void setPeerVerifyDepth(int depth);
    void setCiphers(const QString &ciphers);
    void setSslOptions(const QList<QSsl::SslOption> &options);
    void setSessionTicket(const QByteArray &sessionTicket);

private:
    inline friend bool operator==(const QQmlSslConfiguration &lval,
                                  const QQmlSslConfiguration &rval)
    {
        return lval.m_certificateFiles == rval.m_certificateFiles
                && lval.m_ciphers == rval.m_ciphers
                && lval.m_sslOptions == rval.m_sslOptions
                && lval.m_configuration == rval.m_configuration;
    }

    inline friend bool operator!=(const QQmlSslConfiguration &lval,
                                  const QQmlSslConfiguration &rval)
    {
        return !(lval == rval);
    }

protected:
    void setSslOptionsList(const QSslConfiguration &configuration);
    void setCiphersList(const QSslConfiguration &configuration);

    QStringList m_certificateFiles;
    QString m_ciphers;
    QList<QSsl::SslOption> m_sslOptions;
    QSslConfiguration m_configuration;
};

class Q_QMLNETWORK_EXPORT QQmlSslDefaultConfiguration : public QQmlSslConfiguration
{
    Q_GADGET
    QML_NAMED_ELEMENT(sslConfiguration)
    QML_ADDED_IN_VERSION(6, 7)

public:
    QQmlSslDefaultConfiguration();
};

class Q_QMLNETWORK_EXPORT QQmlSslDefaultDtlsConfiguration : public QQmlSslConfiguration
{
    Q_GADGET
    QML_NAMED_ELEMENT(sslDtlsConfiguration)
    QML_ADDED_IN_VERSION(6, 7)

public:
    QQmlSslDefaultDtlsConfiguration();
};

QT_END_NAMESPACE

#endif // QQMLSSLCONFIGURATION_P_H
