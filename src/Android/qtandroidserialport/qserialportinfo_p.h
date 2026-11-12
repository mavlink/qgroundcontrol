// Copyright (C) 2011-2012 Denis Shienkov <denis.shienkov@gmail.com>
// Copyright (C) 2017 Sergey Belyashov <Sergey.Belyashov@gmail.com>
// Copyright (C) 2013 Laszlo Papp <lpapp@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

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

#include <QtCore/private/qglobal_p.h>
#include <QtCore/qlist.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

class QSerialPortInfo;

class Q_AUTOTEST_EXPORT QSerialPortInfoPrivate
{
public:
    static QString portNameToSystemLocation(const QString &source);
    static QString portNameFromSystemLocation(const QString &source);
    /// Whether a system location names a port of a USB device, which Java opens: the name that Android gives the
    /// device, or the one that survives a replug. Any other location is a device node.
    static bool isUsbLocation(const QString& location);
    /// The serial device nodes in devDir that QGC may read and write: the device's own UARTs and the USB serial
    /// adapters that the kernel drives. sysTtyDir is the kernel's /sys/class/tty, which describes the adapters.
    static QList<QSerialPortInfo> posixPorts(const QString& devDir, const QString& sysTtyDir);

    QString portName;
    QString stableName;
    QString device;
    QString description;
    QString manufacturer;
    QString serialNumber;

    quint16 vendorIdentifier = 0;
    quint16 productIdentifier = 0;

    bool hasVendorIdentifier = false;
    bool hasProductIdentifier = false;
};

QT_END_NAMESPACE
