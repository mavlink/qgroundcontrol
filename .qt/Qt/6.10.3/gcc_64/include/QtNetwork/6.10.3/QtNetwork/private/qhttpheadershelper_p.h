// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QHTTPHEADERSHELPER_H
#define QHTTPHEADERSHELPER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the Network Access API.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtNetwork/private/qtnetworkglobal_p.h>

QT_BEGIN_NAMESPACE

class QHttpHeaders;

namespace QHttpHeadersHelper {
    Q_NETWORK_EXPORT bool compareStrict(const QHttpHeaders &left, const QHttpHeaders &right);
};

QT_END_NAMESPACE

#endif // QHTTPHEADERSHELPER_H
