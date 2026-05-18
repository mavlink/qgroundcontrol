#pragma once

#include "LinkConfiguration.h"
#include "LinkInterface.h"
#include "QGCSerialPortAdapter.h"

#include <QtCore/QString>

#include <atomic>

class QThread;
class QTimer;

/*===========================================================================*/

class SerialConfiguration : public LinkConfiguration
{
    Q_OBJECT
    Q_PROPERTY(qint32  baud            READ baud            WRITE setBaud        NOTIFY baudChanged)
    Q_PROPERTY(int     dataBits        READ dataBits        WRITE setDataBits    NOTIFY dataBitsChanged)
    Q_PROPERTY(int     flowControl     READ flowControl     WRITE setFlowControl NOTIFY flowControlChanged)
    Q_PROPERTY(int     stopBits        READ stopBits        WRITE setStopBits    NOTIFY stopBitsChanged)
    Q_PROPERTY(int     parity          READ parity          WRITE setParity      NOTIFY parityChanged)
    Q_PROPERTY(QString portName        READ portName        WRITE setPortName    NOTIFY portNameChanged)
    Q_PROPERTY(QString portDisplayName READ portDisplayName                      NOTIFY portDisplayNameChanged)
    Q_PROPERTY(bool    usbDirect       READ usbDirect       WRITE setUsbDirect   NOTIFY usbDirectChanged)
    Q_PROPERTY(bool    dtrForceLow     READ dtrForceLow     WRITE setdtrForceLow NOTIFY dtrForceLowChanged)

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

    // The four enum-style properties below store integer values matching the
    // QSerialPort::DataBits / FlowControl / StopBits / Parity enums (the canonical
    // wire-format integers). Translation happens inside QGCSerialPortAdapter.
    int dataBits() const { return _dataBits; }
    void setDataBits(int databits) { if (databits != _dataBits) { _dataBits = databits; emit dataBitsChanged(); } }

    int flowControl() const { return _flowControl; }
    void setFlowControl(int flowControl) { if (flowControl != _flowControl) { _flowControl = flowControl; emit flowControlChanged(); } }

    int stopBits() const { return _stopBits; }
    void setStopBits(int stopBits) { if (stopBits != _stopBits) { _stopBits = stopBits; emit stopBitsChanged(); } }

    int parity() const { return _parity; }
    void setParity(int parity) { if (parity != _parity) { _parity = parity; emit parityChanged(); } }

    QString portName() const { return _portName; }
    void setPortName(const QString &name);

    QString portDisplayName() const { return _portDisplayName; }
    void setPortDisplayName(const QString &portDisplayName) { if (portDisplayName != _portDisplayName) { _portDisplayName = portDisplayName; emit portDisplayNameChanged(); } }

    bool usbDirect() const { return _usbDirect; }
    void setUsbDirect(bool usbDirect) { if (usbDirect != _usbDirect) { _usbDirect = usbDirect; emit usbDirectChanged(); } }

    bool dtrForceLow() const { return _dtrForceLow; }
    void setdtrForceLow(bool dtrForceLow) { if (dtrForceLow != _dtrForceLow) { _dtrForceLow = dtrForceLow; emit dtrForceLowChanged(); } }

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
    void dtrForceLowChanged();

private:
    qint32  _baud = 57600;
    int     _dataBits    = 8;       // QSerialPort::Data8
    int     _flowControl = 0;       // QSerialPort::NoFlowControl
    int     _stopBits    = 1;       // QSerialPort::OneStop
    int     _parity      = 0;       // QSerialPort::NoParity
    QString _portName;
    QString _portDisplayName;
    bool    _usbDirect   = false;
    bool    _dtrForceLow = false;
};

/*===========================================================================*/

class SerialWorker : public QObject
{
    Q_OBJECT

public:
    explicit SerialWorker(const SerialConfiguration *config, QObject *parent = nullptr);
    ~SerialWorker();

    bool isConnected() const;

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
    void _onPortErrorOccurred(QGCSerialPortAdapter::Error portError);
    void _checkPortAvailability();

    /** Emit {@link #errorOccurred} at most once per connect/disconnect cycle —
     *  prevents writeData() failure floods (e.g. write-buffer cap firing under
     *  burst load) from queueing thousands of UI popups. */
    void _emitErrorOnce(const QString &errorString);

private:
    const SerialConfiguration *_serialConfig = nullptr;
    QGCSerialPortAdapter *_port = nullptr;
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
    std::atomic<bool> _disconnectedEmitted{false};
};
