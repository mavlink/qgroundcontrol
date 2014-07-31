/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

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
    _signalEmitter(NULL),
    _rgSignals(NULL),
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

bool MultiSignalSpy::init(
                        QObject*        signalEmitter,  ///< [in] object which the signals are emitted from
                        const char**    rgSignals,      ///< [in] array of signals to spy on
                        size_t          cSignals)       ///< [in] numbers of signals in rgSignals
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
    Q_ASSERT(_rgSpys != NULL);
    for (size_t i=0; i<_cSignals; i++) {
        _rgSpys[i] = new QSignalSpy(_signalEmitter, _rgSignals[i]);
        if (_rgSpys[i] == NULL) {
            qDebug() << "Unabled to allocated QSignalSpy";
            return false;
        }
        if (!_rgSpys[i]->isValid()) {
            qDebug() << "Invalid signal";
            return false;
        }
    }
    
    return true;
}

/// @param mask bit mask specifying which signals to check. The lowest order bit represents
///     index 0 into the rgSignals array and so on up the bit mask.
/// @return true if signal count = 1 for the specified signals
bool MultiSignalSpy::checkSignalByMask(quint16 mask)
{
    for (size_t i=0; i<_cSignals; i++) {
        if ((1 << i) & mask) {
            QSignalSpy* spy = _rgSpys[i];
            Q_ASSERT(spy != NULL);
            
            if (spy->count() !=  1) {
                _printSignalState();
                return false;
            }
        }
    }
    
    return true;
}

/// @return true if signal count = 1 for specified signals and signal count of 0
///     for all other signals
bool MultiSignalSpy::checkOnlySignalByMask(quint16 mask)
{
    for (size_t i=0; i<_cSignals; i++) {
        QSignalSpy* spy = _rgSpys[i];
        Q_ASSERT(spy != NULL);

        if ((1 << i) & mask) {
            if (spy->count() != 1) {
                _printSignalState();
                return false;
            }
        } else {
            if (spy->count() != 0) {
                _printSignalState();
                return false;
            }
        }
    }
    
    return true;
}

/// @return true if signal count = 0 for specified signals
bool MultiSignalSpy::checkNoSignalByMask(quint16 mask)
{
    for (size_t i=0; i<_cSignals; i++) {
        if ((1 << i) & mask) {
            QSignalSpy* spy = _rgSpys[i];
            Q_ASSERT(spy != NULL);

            if (spy->count() != 0) {
                _printSignalState();
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
QSignalSpy* MultiSignalSpy::getSpyByIndex(quint16 index)
{
    Q_ASSERT(index < _cSignals);
    Q_ASSERT(_rgSpys[index] != NULL);

    return _rgSpys[index];
}

/// Sets the signal count to 0 for the specified signal
void MultiSignalSpy::clearSignalByIndex(quint16 index)
{
    Q_ASSERT(index < _cSignals);
    Q_ASSERT(_rgSpys[index] != NULL);
    
    _rgSpys[index]->clear();
}

/// Sets the signal count to 0 for all specified signals
void MultiSignalSpy::clearSignalsByMask(quint16 mask)
{
    for (size_t i=0; i<_cSignals; i++) {
        if ((1 << i) & mask) {
            QSignalSpy* spy = _rgSpys[i];
            Q_ASSERT(spy != NULL);

            spy->clear();
        }
    }
}

/// Sets the signal count to 0 for all signals
void MultiSignalSpy::clearAllSignals(void)
{
    for (quint16 i=0;i<_cSignals; i++) {
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
                                          quint16 index,  ///< [in] index of signal to wait on
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
        QCoreApplication::sendPostedEvents();
        QCoreApplication::processEvents();
        QCoreApplication::flush();
        QTest::qSleep(100);
    }
    
    // Clean up and return status
    if (timerId) {
        killTimer(timerId);
    }

    return spy->count() != 0;
}

void MultiSignalSpy::_printSignalState(void)
{
    for (size_t i=0; i<_cSignals; i++) {
        QSignalSpy* spy = _rgSpys[i];
        Q_ASSERT(spy != NULL);
        qDebug() << "Signal index:" << i << "count:" << spy->count();
    }
}
