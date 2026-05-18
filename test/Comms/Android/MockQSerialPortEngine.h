// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

#include "iqserialportengine_p.h"
#include "qandroidserialenginereceiver_p.h"

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QString>

QT_BEGIN_NAMESPACE
class QObject;
QT_END_NAMESPACE

// In-memory implementation of IQSerialPortEngine for host-side testing. Drives the
// receiver synchronously via simulate*() helpers; tests don't need to spin an event loop.
class MockQSerialPortEngine : public IQSerialPortEngine
{
public:
    MockQSerialPortEngine(QAndroidSerialEngineReceiver* sink, QObject* owner);
    ~MockQSerialPortEngine() override;

    // --- IQSerialPortEngine ---
    [[nodiscard]] bool open(const QString& portName,
                            const AndroidSerial::SerialParameters& params,
                            int flowControl, bool assertDtr) override;
    void close() override;
    bool isOpen() const override { return _state == State::Open; }
    State state() const override { return _state; }
    int deviceHandle() const override { return _deviceHandle; }

    [[nodiscard]] bool setParameters(const AndroidSerial::SerialParameters& p) override;
    [[nodiscard]] bool setDataTerminalReady(bool set) override;
    [[nodiscard]] bool setRequestToSend(bool set) override;
    [[nodiscard]] bool setFlowControl(int flowControl) override;
    int flowControl() const override { return _flowControl; }
    [[nodiscard]] bool setBreak(bool set) override;
    [[nodiscard]] bool purgeBuffers(bool input, bool output) override;

    quint32 controlLinesMask() const override { return _controlLinesMask; }

    int writeSync(QSpan<const char> data, int timeout) override;
    int writeAsync(QSpan<const char> data, int timeout) override;

    [[nodiscard]] bool startReadThread() override;
    [[nodiscard]] bool stopReadThread() override;
    bool readThreadRunning() const override { return _readThreadRunning; }

    void setReadBufferMaxSize(qint64 bytes) override { _readBufferMaxSize = bytes; }
    void clearReadStaging() override { _staged.clear(); }
    QByteArray waitForReadyRead(int msecs, qint64 ownerBufferSize) override;

    // --- Test driver helpers (synchronous receiver invocation) ---
    void simulateRead(const QByteArray& data);
    void simulateClose();
    void simulateException(AndroidSerial::JavaExceptionKind kind, const QString& msg);

    // --- Inspection / fault injection ---
    const QList<QByteArray>& syncWrites() const { return _syncWrites; }
    const QList<QByteArray>& asyncWrites() const { return _asyncWrites; }
    QString lastPortName() const { return _portName; }
    AndroidSerial::SerialParameters lastOpenParams() const { return _openParams; }
    int setParametersCallCount() const { return _setParametersCallCount; }
    bool dtrAsserted() const { return _dtrAsserted; }
    bool rtsAsserted() const { return _rtsAsserted; }
    bool breakSet() const { return _breakSet; }

    void setOpenShouldFail(bool b) { _openShouldFail = b; }
    void setNextWriteFailure(int returnValue) { _nextWriteReturn = returnValue; }
    void setControlLinesMask(quint32 mask) { _controlLinesMask = mask; }

private:
    QAndroidSerialEngineReceiver* _sink;
    [[maybe_unused]] QObject* _owner;  // Held for parity with production engine ctor; mock is single-threaded.
    State _state = State::Closed;

    int _deviceHandle = 1001;
    int _flowControl = 0;
    bool _dtrAsserted = false;
    bool _rtsAsserted = false;
    bool _breakSet = false;
    bool _readThreadRunning = false;
    bool _openShouldFail = false;
    int _nextWriteReturn = -2;  // -2 sentinel = use length; otherwise return this value once
    quint32 _controlLinesMask = 0;
    qint64 _readBufferMaxSize = 0;

    QString _portName;
    AndroidSerial::SerialParameters _openParams{};
    int _setParametersCallCount = 0;

    QByteArray _staged;
    QList<QByteArray> _syncWrites;
    QList<QByteArray> _asyncWrites;
};
