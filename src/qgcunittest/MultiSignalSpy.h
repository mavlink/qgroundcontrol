/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#pragma once

#include <QObject>
#include <QSignalSpy>
#include <QGeoCoordinate>

/// @file
///     @brief This class allows you to keep track of signal counts on a set of signals associated with an object.
///     Mainly used for writing object unit tests.
///
///     @author Don Gagne <don@thegagnes.com>

class MultiSignalSpy : public QObject
{
    Q_OBJECT
    
public:
    MultiSignalSpy(QObject* parent = nullptr);
    ~MultiSignalSpy();

    bool init(QObject* signalEmitter, const char** rgSignals, size_t cSignals);

    /// @param mask bit mask specifying which signals to check. The lowest order bit represents
    ///     index 0 into the rgSignals array and so on up the bit mask.
    /// @return true if signal count = 1 for the specified signals
    bool checkSignalByMask(quint32 mask);

    /// @return true if signal count = 1 for specified signals and signal count of 0
    ///     for all other signals
    bool checkOnlySignalByMask(quint32 mask);

    /// @param mask bit mask specifying which signals to check. The lowest order bit represents
    ///     index 0 into the rgSignals array and so on up the bit mask.
    /// @return true if signal count >= 1 for the specified signals
    bool checkSignalsByMask(quint32 mask);

    /// @return true if signal count >= 1 for specified signals and signal count of 0
    ///     for all other signals
    bool checkOnlySignalsByMask(quint32 mask);

    bool checkNoSignalByMask(quint32 mask);
    bool checkNoSignals(void);

    void clearSignalByIndex(quint32 index);
    void clearSignalsByMask(quint32 mask);
    void clearAllSignals(void);

    bool waitForSignalByIndex(quint32 index, int msec);
    
    QSignalSpy* getSpyByIndex(quint32 index);

    // Returns the value type for the first parameter of the signal
    bool pullBoolFromSignalIndex(quint32 index);
    int pullIntFromSignalIndex(quint32 index);
    QGeoCoordinate pullQGeoCoordinateFromSignalIndex(quint32 index);

private:
    // QObject overrides
    void timerEvent(QTimerEvent * event);
    
    void _printSignalState(quint32 mask);
    bool _checkSignalByMaskWorker(quint32 mask, bool multipleSignalsAllowed);
    bool _checkOnlySignalByMaskWorker(quint32 mask, bool multipleSignalsAllowed);

    QObject*        _signalEmitter;
    const char**    _rgSignals;
    QSignalSpy**    _rgSpys;
    size_t          _cSignals;
    bool            _timeout;
};

