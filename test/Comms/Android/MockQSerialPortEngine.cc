// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "MockQSerialPortEngine.h"

#include <QtCore/QObject>

MockQSerialPortEngine::MockQSerialPortEngine(QAndroidSerialEngineReceiver* sink, QObject* owner)
    : _sink(sink), _owner(owner)
{
}

MockQSerialPortEngine::~MockQSerialPortEngine() = default;

bool MockQSerialPortEngine::open(const QString& portName,
                                 const AndroidSerial::SerialParameters& params,
                                 int flowControl, bool assertDtr)
{
    if (_openShouldFail) return false;

    _portName = portName;
    _openParams = params;
    _flowControl = flowControl;
    _dtrAsserted = assertDtr;
    _state = State::Open;
    return true;
}

void MockQSerialPortEngine::close()
{
    _state = State::Closed;
    _readThreadRunning = false;
}

bool MockQSerialPortEngine::setParameters(const AndroidSerial::SerialParameters& p)
{
    if (_state != State::Open) return false;
    _openParams = p;
    ++_setParametersCallCount;
    return true;
}

bool MockQSerialPortEngine::setDataTerminalReady(bool set)
{
    if (_state != State::Open) return false;
    _dtrAsserted = set;
    return true;
}

bool MockQSerialPortEngine::setRequestToSend(bool set)
{
    if (_state != State::Open) return false;
    _rtsAsserted = set;
    return true;
}

bool MockQSerialPortEngine::setFlowControl(int flowControl)
{
    if (_state != State::Open) return false;
    _flowControl = flowControl;
    return true;
}

bool MockQSerialPortEngine::setBreak(bool set)
{
    if (_state != State::Open) return false;
    _breakSet = set;
    return true;
}

bool MockQSerialPortEngine::purgeBuffers(bool input, bool /*output*/)
{
    if (_state != State::Open) return false;
    if (input) _staged.clear();
    return true;
}

int MockQSerialPortEngine::writeSync(QSpan<const char> data, int /*timeout*/)
{
    if (_state != State::Open) return -1;
    _syncWrites.append(QByteArray(data.data(), static_cast<qsizetype>(data.size())));
    if (_nextWriteReturn != -2) {
        const int rv = _nextWriteReturn;
        _nextWriteReturn = -2;
        return rv;
    }
    return static_cast<int>(data.size());
}

int MockQSerialPortEngine::writeAsync(QSpan<const char> data, int /*timeout*/)
{
    if (_state != State::Open) return -1;
    _asyncWrites.append(QByteArray(data.data(), static_cast<qsizetype>(data.size())));
    if (_nextWriteReturn != -2) {
        const int rv = _nextWriteReturn;
        _nextWriteReturn = -2;
        return rv;
    }
    return static_cast<int>(data.size());
}

bool MockQSerialPortEngine::startReadThread()
{
    if (_state != State::Open) return false;
    _readThreadRunning = true;
    return true;
}

bool MockQSerialPortEngine::stopReadThread()
{
    _readThreadRunning = false;
    return true;
}

QByteArray MockQSerialPortEngine::waitForReadyRead(int /*msecs*/, qint64 /*ownerBufferSize*/)
{
    QByteArray out;
    std::swap(out, _staged);
    return out;
}

// --- Test driver helpers ---

void MockQSerialPortEngine::simulateRead(const QByteArray& data)
{
    // Production engine marshals via QueuedConnection; the mock is synchronous so tests
    // can assert without an event loop. If a test needs queued semantics, replace this
    // body with QMetaObject::invokeMethod(... Qt::QueuedConnection).
    if (!_sink) return;
    QByteArray copy = data;
    _sink->dataReady(std::move(copy));
}

void MockQSerialPortEngine::simulateClose()
{
    if (!_sink) return;
    _sink->closeNotification();
}

void MockQSerialPortEngine::simulateException(AndroidSerial::JavaExceptionKind kind, const QString& msg)
{
    if (!_sink) return;
    _sink->exceptionNotification(kind, msg);
}
