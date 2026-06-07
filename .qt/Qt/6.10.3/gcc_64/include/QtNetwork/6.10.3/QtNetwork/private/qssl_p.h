// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default


#ifndef QSSL_P_H
#define QSSL_P_H


//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of qsslcertificate.cpp.  This header file may change from version to version
// without notice, or even be removed.
//
// We mean it.
//

#include <QtNetwork/private/qtnetworkglobal_p.h>
#include <QtCore/QLoggingCategory>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcSsl)

namespace QTlsPrivate {

enum class Cipher {
    DesCbc,
    DesEde3Cbc,
    Rc2Cbc,
    Aes128Cbc,
    Aes192Cbc,
    Aes256Cbc
};

} // namespace QTlsPrivate

QT_END_NAMESPACE

#endif // QSSL_P_H
