#ifndef QSERIALPORT_ANDROID_P_H
#define QSERIALPORT_ANDROID_P_H
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

#include "qserialport_p.h"

#include <QtCore/qscopedpointer.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qthread.h>

#include "QGCLoggingCategory.h"

Q_DECLARE_LOGGING_CATEGORY(AndroidSerialPortLog)

QT_BEGIN_NAMESPACE

class QSerialPortPrivate : public QSerialPortPrivateData
{
    Q_DECLARE_PUBLIC(QSerialPort)

public:
    QSerialPortPrivate(QSerialPort *q);

    bool open(QIODevice::OpenMode mode);
    void close();

    QSerialPort::PinoutSignals pinoutSignals();

    bool setDataTerminalReady(bool set);
    bool setRequestToSend(bool set);

    bool flush();
    bool clear(QSerialPort::Directions directions);

    bool sendBreak(int duration);
    bool setBreakEnabled(bool set);

    void startWriting();

    bool waitForReadyRead(int msecs);
    bool waitForBytesWritten(int msecs);

    bool setBaudRate();
    bool setBaudRate(qint32 baudRate, QSerialPort::Directions directions);
    bool setDataBits(QSerialPort::DataBits dataBits);
    bool setParity(QSerialPort::Parity parity);
    bool setStopBits(QSerialPort::StopBits stopBits);
    bool setFlowControl(QSerialPort::FlowControl flowControl);
    bool setDataErrorPolicy(QSerialPort::DataErrorPolicy policy);

    bool startAsyncWrite();
    bool completeAsyncWrite();
    void newDataArrived(char *bytesA, int lengthA);
    void exceptionArrived(QString strA);

    void stopReadThread();
    void startReadThread();
    qint64 bytesToWrite() const;
    qint64 writeData(const char *data, qint64 maxSize);

    static qint32 baudRateFromSetting(qint32 setting);
    static qint32 settingFromBaudRate(qint32 baudRate);

    static QList<qint32> standardBaudRates();

    int descriptor;
    bool isCustomBaudRateSupported;

    bool emittedBytesWritten;

    qint64 pendingBytesWritten;

    static void setNativeMethods(void);

private:
    QIODevice::OpenMode rwMode;
    int deviceId;
    int jniDataBits;
    int jniStopBits;
    int jniParity;
    qint64 internalWriteTimeoutMsec;
    bool isReadStopped;

    bool setParameters(int baudRateA, int dataBitsA, int stopBitsA, int parityA);

    QSerialPort::SerialPortError decodeSystemError() const;

    qint64 writeToPort(const char *data, qint64 maxSize);

    bool writeDataOneShot();
};

QT_END_NAMESPACE

#endif // QSERIALPORT_ANDROID_P_H

