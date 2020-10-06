/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MultiSignalSpy.h"
#include <QEventLoop>
#include <QCoreApplication>
#include <QDebug>
#include <QTest>

/// @file
///     @brief This class allows you to keep track of signal counts on a set of signals associated with an object.
///     Mainly used for writing object unit tests.
///
///     @author Don Gagne <don@thegagnes.com>

MultiSignalSpy::MultiSignalSpy(QObject* parent) :
    QObject(parent),
    _signalEmitter(nullptr),
    _rgSignals(nullptr),
    _cSignals(0)
{
}

MultiSignalSpy::~MultiSignalSpy()
{
    Q_ASSERT(_rgSignals);

    for (size_t i=0; i<_cSignals; i++) {
        delete _rgSpys[i];
    }
}

/// Initializes the class. Must be called once before use.
///     @return true if success, false for failure

bool MultiSignalSpy::init(QObject*        signalEmitter,    ///< [in] object which the signals are emitted from
                          const char**    rgSignals,        ///< [in] array of signals to spy on
                          size_t          cSignals)         ///< [in] numbers of signals in rgSignals
{
    if (!signalEmitter || !rgSignals || cSignals == 0) {
        qDebug() << "Invalid arguments";
        return false;
    }
    
    _signalEmitter = signalEmitter;
    _rgSignals = rgSignals;
    _cSignals = cSignals;

    // Allocate and connect QSignalSpy's
    _rgSpys = new QSignalSpy*[_cSignals];
    Q_ASSERT(_rgSpys != nullptr);
    for (size_t i=0; i<_cSignals; i++) {
        _rgSpys[i] = new QSignalSpy(_signalEmitter, _rgSignals[i]);
        if (!_rgSpys[i]->isValid()) {
            qDebug() << "Invalid signal: index" << i;
            return false;
        }
    }
    
    return true;
}

bool MultiSignalSpy::_checkSignalByMaskWorker(quint32 mask, bool multipleSignalsAllowed)
{
    for (size_t i=0; i<_cSignals; i++) {
        if ((1 << i) & mask) {
            QSignalSpy* spy = _rgSpys[i];
            Q_ASSERT(spy != nullptr);
            
            if ((multipleSignalsAllowed && spy->count() ==  0) || (!multipleSignalsAllowed && spy->count() != 1)) {
                qDebug() << "Failed index:" << i;
                _printSignalState(mask);
                return false;
            }
        }
    }
    
    return true;
}

bool MultiSignalSpy::_checkOnlySignalByMaskWorker(quint32 mask, bool multipleSignalsAllowed)
{
    for (size_t i=0; i<_cSignals; i++) {
        QSignalSpy* spy = _rgSpys[i];
        Q_ASSERT(spy != nullptr);

        if ((1 << i) & mask) {
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

bool MultiSignalSpy::checkSignalByMask(quint32 mask)
{
    return _checkSignalByMaskWorker(mask, false /* multipleSignalsAllowed */);
}

bool MultiSignalSpy::checkOnlySignalByMask(quint32 mask)
{
    return _checkOnlySignalByMaskWorker(mask, false /* multipleSignalsAllowed */);
}

bool MultiSignalSpy::checkSignalsByMask(quint32 mask)
{
    return _checkSignalByMaskWorker(mask, true /* multipleSignalsAllowed */);
}

bool MultiSignalSpy::checkOnlySignalsByMask(quint32 mask)
{
    return _checkOnlySignalByMaskWorker(mask, true /* multipleSignalsAllowed */);
}

/// @return true if signal count = 0 for specified signals
bool MultiSignalSpy::checkNoSignalByMask(quint32 mask)
{
    for (size_t i=0; i<_cSignals; i++) {
        if ((1 << i) & mask) {
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
bool MultiSignalSpy::checkNoSignals(void)
{
    return checkNoSignalByMask(~0);
}

/// @return QSignalSpy for the specified signal
QSignalSpy* MultiSignalSpy::getSpyByIndex(quint32 index)
{
    Q_ASSERT(index < _cSignals);
    Q_ASSERT(_rgSpys[index] != nullptr);

    return _rgSpys[index];
}

/// Sets the signal count to 0 for the specified signal
void MultiSignalSpy::clearSignalByIndex(quint32 index)
{
    Q_ASSERT(index < _cSignals);
    Q_ASSERT(_rgSpys[index] != nullptr);
    
    _rgSpys[index]->clear();
}

/// Sets the signal count to 0 for all specified signals
void MultiSignalSpy::clearSignalsByMask(quint32 mask)
{
    for (size_t i=0; i<_cSignals; i++) {
        if ((1 << i) & mask) {
            QSignalSpy* spy = _rgSpys[i];
            Q_ASSERT(spy != nullptr);

            spy->clear();
        }
    }
}

/// Sets the signal count to 0 for all signals
void MultiSignalSpy::clearAllSignals(void)
{
    for (quint32 i=0;i<_cSignals; i++) {
        clearSignalByIndex(i);
    }
}

void MultiSignalSpy::timerEvent(QTimerEvent * event)
{
    Q_UNUSED(event);
    _timeout = true;
}

/// Waits the specified signal
///     @return false for timeout
bool MultiSignalSpy::waitForSignalByIndex(
                                          quint32 index,  ///< [in] index of signal to wait on
                                          int     msec)   ///< [in] numbers of milleconds to wait before timeout, -1 wait forever
{
    // Check input parameters
    if (msec < -1 || msec == 0)
        return false;
    
    // activate the timeout
    _timeout = false;
    int timerId;
    if (msec != -1) {
        timerId = startTimer(msec);
        Q_ASSERT(timerId);
    } else {
        timerId = 0;
    }
    
    // Begin waiting
    QSignalSpy* spy = _rgSpys[index];
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

void MultiSignalSpy::_printSignalState(quint32 mask)
{
    for (size_t i=0; i<_cSignals; i++) {
        bool expected = (1 << i) & mask;

        QSignalSpy* spy = _rgSpys[i];
        Q_ASSERT(spy != nullptr);
        qDebug() << "Signal index:" << i << "count:" << spy->count() << "expected:" << expected << _rgSignals[i];
    }
}

bool MultiSignalSpy::pullBoolFromSignalIndex(quint32 index)
{
    QSignalSpy* spy = getSpyByIndex(index);
    return spy->value(0).value(0).toBool();
}

int MultiSignalSpy::pullIntFromSignalIndex(quint32 index)
{
    QSignalSpy* spy = getSpyByIndex(index);
    return spy->value(0).value(0).toInt();
}

QGeoCoordinate MultiSignalSpy::pullQGeoCoordinateFromSignalIndex(quint32 index)
{
    QSignalSpy* spy = getSpyByIndex(index);
    return spy->value(0).value(0).value<QGeoCoordinate>();
}
