// Copyright (C) 2013 Ruslan Nigmatullin <euroelessar@yandex.ru>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:cryptography

#ifndef QMESSAGEAUTHENTICATIONCODE_H
#define QMESSAGEAUTHENTICATIONCODE_H

#include <QtCore/qcryptographichash.h>

QT_BEGIN_NAMESPACE


class QMessageAuthenticationCodePrivate;
class QIODevice;

// implemented in qcryptographichash.cpp
class Q_CORE_EXPORT QMessageAuthenticationCode
{
public:
#if QT_CORE_REMOVED_SINCE(6, 6)
    explicit QMessageAuthenticationCode(QCryptographicHash::Algorithm method,
                                        const QByteArray &key);
#endif
    explicit QMessageAuthenticationCode(QCryptographicHash::Algorithm method,
                                        QByteArrayView key = {});

    QMessageAuthenticationCode(QMessageAuthenticationCode &&other) noexcept
        : d{std::exchange(other.d, nullptr)} {}
    ~QMessageAuthenticationCode();

    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_MOVE_AND_SWAP(QMessageAuthenticationCode)
    void swap(QMessageAuthenticationCode &other) noexcept
    { qt_ptr_swap(d, other.d); }

    void reset() noexcept;

#if QT_CORE_REMOVED_SINCE(6, 6)
    void setKey(const QByteArray &key);
#endif
    void setKey(QByteArrayView key) noexcept;

    void addData(const char *data, qsizetype length);
#if QT_CORE_REMOVED_SINCE(6, 6)
    void addData(const QByteArray &data);
#endif
    void addData(QByteArrayView data) noexcept;
    bool addData(QIODevice *device);

    QByteArrayView resultView() const noexcept;
    QByteArray result() const;

#if QT_CORE_REMOVED_SINCE(6, 6)
    static QByteArray hash(const QByteArray &message, const QByteArray &key,
                           QCryptographicHash::Algorithm method);
#endif
    static QByteArray hash(QByteArrayView message, QByteArrayView key,
                           QCryptographicHash::Algorithm method);

    static QByteArrayView
    hashInto(QSpan<char> buffer, QByteArrayView message, QByteArrayView key,
             QCryptographicHash::Algorithm method) noexcept
    { return hashInto(as_writable_bytes(buffer), {&message, 1}, key, method); }
    static QByteArrayView
    hashInto(QSpan<uchar> buffer, QByteArrayView message, QByteArrayView key,
             QCryptographicHash::Algorithm method) noexcept
    { return hashInto(as_writable_bytes(buffer), {&message, 1}, key, method); }
    static QByteArrayView
    hashInto(QSpan<std::byte> buffer, QByteArrayView message,
             QByteArrayView key, QCryptographicHash::Algorithm method) noexcept
    { return hashInto(buffer, {&message, 1}, key, method); }
    static QByteArrayView
    hashInto(QSpan<char> buffer, QSpan<const QByteArrayView> messageParts,
             QByteArrayView key, QCryptographicHash::Algorithm method) noexcept
    { return hashInto(as_writable_bytes(buffer), messageParts, key, method); }
    static QByteArrayView
    hashInto(QSpan<uchar> buffer, QSpan<const QByteArrayView> messageParts,
             QByteArrayView key, QCryptographicHash::Algorithm method) noexcept
    { return hashInto(as_writable_bytes(buffer), messageParts, key, method); }
    static QByteArrayView
    hashInto(QSpan<std::byte> buffer, QSpan<const QByteArrayView> message,
             QByteArrayView key, QCryptographicHash::Algorithm method) noexcept;

private:
    Q_DISABLE_COPY(QMessageAuthenticationCode)
    QMessageAuthenticationCodePrivate *d;
};

QT_END_NAMESPACE

#endif
