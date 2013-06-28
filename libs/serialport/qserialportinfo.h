/****************************************************************************
**
** Copyright (C) 2012 Denis Shienkov <denis.shienkov@gmail.com>
** Copyright (C) 2012 Laszlo Papp <lpapp@kde.org>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtSerialPort module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QSERIALPORTINFO_H
#define QSERIALPORTINFO_H

#include <QtCore/qlist.h>
#include <QtCore/qscopedpointer.h>

#include <qserialportglobal.h>
//#include <QtSerialPort/qserialportglobal.h>

QT_BEGIN_NAMESPACE

class QSerialPort;
class QSerialPortInfoPrivate;
class QSerialPortInfoPrivateDeleter;

class Q_SERIALPORT_EXPORT QSerialPortInfo
{
    Q_DECLARE_PRIVATE(QSerialPortInfo)
public:
    QSerialPortInfo();
    explicit QSerialPortInfo(const QSerialPort &port);
    explicit QSerialPortInfo(const QString &name);
    QSerialPortInfo(const QSerialPortInfo &other);
    ~QSerialPortInfo();

    QSerialPortInfo& operator=(const QSerialPortInfo &other);
    void swap(QSerialPortInfo &other);

    QString portName() const;
    QString systemLocation() const;
    QString description() const;
    QString manufacturer() const;

    quint16 vendorIdentifier() const;
    quint16 productIdentifier() const;

    bool hasVendorIdentifier() const;
    bool hasProductIdentifier() const;

    bool isNull() const;
    bool isBusy() const;
    bool isValid() const;

    static QList<qint32> standardBaudRates();
    static QList<QSerialPortInfo> availablePorts();

private:
    QScopedPointer<QSerialPortInfoPrivate, QSerialPortInfoPrivateDeleter> d_ptr;
};

inline bool QSerialPortInfo::isNull() const
{ return !d_ptr; }

QT_END_NAMESPACE

#endif // QSERIALPORTINFO_H
