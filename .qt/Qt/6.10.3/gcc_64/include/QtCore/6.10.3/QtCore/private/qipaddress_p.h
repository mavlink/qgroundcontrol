// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QIPADDRESS_P_H
#define QIPADDRESS_P_H

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

#include <QtCore/private/qglobal_p.h>
#include "qstring.h"

QT_BEGIN_NAMESPACE

namespace QIPAddressUtils {

typedef quint32 IPv4Address;
typedef quint8 IPv6Address[16];

Q_CORE_EXPORT bool parseIp4(IPv4Address &address, const QChar *begin, const QChar *end);
Q_CORE_EXPORT const QChar *parseIp6(IPv6Address &address, const QChar *begin, const QChar *end);
Q_CORE_EXPORT void toString(QString &appendTo, IPv4Address address);
Q_CORE_EXPORT void toString(QString &appendTo, const IPv6Address address);

} // namespace

QT_END_NAMESPACE

#endif // QIPADDRESS_P_H
