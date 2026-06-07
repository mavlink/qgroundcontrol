// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQMLSSLKEY_P_H
#define QQMLSSLKEY_P_H

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

#include <QtCore/QByteArray>
#include <QtCore/QMetaType>
#include <QtNetwork/qsslkey.h>
#include <QtNetwork/qssl.h>
#include <QtQml/qqml.h>

QT_BEGIN_NAMESPACE

class Q_QMLNETWORK_EXPORT QQmlSslKey
{
    Q_GADGET
    QML_NAMED_ELEMENT(sslKey)
    QML_ADDED_IN_VERSION(6, 7)

    Q_PROPERTY(QString keyFile READ keyFile
                       WRITE setKeyFile)
    Q_PROPERTY(QSsl::KeyAlgorithm keyAlgorithm READ keyAlgorithm
                       WRITE setKeyAlgorithm)
    Q_PROPERTY(QSsl::EncodingFormat keyFormat READ keyFormat
                       WRITE setKeyFormat)
    Q_PROPERTY(QByteArray keyPassPhrase READ keyPassPhrase
                       WRITE setKeyPassPhrase)
    Q_PROPERTY(QSsl::KeyType keyType READ keyType WRITE setKeyType)

public:
    QSslKey getSslKey() const;
    QString keyFile() const { return m_keyFile; }
    QSsl::KeyAlgorithm keyAlgorithm() const { return m_keyAlgorithm; }
    QSsl::EncodingFormat keyFormat() const { return m_keyFormat; }
    QByteArray keyPassPhrase() const { return m_keyPassPhrase; }
    QSsl::KeyType keyType() const { return m_keyType; }

    void setKeyFile(const QString &key);
    void setKeyAlgorithm(QSsl::KeyAlgorithm value);
    void setKeyFormat(QSsl::EncodingFormat value);
    void setKeyPassPhrase(const QByteArray &value);
    void setKeyType(QSsl::KeyType type);

private:
    inline friend bool operator==(const QQmlSslKey &lvalue, const QQmlSslKey &rvalue)
    {
        return (lvalue.m_keyFile == rvalue.m_keyFile
                && lvalue.m_keyAlgorithm == rvalue.m_keyAlgorithm
                && lvalue.m_keyFormat == rvalue.m_keyFormat
                && lvalue.m_keyType == rvalue.m_keyType
                && lvalue.m_keyPassPhrase == rvalue.m_keyPassPhrase);
    }

    QString m_keyFile;
    QByteArray m_keyPassPhrase;
    QSsl::KeyAlgorithm m_keyAlgorithm = QSsl::Rsa;
    QSsl::EncodingFormat m_keyFormat = QSsl::Pem;
    QSsl::KeyType m_keyType = QSsl::PrivateKey;
};

QT_END_NAMESPACE

#endif // QQMLSSLKEY_P_H
