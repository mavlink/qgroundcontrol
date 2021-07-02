/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MultiSignalSpyV2.h"
#include <QEventLoop>
#include <QCoreApplication>
#include <QDebug>
#include <QTest>

MultiSignalSpyV2::MultiSignalSpyV2(QObject* parent)
    : QObject(parent)
{

}

MultiSignalSpyV2::~MultiSignalSpyV2()
{
    for (QSignalSpy* spy: _rgSpys) {
        delete spy;
    }
}

/// Initializes the class. Must be called once before use.
/// @param signalEmitter[in] QObject which the signals are emitted from
///     @return true if success, false for failure
bool MultiSignalSpyV2::init(QObject* signalEmitter)
{
    bool error = false;

    if (!signalEmitter) {
        qWarning() << "No signalEmitter specified";
        return false;
    }
    
    _signalEmitter = signalEmitter;

    const QMetaObject* metaObject = signalEmitter->metaObject();
    for (int i = 0; i < metaObject->methodCount(); i++) {
        const QMetaMethod method = metaObject->method(i);
        if (method.methodType() == QMetaMethod::Signal) {
            QString signalName = method.name();

#if 0
            // FIXME: CompexMissionItem has duplicate signals which still need to be fixed
            if (signalName != "destroyed" && _rgSignalNames.contains(signalName)) {
                qWarning() << "Duplicate signal name" << signalName;
                error = true;
                break;
            }
#endif
            _rgSignalNames.append(signalName);

            QSignalSpy* spy = new QSignalSpy(_signalEmitter, QStringLiteral("2%1").arg(method.methodSignature().data()).toLocal8Bit().data());
            if (spy->isValid()) {
                _rgSpys.append(spy);
            } else {
                qWarning() << "Invalid signal:" << signalName;
                error = true;
                break;
            }
        }
    }

    if (error) {
        for (QSignalSpy* spy: _rgSpys) {
            delete spy;
        }
        _rgSignalNames.clear();
        _rgSpys.clear();
        return false;
    }

    return true;
}

bool MultiSignalSpyV2::_checkSignalByMaskWorker(quint64 mask, bool multipleSignalsAllowed)
{
    for (int i=0; i<_rgSignalNames.count(); i++) {
        if ((1ll << i) & mask) {
            QSignalSpy* spy = _rgSpys[i];
            Q_ASSERT(spy != nullptr);
            
            if ((multipleSignalsAllowed && spy->count() ==  0) || (!multipleSignalsAllowed && spy->count() != 1)) {
                qWarning() << "Failed index:" << i;
                _printSignalState(mask);
                return false;
            }
        }
    }
    
    return true;
}

bool MultiSignalSpyV2::_checkOnlySignalByMaskWorker(quint64 mask, bool multipleSignalsAllowed)
{
    for (int i=0; i<_rgSignalNames.count(); i++) {
        QSignalSpy* spy = _rgSpys[i];
        Q_ASSERT(spy != nullptr);

        if ((1ll << i) & mask) {
            if ((multipleSignalsAllowed && spy->count() ==  0) || (!multipleSignalsAllowed && spy->count() != 1)) {
                _printSignalState(mask);
                return false;
            }
        } else {
            if (spy->count() != 0) {
                _printSignalState(mask);
                return false;
            }
        }
    }
    
    return true;
}

bool MultiSignalSpyV2::checkSignalByMask(quint64 mask)
{
    return _checkSignalByMaskWorker(mask, false /* multipleSignalsAllowed */);
}

bool MultiSignalSpyV2::checkOnlySignalByMask(quint64 mask)
{
    return _checkOnlySignalByMaskWorker(mask, false /* multipleSignalsAllowed */);
}

bool MultiSignalSpyV2::checkSignalsByMask(quint64 mask)
{
    return _checkSignalByMaskWorker(mask, true /* multipleSignalsAllowed */);
}

bool MultiSignalSpyV2::checkOnlySignalsByMask(quint64 mask)
{
    return _checkOnlySignalByMaskWorker(mask, true /* multipleSignalsAllowed */);
}

/// @return true if signal count = 0 for specified signals
bool MultiSignalSpyV2::checkNoSignalByMask(quint64 mask)
{
    for (int i=0; i<_rgSignalNames.count(); i++) {
        if ((1ll << i) & mask) {
            QSignalSpy* spy = _rgSpys[i];
            Q_ASSERT(spy != nullptr);

            if (spy->count() != 0) {
                _printSignalState(mask);
                return false;
            }
        }
    }
    
    return true;
}

/// @return true if signal count = 0 on all signals
bool MultiSignalSpyV2::checkNoSignals(void)
{
    return checkNoSignalByMask(~0);
}

/// @return QSignalSpy for the specified signal
QSignalSpy* MultiSignalSpyV2::getSpy(const char* signalName)
{
    for (int i=0; i<_rgSignalNames.count(); i++) {
        if (_rgSignalNames[i] == signalName) {
            return _rgSpys[i];
        }
    }

    qWarning() << "MultiSignalSpyV2::getSpy no such signal" << signalName;
    return nullptr;
}

/// Sets the signal count to 0 for the specified signal
void MultiSignalSpyV2::clearSignal(const char* signalName)
{
    getSpy(signalName)->clear();
}

/// Sets the signal count to 0 for all specified signals
void MultiSignalSpyV2::clearSignalsByMask(quint64 mask)
{
    for (int i=0; i<_rgSignalNames.count(); i++) {
        if ((1ll << i) & mask) {
            QSignalSpy* spy = _rgSpys[i];
            Q_ASSERT(spy != nullptr);

            spy->clear();
        }
    }
}

/// Sets the signal count to 0 for all signals
void MultiSignalSpyV2::clearAllSignals(void)
{
    for (QSignalSpy* spy: _rgSpys) {
        spy->clear();
    }
}

void MultiSignalSpyV2::timerEvent(QTimerEvent * event)
{
    Q_UNUSED(event);
    _timeout = true;
}

/// Waits the specified signal
///     @return false for timeout
bool MultiSignalSpyV2::waitForSignal(const char* signalName, int msecs)
{
    // Check input parameters
    if (msecs < -1 || msecs == 0)
        return false;
    
    // activate the timeout
    _timeout = false;
    int timerId;
    if (msecs != -1) {
        timerId = startTimer(msecs);
        Q_ASSERT(timerId);
    } else {
        timerId = 0;
    }
    
    // Begin waiting
    QSignalSpy* spy = getSpy(signalName);
    Q_ASSERT(spy);
    
    while (spy->count() == 0 && !_timeout) {
        QTest::qWait(100);
    }
    
    // Clean up and return status
    if (timerId) {
        killTimer(timerId);
    }

    return spy->count() != 0;
}

void MultiSignalSpyV2::_printSignalState(quint64 mask)
{
    for (int i=0; i<_rgSignalNames.count(); i++) {
        bool expected = (1ll << i) & mask;

        QSignalSpy* spy = _rgSpys[i];
        Q_ASSERT(spy != nullptr);
        qDebug() << "Signal index:" << i << "count:" << spy->count() << "expected:" << expected << _rgSignalNames[i];
    }
}

bool MultiSignalSpyV2::pullBoolFromSignal(const char* signalName)
{
    QSignalSpy* spy = getSpy(signalName);
    return spy->value(0).value(0).toBool();
}

int MultiSignalSpyV2::pullIntFromSignal(const char* signalName)
{
    QSignalSpy* spy = getSpy(signalName);
    return spy->value(0).value(0).toInt();
}

QGeoCoordinate MultiSignalSpyV2::pullQGeoCoordinateFromSignal(const char* signalName)
{
    QSignalSpy* spy = getSpy(signalName);
    return spy->value(0).value(0).value<QGeoCoordinate>();
}

quint64 MultiSignalSpyV2::signalNameToMask(const char* signalName)
{
    for (int i=0; i<_rgSignalNames.count(); i++) {
        if (_rgSignalNames[i] == signalName) {
            return 1ll << i;
        }
    }

    qWarning() << "MultiSignalSpyV2::signalNameToMask signal not found" << signalName;

    return 0;
}
