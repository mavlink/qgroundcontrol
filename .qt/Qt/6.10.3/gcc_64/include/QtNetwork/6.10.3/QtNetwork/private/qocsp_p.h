// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QOCSP_P_H
#define QOCSP_P_H

#include <QtNetwork/private/qtnetworkglobal_p.h>

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

// Note, this file is a workaround: on 64-bit Windows one of OpenSSL
// includes combined with openssl/ocsp.h results in macros from
// wincrypt.h exposed. OpenSSL's own very "unique" and "inventive"
// names like OCSP_RESPONSE or X509_NAME were asking to clash with
// other entities (presumably macros) with the same names. Normally,
// ossl_typ.h un-defines them, but due to a bug in OpenSSL, fails
// to do this on Win 64. Thus we have to do it here. We only undef
// 3 names, ossl_typ.h has more, but apparently we don't need them
// (no name clash so far).

QT_REQUIRE_CONFIG(ocsp);

#ifdef Q_OS_WIN
#undef X509_NAME
#undef OCSP_REQUEST
#undef OCSP_RESPONSE
#endif // Q_OS_WIN

#include <openssl/ocsp.h>

#endif // QOCSP_P_H
