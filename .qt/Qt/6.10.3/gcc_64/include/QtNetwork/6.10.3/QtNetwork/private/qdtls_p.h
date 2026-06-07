// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QDTLS_P_H
#define QDTLS_P_H

#include <private/qtnetworkglobal_p.h>

#include "qtlsbackend_p.h"

#include <QtCore/private/qobject_p.h>
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

QT_REQUIRE_CONFIG(dtls);

QT_BEGIN_NAMESPACE

class QHostAddress;

class QDtlsClientVerifierPrivate : public QObjectPrivate
{
public:
    QDtlsClientVerifierPrivate();
    ~QDtlsClientVerifierPrivate();
    std::unique_ptr<QTlsPrivate::DtlsCookieVerifier> backend;
};

class QDtlsPrivate : public QObjectPrivate
{
public:
    QDtlsPrivate();
    ~QDtlsPrivate();
    std::unique_ptr<QTlsPrivate::DtlsCryptograph> backend;
};

QT_END_NAMESPACE

#endif // QDTLS_P_H
