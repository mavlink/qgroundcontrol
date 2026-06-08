#pragma once

#include "LinkConfiguration.h"
#include "LinkInterface.h"
#include "QGCSerialPort.h"
#include "WorkerThread.h"

#include <QtCore/QByteArray>
#include <QtCore/QString>

#include <atomic>
#include <memory>

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

    // QML-facing as int; stored as the QGC* enums so portConfig() needs no casts and setters reject out-of-range values.
    int dataBits() const { return static_cast<int>(_dataBits); }
    void setDataBits(int dataBits) { const auto v = static_cast<QGCDataBits>(dataBits); if (v != _dataBits) { _dataBits = v; emit dataBitsChanged(); } }

    int flowControl() const { return static_cast<int>(_flowControl); }
    void setFlowControl(int flowControl) { const auto v = static_cast<QGCFlowControl>(flowControl); if (v != _flowControl) { _flowControl = v; emit flowControlChanged(); } }

    int stopBits() const { return static_cast<int>(_stopBits); }
    void setStopBits(int stopBits) { const auto v = static_cast<QGCStopBits>(stopBits); if (v != _stopBits) { _stopBits = v; emit stopBitsChanged(); } }

    int parity() const { return static_cast<int>(_parity); }
    void setParity(int parity) { const auto v = static_cast<QGCParity>(parity); if (v != _parity) { _parity = v; emit parityChanged(); } }

    QString portName() const { return _portName; }
    void setPortName(const QString &name);

    // Derived on demand from the current enumeration; not persisted, so a renamed/reprobed device never shows a stale name.
    QString portDisplayName() const;

    bool usbDirect() const { return _usbDirect; }
    void setUsbDirect(bool usbDirect) { if (usbDirect != _usbDirect) { _usbDirect = usbDirect; emit usbDirectChanged(); } }

    bool dtrForceLow() const { return _dtrForceLow; }
    void setdtrForceLow(bool dtrForceLow) { if (dtrForceLow != _dtrForceLow) { _dtrForceLow = dtrForceLow; emit dtrForceLowChanged(); } }

    // Builds SerialPortConfig from QML-facing fields.
    SerialPortConfig portConfig() const;

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
    qint32         _baud        = 57600;
    QGCDataBits    _dataBits    = QGCDataBits::Data8;
    QGCFlowControl _flowControl = QGCFlowControl::None;
    QGCStopBits    _stopBits    = QGCStopBits::OneStop;
    QGCParity      _parity      = QGCParity::None;
    QString        _portName;
    bool           _usbDirect   = false;
    bool           _dtrForceLow = false;

    mutable QString _portDisplayName;
    mutable bool    _portDisplayNameValid = false;
};

class SerialWorker : public QObject
{
    Q_OBJECT

public:
    explicit SerialWorker(const SharedLinkConfigurationPtr &config, QObject *parent = nullptr);
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
    void _onPortBytesWritten();
    void _onPortErrorOccurred(QGCSerialPortError portError);

    // At most once per connect/disconnect cycle — prevents write-cap floods from queueing UI popups.
    void _emitErrorOnce(const QString &errorString);

private:
    // Drains _pendingWrite into the port without blocking; resumes from _onPortBytesWritten as the port drains.
    void _flushPendingWrites();

    // Holds a ref to the shared config so a detached (wedged) worker that outlives its SerialLink
    // never dangles _serialConfig, which aliases into it.
    SharedLinkConfigurationPtr _configHolder;
    const SerialConfiguration *_serialConfig = nullptr;
    QGCSerialPort *_port = nullptr;
    bool _errorEmitted = false;
    // Bytes accepted from writeData() but not yet handed to the port (port write buffer was full).
    QByteArray _pendingWrite;
    // Mirrors _port->isOpen() for cross-thread reads; races on QIODevicePrivate::openMode otherwise.
    std::atomic<bool> _connected{false};
};

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
    std::unique_ptr<WorkerThread> _workerThread;
    // True between an emitted connected() and its disconnected(); ~SerialLink uses it to flush a pending
    // disconnected() on the USB-unplug-then-quit race, and _onDisconnected() to emit exactly once.
    std::atomic<bool> _emittedConnected{false};
};
