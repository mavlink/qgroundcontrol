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
///     @brief Works just like MultiSignalSpy but the signal arrays are setup automatically through introspection on
///     QMetaObject information. So no need to set up array an index/mask enums.

class MultiSignalSpyV2 : public QObject
{
    Q_OBJECT
    
public:
    MultiSignalSpyV2(QObject* parent = nullptr);
    ~MultiSignalSpyV2();

    bool init(QObject* signalEmitter);

    quint64 signalNameToMask(const char* signalName);

    /// @param mask bit mask specifying which signals to check. The lowest order bit represents
    ///     index 0 into the rgSignals array and so on up the bit mask.
    /// @return true if signal count = 1 for the specified signals
    bool checkSignalByMask(quint64 mask);

    /// @return true if signal count = 1 for specified signals and signal count of 0
    ///     for all other signals
    bool checkOnlySignalByMask(quint64 mask);

    /// @param mask bit mask specifying which signals to check. The lowest order bit represents
    ///     index 0 into the rgSignals array and so on up the bit mask.
    /// @return true if signal count >= 1 for the specified signals
    bool checkSignalsByMask(quint64 mask);

    /// @return true if signal count >= 1 for specified signals and signal count of 0
    ///     for all other signals
    bool checkOnlySignalsByMask(quint64 mask);

    bool checkNoSignalByMask(quint64 mask);
    bool checkNoSignals(void);

    void clearSignal(const char* signalName);
    void clearSignalsByMask(quint64 mask);
    void clearAllSignals(void);

    bool waitForSignal(const char* signalName, int msec);
    
    QSignalSpy* getSpy(const char* signalName);

    // Returns the value type for the first parameter of the signal
    bool            pullBoolFromSignal          (const char* signalName);
    int             pullIntFromSignal           (const char* signalName);
    QGeoCoordinate  pullQGeoCoordinateFromSignal(const char* signalName);

private:
    // QObject overrides
    void timerEvent(QTimerEvent * event);
    
    void _printSignalState              (quint64 mask);
    bool _checkSignalByMaskWorker       (quint64 mask, bool multipleSignalsAllowed);
    bool _checkOnlySignalByMaskWorker   (quint64 mask, bool multipleSignalsAllowed);

    QObject*            _signalEmitter = nullptr;
    QStringList         _rgSignalNames;
    QList<QSignalSpy*>  _rgSpys;
    bool                _timeout;
};

