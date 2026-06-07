// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QPASSWORDDIGESTOR_H
#define QPASSWORDDIGESTOR_H

#if 0
#pragma qt_class(QPasswordDigestor)
#endif

#include <QtNetwork/qtnetworkglobal.h>
#include <QtCore/QByteArray>
#include <QtCore/QCryptographicHash>

QT_BEGIN_NAMESPACE

namespace QPasswordDigestor {
Q_NETWORK_EXPORT QByteArray deriveKeyPbkdf1(QCryptographicHash::Algorithm algorithm,
                                   const QByteArray &password, const QByteArray &salt,
                                   int iterations, quint64 dkLen);
Q_NETWORK_EXPORT QByteArray deriveKeyPbkdf2(QCryptographicHash::Algorithm algorithm,
                                   const QByteArray &password, const QByteArray &salt,
                                   int iterations, quint64 dkLen);
} // namespace QPasswordDigestor

QT_END_NAMESPACE

#endif // QPASSWORDDIGESTOR_H
