// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an implementation
// detail of QGC's Android QSerialPort backend.
//

#include <QtCore/QByteArray>
#include <QtCore/QSpan>
#include <QtCore/QString>
#include <QtCore/QtTypes>

#include "AndroidSerial.h"

// Abstract engine surface consumed by QSerialPortPrivate. Decouples the Qt-side QIODevice
// implementation from the JNI-backed concrete engine (QAndroidSerialEngine), so a host-side
// mock (MockQSerialPortEngine) can implement the same contract without JNI dependencies.
//
// Receiver callbacks (data/close/exception) live on QAndroidSerialEngineReceiver — kept
// separate so the engine can call back into QSerialPortPrivate without circular includes.
class IQSerialPortEngine
{
public:
    enum class State { Closed, Opening, Open, Closing };

    virtual ~IQSerialPortEngine() = default;

    // --- Lifecycle ---
    [[nodiscard]] virtual bool open(const QString& portName,
                                    const AndroidSerial::SerialParameters& params,
                                    int flowControl, bool assertDtr) = 0;
    virtual void close() = 0;
    virtual bool isOpen() const = 0;
    virtual State state() const = 0;
    virtual int deviceHandle() const = 0;

    // --- Configuration ---
    [[nodiscard]] virtual bool setParameters(const AndroidSerial::SerialParameters& p) = 0;
    [[nodiscard]] virtual bool setDataTerminalReady(bool set) = 0;
    [[nodiscard]] virtual bool setRequestToSend(bool set) = 0;
    [[nodiscard]] virtual bool setFlowControl(int flowControl) = 0;
    virtual int flowControl() const = 0;
    [[nodiscard]] virtual bool setBreak(bool set) = 0;
    [[nodiscard]] virtual bool purgeBuffers(bool input, bool output) = 0;

    // Bitmask of QSerialPort::PinoutSignal values. Returned as quint32 so this interface
    // header doesn't have to drag in the platform-specific QSerialPort header — the
    // QSerialPortPrivate consumer casts back to QSerialPort::PinoutSignals at the boundary.
    virtual quint32 controlLinesMask() const = 0;

    // --- I/O ---
    virtual int writeSync(QSpan<const char> data, int timeout) = 0;
    virtual int writeAsync(QSpan<const char> data, int timeout) = 0;

    // --- Read thread ---
    [[nodiscard]] virtual bool startReadThread() = 0;
    [[nodiscard]] virtual bool stopReadThread() = 0;
    virtual bool readThreadRunning() const = 0;

    // --- Read staging ---
    virtual void setReadBufferMaxSize(qint64 bytes) = 0;
    virtual void clearReadStaging() = 0;
    virtual QByteArray waitForReadyRead(int msecs, qint64 ownerBufferSize) = 0;
};
