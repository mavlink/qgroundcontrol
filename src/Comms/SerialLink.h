/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QString>

#ifdef Q_OS_ANDROID
#include "qserialport.h"
#else
#include <QtSerialPort/QSerialPort>
#endif

#include "LinkConfiguration.h"
#include "LinkInterface.h"

class QThread;
class QTimer;

Q_DECLARE_LOGGING_CATEGORY(SerialLinkLog)

/*===========================================================================*/

class SerialConfiguration : public LinkConfiguration
{
    Q_OBJECT
    Q_PROPERTY(qint32                   baud            READ baud            WRITE setBaud        NOTIFY baudChanged)
    Q_PROPERTY(QSerialPort::DataBits    dataBits        READ dataBits        WRITE setDataBits    NOTIFY dataBitsChanged)
    Q_PROPERTY(QSerialPort::FlowControl flowControl     READ flowControl     WRITE setFlowControl NOTIFY flowControlChanged)
    Q_PROPERTY(QSerialPort::StopBits    stopBits        READ stopBits        WRITE setStopBits    NOTIFY stopBitsChanged)
    Q_PROPERTY(QSerialPort::Parity      parity          READ parity          WRITE setParity      NOTIFY parityChanged)
    Q_PROPERTY(QString                  portName        READ portName        WRITE setPortName    NOTIFY portNameChanged)
    Q_PROPERTY(QString                  portDisplayName READ portDisplayName                      NOTIFY portDisplayNameChanged)
    Q_PROPERTY(bool                     usbDirect       READ usbDirect       WRITE setUsbDirect   NOTIFY usbDirectChanged)

public:
    explicit SerialConfiguration(const QString &name, QObject *parent = nullptr);
    explicit SerialConfiguration(const SerialConfiguration *copy, QObject *parent = nullptr);
    virtual ~SerialConfiguration();

    LinkType type() const override { return LinkConfiguration::TypeSerial; }
    void copyFrom(const LinkConfiguration *source) override;
    void loadSettings(QSettings &settings, const QString &root) override;
    void saveSettings(QSettings &settings, const QString &root) const override;
    QString settingsURL() const override { return QStringLiteral("SerialSettings.qml"); }
    QString settingsTitle() const override { return tr("Serial Link Settings"); }

    qint32 baud() const { return _baud; }
    void setBaud(qint32 baud) { if (baud != _baud) { _baud = baud; emit baudChanged(); } }

    QSerialPort::DataBits dataBits() const { return _dataBits; }
    void setDataBits(QSerialPort::DataBits databits) { if (databits != _dataBits) { _dataBits = databits; emit dataBitsChanged(); } }

    QSerialPort::FlowControl flowControl() const { return _flowControl; }
    void setFlowControl(QSerialPort::FlowControl flowControl) { if (flowControl != _flowControl) { _flowControl = flowControl; emit flowControlChanged(); } }

    QSerialPort::StopBits stopBits() const { return _stopBits; }
    void setStopBits(QSerialPort::StopBits stopBits) { if (stopBits != _stopBits) { _stopBits = stopBits; emit stopBitsChanged(); } }

    QSerialPort::Parity parity() const { return _parity; }
    void setParity(QSerialPort::Parity parity) { if (parity != _parity) { _parity = parity; emit parityChanged(); } }

    QString portName() const { return _portName; }
    void setPortName(const QString &name);

    QString portDisplayName() const { return _portDisplayName; }
    void setPortDisplayName(const QString &portDisplayName) { if (portDisplayName != _portDisplayName) { _portDisplayName = portDisplayName; emit portDisplayNameChanged(); } }

    bool usbDirect() const { return _usbDirect; }
    void setUsbDirect(bool usbDirect) { if (usbDirect != _usbDirect) { _usbDirect = usbDirect; emit usbDirectChanged(); } }

    static QStringList supportedBaudRates();
    static QString cleanPortDisplayName(const QString &name);

signals:
    void baudChanged();
    void dataBitsChanged();
    void flowControlChanged();
    void stopBitsChanged();
    void parityChanged();
    void portNameChanged();
    void portDisplayNameChanged();
    void usbDirectChanged();

private:
    qint32 _baud = QSerialPort::Baud57600;
    QSerialPort::DataBits _dataBits = QSerialPort::Data8;
    QSerialPort::FlowControl _flowControl = QSerialPort::NoFlowControl;
    QSerialPort::StopBits _stopBits = QSerialPort::OneStop;
    QSerialPort::Parity _parity = QSerialPort::NoParity;
    QString _portName;
    QString _portDisplayName;
    bool _usbDirect = false;
};

/*===========================================================================*/

class SerialWorker : public QObject
{
    Q_OBJECT

public:
    explicit SerialWorker(const SerialConfiguration *config, QObject *parent = nullptr);
    ~SerialWorker();

    bool isConnected() const;
    const QSerialPort *port() const { return _port; }

signals:
    void connected();
    void disconnected();
    void dataReceived(const QByteArray &data);
    void dataSent(const QByteArray &data);
    void errorOccurred(const QString &errorString);

public slots:
    void setupPort();
    void connectToPort();
    void disconnectFromPort();
    void writeData(const QByteArray &data);

private slots:
    void _onPortConnected();
    void _onPortDisconnected();
    void _onPortReadyRead();
    void _onPortBytesWritten(qint64 bytes) const;
    void _onPortErrorOccurred(QSerialPort::SerialPortError portError);
    void _checkPortAvailability();

private:
    const SerialConfiguration *_serialConfig = nullptr;
    QSerialPort *_port = nullptr;
    QTimer *_timer = nullptr;
    bool _errorEmitted = false;
};

/*===========================================================================*/

class SerialLink : public LinkInterface
{
    Q_OBJECT

public:
    explicit SerialLink(SharedLinkConfigurationPtr &config, QObject *parent = nullptr);
    virtual ~SerialLink();

    bool isConnected() const override;
    bool isSecureConnection() const override { return _serialConfig->usbDirect(); }

    const QSerialPort *port() const { return _worker->port(); }

public slots:
    void disconnect() override;

private slots:
    void _onConnected();
    void _onDisconnected();
    void _onDataReceived(const QByteArray &data);
    void _onDataSent(const QByteArray &data);
    void _onErrorOccurred(const QString &errorString);

private:
    bool _connect() override;
    void _writeBytes(const QByteArray &data) override;

    const SerialConfiguration *_serialConfig = nullptr;
    SerialWorker *_worker = nullptr;
    QThread *_workerThread = nullptr;
};
