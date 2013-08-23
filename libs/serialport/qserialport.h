/****************************************************************************
**
** Copyright (C) 2012 Denis Shienkov <denis.shienkov@gmail.com>
** Copyright (C) 2013 Laszlo Papp <lpapp@kde.org>
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

#ifndef QSERIALPORT_H
#define QSERIALPORT_H

#include <QtCore/qiodevice.h>

#include <qserialportglobal.h>
//#include <QtSerialPort/qserialportglobal.h>
QT_BEGIN_NAMESPACE

class QSerialPortInfo;
class QSerialPortPrivate;

class Q_SERIALPORT_EXPORT QSerialPort : public QIODevice
{
    Q_OBJECT

    Q_PROPERTY(qint32 baudRate READ baudRate WRITE setBaudRate NOTIFY baudRateChanged)
    Q_PROPERTY(DataBits dataBits READ dataBits WRITE setDataBits NOTIFY dataBitsChanged)
    Q_PROPERTY(Parity parity READ parity WRITE setParity NOTIFY parityChanged)
    Q_PROPERTY(StopBits stopBits READ stopBits WRITE setStopBits NOTIFY stopBitsChanged)
    Q_PROPERTY(FlowControl flowControl READ flowControl WRITE setFlowControl NOTIFY flowControlChanged)
    Q_PROPERTY(DataErrorPolicy dataErrorPolicy READ dataErrorPolicy WRITE setDataErrorPolicy NOTIFY dataErrorPolicyChanged)
    Q_PROPERTY(bool dataTerminalReady READ isDataTerminalReady WRITE setDataTerminalReady NOTIFY dataTerminalReadyChanged)
    Q_PROPERTY(bool requestToSend READ isRequestToSend WRITE setRequestToSend NOTIFY requestToSendChanged)
    Q_PROPERTY(SerialPortError error READ error RESET clearError NOTIFY error)
    Q_PROPERTY(bool settingsRestoredOnClose READ settingsRestoredOnClose WRITE setSettingsRestoredOnClose NOTIFY settingsRestoredOnCloseChanged)

    Q_ENUMS( Directions Rate DataBits Parity StopBits FlowControl PinoutSignals DataErrorPolicy SerialPortError )

public:

    enum Direction  {
        Input = 1,
        Output = 2,
        AllDirections = Input | Output
    };
    Q_DECLARE_FLAGS(Directions, Direction)

    enum BaudRate {
        Baud1200 = 1200,
        Baud2400 = 2400,
        Baud4800 = 4800,
        Baud9600 = 9600,
        Baud19200 = 19200,
        Baud38400 = 38400,
        Baud57600 = 57600,
        Baud115200 = 115200,
        UnknownBaud = -1
    };

    enum DataBits {
        Data5 = 5,
        Data6 = 6,
        Data7 = 7,
        Data8 = 8,
        UnknownDataBits = -1
    };

    enum Parity {
        NoParity = 0,
        EvenParity = 2,
        OddParity = 3,
        SpaceParity = 4,
        MarkParity = 5,
        UnknownParity = -1
    };

    enum StopBits {
        OneStop = 1,
        OneAndHalfStop = 3,
        TwoStop = 2,
        UnknownStopBits = -1
    };

    enum FlowControl {
        NoFlowControl,
        HardwareControl,
        SoftwareControl,
        UnknownFlowControl = -1
    };

    enum PinoutSignal {
        NoSignal = 0x00,
        TransmittedDataSignal = 0x01,
        ReceivedDataSignal = 0x02,
        DataTerminalReadySignal = 0x04,
        DataCarrierDetectSignal = 0x08,
        DataSetReadySignal = 0x10,
        RingIndicatorSignal = 0x20,
        RequestToSendSignal = 0x40,
        ClearToSendSignal = 0x80,
        SecondaryTransmittedDataSignal = 0x100,
        SecondaryReceivedDataSignal = 0x200
    };
    Q_DECLARE_FLAGS(PinoutSignals, PinoutSignal)

    enum DataErrorPolicy {
        SkipPolicy,
        PassZeroPolicy,
        IgnorePolicy,
        StopReceivingPolicy,
        UnknownPolicy = -1
    };

    enum SerialPortError {
        NoError,
        DeviceNotFoundError,
        PermissionError,
        OpenError,
        ParityError,
        FramingError,
        BreakConditionError,
        WriteError,
        ReadError,
        ResourceError,
        UnsupportedOperationError,
        UnknownError
    };

    explicit QSerialPort(QObject *parent = 0);
    explicit QSerialPort(const QString &name, QObject *parent = 0);
    explicit QSerialPort(const QSerialPortInfo &info, QObject *parent = 0);
    virtual ~QSerialPort();

    void setPortName(const QString &name);
    QString portName() const;

    void setPort(const QSerialPortInfo &info);

    bool open(OpenMode mode) Q_DECL_OVERRIDE;
    void close() Q_DECL_OVERRIDE;

    void setSettingsRestoredOnClose(bool restore);
    bool settingsRestoredOnClose() const;

    bool setBaudRate(qint32 baudRate, Directions dir = AllDirections);
    qint32 baudRate(Directions dir = AllDirections) const;

    bool setDataBits(DataBits dataBits);
    DataBits dataBits() const;

    bool setParity(Parity parity);
    Parity parity() const;

    bool setStopBits(StopBits stopBits);
    StopBits stopBits() const;

    bool setFlowControl(FlowControl flow);
    FlowControl flowControl() const;

    bool setDataTerminalReady(bool set);
    bool isDataTerminalReady();

    bool setRequestToSend(bool set);
    bool isRequestToSend();

    PinoutSignals pinoutSignals();

    bool flush();
    bool clear(Directions dir = AllDirections);
    bool atEnd() const Q_DECL_OVERRIDE;

    bool setDataErrorPolicy(DataErrorPolicy policy = IgnorePolicy);
    DataErrorPolicy dataErrorPolicy() const;

    SerialPortError error() const;
    void clearError();

    qint64 readBufferSize() const;
    void setReadBufferSize(qint64 size);

    bool isSequential() const Q_DECL_OVERRIDE;

    qint64 bytesAvailable() const Q_DECL_OVERRIDE;
    qint64 bytesToWrite() const Q_DECL_OVERRIDE;
    bool canReadLine() const Q_DECL_OVERRIDE;

    bool waitForReadyRead(int msecs) Q_DECL_OVERRIDE;
    bool waitForBytesWritten(int msecs) Q_DECL_OVERRIDE;

    bool sendBreak(int duration = 0);
    bool setBreakEnabled(bool set = true);

Q_SIGNALS:
    void baudRateChanged(qint32 baudRate, QSerialPort::Directions dir);
    void dataBitsChanged(QSerialPort::DataBits dataBits);
    void parityChanged(QSerialPort::Parity parity);
    void stopBitsChanged(QSerialPort::StopBits stopBits);
    void flowControlChanged(QSerialPort::FlowControl flow);
    void dataErrorPolicyChanged(QSerialPort::DataErrorPolicy policy);
    void dataTerminalReadyChanged(bool set);
    void requestToSendChanged(bool set);
    void error(QSerialPort::SerialPortError serialPortError);
    void settingsRestoredOnCloseChanged(bool restore);

protected:
    qint64 readData(char *data, qint64 maxSize) Q_DECL_OVERRIDE;
    qint64 readLineData(char *data, qint64 maxSize) Q_DECL_OVERRIDE;
    qint64 writeData(const char *data, qint64 maxSize) Q_DECL_OVERRIDE;

private:
    void setError(QSerialPort::SerialPortError error, const QString &errorString = QString());

    QSerialPortPrivate * const d_ptr;

    Q_DECLARE_PRIVATE(QSerialPort)
    Q_DISABLE_COPY(QSerialPort)
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QSerialPort::Directions)
Q_DECLARE_OPERATORS_FOR_FLAGS(QSerialPort::PinoutSignals)

QT_END_NAMESPACE

#endif // QSERIALPORT_H
