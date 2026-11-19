#pragma once

#include <atomic>
#include <memory>
#include <optional>

#include <QtCore/QByteArray>
#include <QtCore/QIODevice>
#include <QtCore/QLoggingCategory>
#include <QtCore/QMutex>
#include <QtCore/QString>
#include <QtCore/QWaitCondition>

Q_DECLARE_LOGGING_CATEGORY(AndroidSerialPortLog)

class AndroidSerialBackend;
class QDeadlineTimer;
class QSerialPortInfo;

/// The serial port of QGC on Android. Qt Serial Port's own opens device nodes only, and Android lets an app reach
/// a USB serial adapter through its USB API alone. This class has the name and the interface of Qt Serial Port's,
/// so that code which uses a serial port is the same on every platform and includes <QtSerialPort/QSerialPort>.
/// Of that interface it leaves out the properties and their bindables, and a baud rate for one direction only.
///
/// The port's name decides what it opens (AndroidSerialBackend): a USB serial adapter, which Java drives, or a
/// serial device node. Input arrives on other threads and is announced on the port's own, which is the only one
/// that may call the port.
class QSerialPort : public QIODevice
{
    Q_OBJECT

public:
    explicit QSerialPort(QObject* parent = nullptr);
    explicit QSerialPort(const QString& name, QObject* parent = nullptr);
    explicit QSerialPort(const QSerialPortInfo& info, QObject* parent = nullptr);
    ~QSerialPort() override;

    enum Direction
    {
        Input = 1,
        Output = 2,
        AllDirections = Input | Output
    };
    Q_FLAG(Direction)
    Q_DECLARE_FLAGS(Directions, Direction)

    enum BaudRate
    {
        Baud1200 = 1200,
        Baud2400 = 2400,
        Baud4800 = 4800,
        Baud9600 = 9600,
        Baud19200 = 19200,
        Baud38400 = 38400,
        Baud57600 = 57600,
        Baud115200 = 115200
    };
    Q_ENUM(BaudRate)

    enum DataBits
    {
        Data5 = 5,
        Data6 = 6,
        Data7 = 7,
        Data8 = 8
    };
    Q_ENUM(DataBits)

    enum Parity
    {
        NoParity = 0,
        EvenParity = 2,
        OddParity = 3,
        SpaceParity = 4,
        MarkParity = 5
    };
    Q_ENUM(Parity)

    enum StopBits
    {
        OneStop = 1,
        OneAndHalfStop = 3,
        TwoStop = 2
    };
    Q_ENUM(StopBits)

    enum FlowControl
    {
        NoFlowControl,
        HardwareControl,
        SoftwareControl
    };
    Q_ENUM(FlowControl)

    enum PinoutSignal
    {
        NoSignal = 0x00,
        DataTerminalReadySignal = 0x04,
        DataCarrierDetectSignal = 0x08,
        DataSetReadySignal = 0x10,
        RingIndicatorSignal = 0x20,
        RequestToSendSignal = 0x40,
        ClearToSendSignal = 0x80,
        SecondaryTransmittedDataSignal = 0x100,
        SecondaryReceivedDataSignal = 0x200
    };
    Q_FLAG(PinoutSignal)
    Q_DECLARE_FLAGS(PinoutSignals, PinoutSignal)

    enum SerialPortError
    {
        NoError,
        DeviceNotFoundError,
        PermissionError,
        OpenError,
        WriteError,
        ReadError,
        ResourceError,
        UnsupportedOperationError,
        UnknownError,
        TimeoutError,
        NotOpenError
    };
    Q_ENUM(SerialPortError)

    void setPortName(const QString& name);
    QString portName() const;
    void setPort(const QSerialPortInfo& info);

    bool open(OpenMode mode) override;
    void close() override;

    /// A closed port keeps the settings it is given and applies them when it opens. An open port keeps a setting
    /// only if the device took it.
    bool setBaudRate(qint32 baudRate, Directions directions = AllDirections);
    qint32 baudRate(Directions directions = AllDirections) const;
    bool setDataBits(DataBits dataBits);

    DataBits dataBits() const { return _dataBits; }

    bool setParity(Parity parity);

    Parity parity() const { return _parity; }

    bool setStopBits(StopBits stopBits);

    StopBits stopBits() const { return _stopBits; }

    bool setFlowControl(FlowControl flowControl);

    FlowControl flowControl() const { return _flowControl; }

    /// Does not ask the device how the line is: it signals a change from what the port itself last set since it
    /// opened, and the first set counts as one. Likewise setRequestToSend().
    bool setDataTerminalReady(bool set);
    bool isDataTerminalReady();
    bool setRequestToSend(bool set);
    bool isRequestToSend();
    PinoutSignals pinoutSignals();
    bool setBreakEnabled(bool set = true);

    bool isBreakEnabled() const { return _breakEnabled; }

    /// Hands buffered output to the device, as far as it takes it now, and returns whether it took any.
    bool flush();
    bool clear(Directions directions = AllDirections);

    SerialPortError error() const { return _error; }

    void clearError();

    /// How much input the port lets its owner see at a time: bytesAvailable() does not exceed it. What arrives
    /// beyond it waits, hidden, for a read to make room, and when that alone reaches the limit the device is
    /// read no further, so the port holds up to twice the limit. 0 or less is no limit.
    void setReadBufferSize(qint64 size);

    qint64 readBufferSize() const { return _readBufferMaxSize; }

    /// The file descriptor of the open device, or -1.
    int handle() const;

    bool isSequential() const override;
    qint64 bytesAvailable() const override;
    qint64 bytesToWrite() const override;
    bool canReadLine() const override;
    /// Returns true only for input that was not announced before.
    bool waitForReadyRead(int msecs = 30000) override;
    bool waitForBytesWritten(int msecs = 30000) override;

signals:
    void baudRateChanged(qint32 baudRate, QSerialPort::Directions directions);
    void dataBitsChanged(QSerialPort::DataBits dataBits);
    void parityChanged(QSerialPort::Parity parity);
    void stopBitsChanged(QSerialPort::StopBits stopBits);
    void flowControlChanged(QSerialPort::FlowControl flowControl);
    void dataTerminalReadyChanged(bool set);
    void requestToSendChanged(bool set);
    /// Emitted whenever the error is set, also to NoError.
    void errorOccurred(QSerialPort::SerialPortError error);
    void breakEnabledChanged(bool set);

protected:
    qint64 readData(char* data, qint64 maxSize) override;
    qint64 writeData(const char* data, qint64 maxSize) override;

private:
    // The only way a backend reaches into the port.
    friend class AndroidSerialBackendHost;

    void _setError(SerialPortError error, const QString& text = QString());

    /// The backend for a port of this name: USB if Java names its ports like it, otherwise POSIX.
    AndroidSerialBackend* _backendFor(const QString& location);
    /// The backend of the open port. If the port is not open, sets NotOpenError and returns null.
    AndroidSerialBackend* _openBackend();
    /// Opens the device and sets it up as the port is set. Returns false after an error, perhaps with the
    /// device still open.
    bool _openDevice(OpenMode mode);
    void _closeDevice();
    /// Forgets the input and output that the port holds, and what was reported of them.
    void _discardBuffers();
    /// Has the device discard what it holds, and then read on if the port has room. The backend is passed
    /// because the port's owner may close the port over an error of it.
    bool _clearDevice(AndroidSerialBackend* backend, Directions directions);

    /// Makes sure that the backend reads if the port has room for input, and that input which waits for room is
    /// admitted once there is some. Returns false after an error.
    bool _readOn();
    /// Has the backend read on if it was told that the port had no room and the port has room again. Returns
    /// false after an error.
    bool _readOnIfRoom();
    qsizetype _hiddenSizeLocked() const;
    bool _hasRoomForInputLocked() const;
    /// Whether hidden input can be shown to the owner now. On the port's thread only, like _admitInputLocked():
    /// both ask bytesAvailable().
    bool _canAdmitInputLocked() const;
    /// Lets the owner see input that has arrived, as far as the read buffer size allows, and returns whether
    /// there was any to let it see. The caller announces it.
    bool _admitInputLocked();
    bool _admitArrivedInput();
    void _scheduleAdmission();
    void _admitQueuedInput();
    /// Announces admitted input, once the backend reads on if that made room.
    void _announceAdmittedInput();

    void _scheduleWrite();
    void _pumpQueuedWrites();
    /// Accounts for what the backend has written, announces it, and hands the backend more output unless it
    /// still has some. Never waits. Returns false after an error.
    bool _pumpWrites();
    /// Writes until no output is left, or the deadline has passed and what the device took by then is accounted
    /// for. Returns false after an error, which a passed deadline is not.
    bool _writeUntil(const QDeadlineTimer& deadline);
    void _flushBeforeClose();

    // For the backend, as AndroidSerialBackendHost describes them.
    bool _newDataArrived(const char* bytes, int length);
    bool _hasRoomForInput();
    bool _inputArrived();
    void _waitForArrivedInput(const QDeadlineTimer& deadline);
    void _writeCompleted(qint64 bytes);
    void _waitForWriteProgress(const QDeadlineTimer& deadline);
    void _interruptWait();

    QString _systemLocation;
    qint32 _baudRate = Baud9600;
    DataBits _dataBits = Data8;
    Parity _parity = NoParity;
    StopBits _stopBits = OneStop;
    FlowControl _flowControl = NoFlowControl;
    bool _breakEnabled = false;
    // What the port last set each line to since it opened, by which it tells a change. Asking the device instead
    // would read all its control lines, for which a CDC-ACM device is polled from then until the port closes.
    std::optional<bool> _dataTerminalReady;
    std::optional<bool> _requestToSend;
    SerialPortError _error = NoError;

    // Each is made when first needed and then kept. A backend reports an error from inside its own function,
    // and the port's owner may close the port in response, so a close must not destroy it.
    std::unique_ptr<AndroidSerialBackend> _usbBackend;
    std::unique_ptr<AndroidSerialBackend> _posixBackend;
    // The backend of the open port, or null.
    AndroidSerialBackend* _backend = nullptr;

    // Guards what other threads bring, and announces it: input that arrived and reports of output written.
    mutable QMutex _arrivalMutex;
    QWaitCondition _arrivalCondition;
    // All input that has arrived and that the owner has not read.
    QByteArray _input;
    // How much of it, from the front, the owner may read. The rest is hidden until the read buffer size allows
    // it. Only the port's thread changes it.
    qsizetype _admitted = 0;
    qint64 _readBufferMaxSize = 0;
    // Whether the backend was last told that the port had no room, and has not been asked to read since.
    bool _readingHeldBack = false;
    // What _writeCompleted() has been told since _pumpWrites() last looked, and whether it was called at all.
    qint64 _completedWriteBytes = 0;
    bool _writeProgress = false;
    // Whether _interruptWait() has been called since a wait last ended.
    bool _waitInterrupted = false;

    std::atomic<bool> _admissionPending{false};
    bool _emittingReadyRead = false;

    // Output that the backend has not taken yet.
    QByteArray _output;
    std::atomic<bool> _writePending{false};
    // Output that the backend has taken and not yet reported written. bytesToWrite() counts it.
    qint64 _bytesInFlight = 0;
    // Output that clear() gave up while the backend had it. The backend still reports all of it, sent or not,
    // which the port does not announce.
    qint64 _discardedInFlight = 0;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QSerialPort::Directions)
Q_DECLARE_OPERATORS_FOR_FLAGS(QSerialPort::PinoutSignals)
