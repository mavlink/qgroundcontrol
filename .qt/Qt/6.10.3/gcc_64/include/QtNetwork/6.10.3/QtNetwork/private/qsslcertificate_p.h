// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default


#ifndef QSSLCERTIFICATE_P_H
#define QSSLCERTIFICATE_P_H

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

#include "qsslcertificateextension.h"
#include "qsslcertificate.h"
#include "qtlsbackend_p.h"

#include <qlist.h>

#include <memory>

QT_BEGIN_NAMESPACE

class QSslCertificatePrivate
{
public:
    QSslCertificatePrivate();
    ~QSslCertificatePrivate();

    QList<QSslCertificateExtension> extensions() const;
    Q_NETWORK_EXPORT static bool isBlacklisted(const QSslCertificate &certificate);
    Q_NETWORK_EXPORT static QByteArray subjectInfoToString(QSslCertificate::SubjectInfo info);

    QAtomicInt ref;
    std::unique_ptr<QTlsPrivate::X509Certificate> backend;
};

QT_END_NAMESPACE

#endif // QSSLCERTIFICATE_P_H
