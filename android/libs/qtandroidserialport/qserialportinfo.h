// Copyright (C) 2012 Denis Shienkov <denis.shienkov@gmail.com>
// Copyright (C) 2012 Laszlo Papp <lpapp@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSERIALPORTINFO_H
#define QSERIALPORTINFO_H

#include <QtCore/qlist.h>
#include <QtCore/qscopedpointer.h>

#include "qserialportglobal.h"

QT_BEGIN_NAMESPACE

class QSerialPort;
class QSerialPortInfoPrivate;

class Q_SERIALPORT_EXPORT QSerialPortInfo
{
    Q_DECLARE_PRIVATE(QSerialPortInfo)
public:
    QSerialPortInfo();
    explicit QSerialPortInfo(const QSerialPort &port);
    explicit QSerialPortInfo(const QString &name);
    QSerialPortInfo(const QSerialPortInfo &other);
    QSerialPortInfo(const QSerialPortInfoPrivate &dd);
    ~QSerialPortInfo();

    QSerialPortInfo& operator=(const QSerialPortInfo &other);
    void swap(QSerialPortInfo &other);

    QString portName() const;
    QString systemLocation() const;
    QString description() const;
    QString manufacturer() const;
    QString serialNumber() const;

    quint16 vendorIdentifier() const;
    quint16 productIdentifier() const;

    bool hasVendorIdentifier() const;
    bool hasProductIdentifier() const;

    bool isNull() const;

    static QList<qint32> standardBaudRates();
    static QList<QSerialPortInfo> availablePorts();

private:
    // QSerialPortInfo(const QSerialPortInfoPrivate &dd);
    friend QList<QSerialPortInfo> availablePortsByUdev(bool &ok);
    friend QList<QSerialPortInfo> availablePortsBySysfs(bool &ok);
    friend QList<QSerialPortInfo> availablePortsByFiltersOfDevices(bool &ok);
    std::unique_ptr<QSerialPortInfoPrivate> d_ptr;
};

inline bool QSerialPortInfo::isNull() const
{ return !d_ptr; }

QT_END_NAMESPACE

#endif // QSERIALPORTINFO_H
