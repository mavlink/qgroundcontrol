// Copyright (C) 2011-2012 Denis Shienkov <denis.shienkov@gmail.com>
// Copyright (C) 2011 Sergey Belyashov <Sergey.Belyashov@gmail.com>
// Copyright (C) 2012 Laszlo Papp <lpapp@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qserialportinfo.h"
#include "qserialportinfo_p.h"
#include "qserialport_p.h"

#include <AndroidSerial.h>
#include <QGCLoggingCategory.h>

#include <QtCore/QStringList>

QGC_LOGGING_CATEGORY(QSerialPortInfo_AndroidLog, "qgc.android.libs.qtandroidserialport.qserialportinfo_android")

QT_BEGIN_NAMESPACE

QList<QSerialPortInfo> availablePortsByFiltersOfDevices(bool &ok)
{
    const QList<QSerialPortInfo> serialPortInfoList = AndroidSerial::availableDevices();
    ok = !serialPortInfoList.isEmpty();
    return serialPortInfoList;
}

QList<QSerialPortInfo> availablePortsBySysfs(bool &ok)
{
    ok = false;
    return QList<QSerialPortInfo>();
}

QList<QSerialPortInfo> availablePortsByUdev(bool &ok)
{
    ok = false;
    return QList<QSerialPortInfo>();
}

QList<QSerialPortInfo> QSerialPortInfo::availablePorts()
{
    bool ok = false;
    const QList<QSerialPortInfo> serialPortInfoList = availablePortsByFiltersOfDevices(ok);
    return (ok ? serialPortInfoList : QList<QSerialPortInfo>());
}

QString QSerialPortInfoPrivate::portNameToSystemLocation(const QString &source)
{
    return (source.startsWith(QLatin1Char('/'))
            || source.startsWith(QLatin1String("./"))
            || source.startsWith(QLatin1String("../")))
            ? source : (QLatin1String("/dev/") + source);
}

QString QSerialPortInfoPrivate::portNameFromSystemLocation(const QString &source)
{
    return source.startsWith(QLatin1String("/dev/"))
            ? source.mid(5) : source;
}

QT_END_NAMESPACE
