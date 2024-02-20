/****************************************************************************
**
** Copyright (C) 2011-2012 Denis Shienkov <denis.shienkov@gmail.com>
** Copyright (C) 2011 Sergey Belyashov <Sergey.Belyashov@gmail.com>
** Copyright (C) 2012 Laszlo Papp <lpapp@kde.org>
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtSerialPort module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qserialportinfo.h"
#include "qserialportinfo_p.h"
#include "qserialport.h"

QT_BEGIN_NAMESPACE

QSerialPortInfo::QSerialPortInfo()
{
}

QSerialPortInfo::QSerialPortInfo(const QSerialPortInfo &other)
    : d_ptr(other.d_ptr ? new QSerialPortInfoPrivate(*other.d_ptr) : Q_NULLPTR)
{
}

QSerialPortInfo::QSerialPortInfo(const QSerialPort &port)
{
    foreach (const QSerialPortInfo &serialPortInfo, availablePorts()) {
        if (port.portName() == serialPortInfo.portName()) {
            *this = serialPortInfo;
            break;
        }
    }
}

QSerialPortInfo::QSerialPortInfo(const QString &name)
{
    foreach (const QSerialPortInfo &serialPortInfo, availablePorts()) {
        if (name == serialPortInfo.portName()) {
            *this = serialPortInfo;
            break;
        }
    }
}

QSerialPortInfo::QSerialPortInfo(const QSerialPortInfoPrivate &dd)
    : d_ptr(new QSerialPortInfoPrivate(dd))
{
}

QSerialPortInfo::~QSerialPortInfo()
{
}

#if 0
void QSerialPortInfo::swap(QSerialPortInfo &other)
{
    d_ptr.swap(other.d_ptr);
}

QSerialPortInfo& QSerialPortInfo::operator=(const QSerialPortInfo &other)
{
    QSerialPortInfo(other).swap(*this);
    return *this;
}
#endif

QString QSerialPortInfo::portName() const
{
    Q_D(const QSerialPortInfo);
    return !d ? QString() : d->portName;
}

QString QSerialPortInfo::systemLocation() const
{
    Q_D(const QSerialPortInfo);
    return !d ? QString() : d->device;
}

QString QSerialPortInfo::description() const
{
    Q_D(const QSerialPortInfo);
    return !d ? QString() : d->description;
}

QString QSerialPortInfo::manufacturer() const
{
    Q_D(const QSerialPortInfo);
    return !d ? QString() : d->manufacturer;
}

QString QSerialPortInfo::serialNumber() const
{
    Q_D(const QSerialPortInfo);
    return !d ? QString() : d->serialNumber;
}

quint16 QSerialPortInfo::vendorIdentifier() const
{
    Q_D(const QSerialPortInfo);
    return !d ? 0 : d->vendorIdentifier;
}

quint16 QSerialPortInfo::productIdentifier() const
{
    Q_D(const QSerialPortInfo);
    return !d ? 0 : d->productIdentifier;
}

bool QSerialPortInfo::hasVendorIdentifier() const
{
    Q_D(const QSerialPortInfo);
    return !d ? false : d->hasVendorIdentifier;
}

bool QSerialPortInfo::hasProductIdentifier() const
{
    Q_D(const QSerialPortInfo);
    return !d ? false : d->hasProductIdentifier;
}

QT_END_NAMESPACE
