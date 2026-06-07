// Copyright (C) 2017 The Qt Company Ltd.
// Copyright (C) 2017 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QNETWORKINTERFACE_UNIX_P_H
#define QNETWORKINTERFACE_UNIX_P_H

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

#include "qnetworkinterface_p.h"
#include "private/qnet_unix_p.h"

#ifndef QT_NO_NETWORKINTERFACE

#define IP_MULTICAST    // make AIX happy and define IFF_MULTICAST

#include <sys/types.h>
#include <sys/socket.h>
#ifdef Q_OS_SOLARIS
#  include <sys/sockio.h>
#endif
#ifdef Q_OS_HAIKU
#  include <sys/sockio.h>
#  define IFF_RUNNING 0x0001
#endif
#if QT_CONFIG(linux_netlink)
// Same as net/if.h but contains other things we need in
// qnetworkinterface_linux.cpp.
#  include <linux/if.h>
#else
#  include <net/if.h>
#endif

QT_BEGIN_NAMESPACE

static QNetworkInterface::InterfaceFlags convertFlags(uint rawFlags)
{
    QNetworkInterface::InterfaceFlags flags;
    flags |= (rawFlags & IFF_UP) ? QNetworkInterface::IsUp : QNetworkInterface::InterfaceFlag(0);
    flags |= (rawFlags & IFF_RUNNING) ? QNetworkInterface::IsRunning : QNetworkInterface::InterfaceFlag(0);
    flags |= (rawFlags & IFF_BROADCAST) ? QNetworkInterface::CanBroadcast : QNetworkInterface::InterfaceFlag(0);
    flags |= (rawFlags & IFF_LOOPBACK) ? QNetworkInterface::IsLoopBack : QNetworkInterface::InterfaceFlag(0);
#ifdef IFF_POINTOPOINT //cygwin doesn't define IFF_POINTOPOINT
    flags |= (rawFlags & IFF_POINTOPOINT) ? QNetworkInterface::IsPointToPoint : QNetworkInterface::InterfaceFlag(0);
#endif

#ifdef IFF_MULTICAST
    flags |= (rawFlags & IFF_MULTICAST) ? QNetworkInterface::CanMulticast : QNetworkInterface::InterfaceFlag(0);
#endif
    return flags;
}

QT_END_NAMESPACE

#endif // QT_NO_NETWORKINTERFACE

#endif // QNETWORKINTERFACE_UNIX_P_H
