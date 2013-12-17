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

#ifndef QSERIALPORT_UNIX_P_H
#define QSERIALPORT_UNIX_P_H

#include "qserialport_p.h"

#include <limits.h>
#include <termios.h>
#ifdef Q_OS_LINUX
#  include <linux/serial.h>
#endif

QT_BEGIN_NAMESPACE

class QSocketNotifier;

class QSerialPortPrivate : public QSerialPortPrivateData
{
public:
    QSerialPortPrivate(QSerialPort *q);

    bool open(QIODevice::OpenMode mode);
    void close();

    QSerialPort::PinoutSignals pinoutSignals() const;

    bool setDataTerminalReady(bool set);
    bool setRequestToSend(bool set);

    bool flush();
    bool clear(QSerialPort::Directions dir);

    bool sendBreak(int duration);
    bool setBreakEnabled(bool set);

    qint64 systemInputQueueSize () const;
    qint64 systemOutputQueueSize () const;

    qint64 bytesAvailable() const;

    qint64 readFromBuffer(char *data, qint64 maxSize);
    qint64 writeToBuffer(const char *data, qint64 maxSize);

    bool waitForReadyRead(int msecs);
    bool waitForBytesWritten(int msecs);

    bool setBaudRate(qint32 baudRate, QSerialPort::Directions dir);
    bool setDataBits(QSerialPort::DataBits dataBits);
    bool setParity(QSerialPort::Parity parity);
    bool setStopBits(QSerialPort::StopBits stopBits);
    bool setFlowControl(QSerialPort::FlowControl flow);
    bool setDataErrorPolicy(QSerialPort::DataErrorPolicy policy);

    bool readNotification();
    bool writeNotification(int maxSize = INT_MAX);
    bool exceptionNotification();

    static QString portNameToSystemLocation(const QString &port);
    static QString portNameFromSystemLocation(const QString &location);

    static qint32 baudRateFromSetting(qint32 setting);
    static qint32 settingFromBaudRate(qint32 baudRate);

    static QList<qint32> standardBaudRates();

    struct termios currentTermios;
    struct termios restoredTermios;
#ifdef Q_OS_LINUX
    struct serial_struct currentSerialInfo;
    struct serial_struct restoredSerialInfo;
#endif
    int descriptor;
    bool isCustomBaudRateSupported;

    QSocketNotifier *readNotifier;
    QSocketNotifier *writeNotifier;
    QSocketNotifier *exceptionNotifier;

    bool readPortNotifierCalled;
    bool readPortNotifierState;
    bool readPortNotifierStateSet;

    bool emittedReadyRead;
    bool emittedBytesWritten;

private:
    bool updateTermios();

    void detectDefaultSettings();
    QSerialPort::SerialPortError decodeSystemError() const;

    bool isReadNotificationEnabled() const;
    void setReadNotificationEnabled(bool enable);
    bool isWriteNotificationEnabled() const;
    void setWriteNotificationEnabled(bool enable);
    bool isExceptionNotificationEnabled() const;
    void setExceptionNotificationEnabled(bool enable);

    bool waitForReadOrWrite(bool *selectForRead, bool *selectForWrite,
                            bool checkRead, bool checkWrite,
                            int msecs, bool *timedOut);

    qint64 readFromPort(char *data, qint64 maxSize);
    qint64 writeToPort(const char *data, qint64 maxSize);

#ifndef CMSPAR
    qint64 writePerChar(const char *data, qint64 maxSize);
#endif
    qint64 readPerChar(char *data, qint64 maxSize);

};

QT_END_NAMESPACE

#endif // QSERIALPORT_UNIX_P_H
