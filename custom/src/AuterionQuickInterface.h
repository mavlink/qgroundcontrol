/*!
 * @file
 *   @brief Auterion QtQuick Interface
 *   @author Gus Grubba <gus@grubba.com>
 */

#pragma once

#include "Vehicle.h"

#include <QObject>
#include <QTimer>
#include <QColor>
#include <QGeoPositionInfo>
#include <QGeoPositionInfoSource>

//-----------------------------------------------------------------------------
// QtQuick Interface (UI)
class AuterionQuickInterface : public QObject
{
    Q_OBJECT
public:
    AuterionQuickInterface(QObject* parent = nullptr);
    ~AuterionQuickInterface();

    enum CheckList {
        NotSetup = 0,
        Passed,
        Failed,
    };

    Q_ENUM(CheckList)

    Q_PROPERTY(bool         debugBuild      READ    debugBuild      CONSTANT)
    Q_PROPERTY(bool         testFlight      READ    testFlight      WRITE setTestFlight         NOTIFY testFlightChanged)
    Q_PROPERTY(QString      pilotID         READ    pilotID         WRITE setPilotID            NOTIFY pilotIDChanged)
    Q_PROPERTY(CheckList    checkListState  READ    checkListState  WRITE setCheckListState     NOTIFY checkListStateChanged)
    Q_PROPERTY(QStringList  batteries       READ    batteries       NOTIFY batteriesChanged)

    Q_INVOKABLE bool addBatteryScan (QString batteryID);
    Q_INVOKABLE void resetBatteries ();

    void        init                ();
    bool        testFlight          () { return _testFlight; }
#if defined(QT_DEBUG)
    bool        debugBuild          () { return true; }
#else
    bool        debugBuild          () { return true; }
#endif
    QString     pilotID             () { return _pilotID; }
    CheckList   checkListState      () { return _checkListState; }
    QStringList batteries           () { return _batteries; }

    void    setTestFlight           (bool b);
    void    setPilotID              (QString pid)   { _pilotID = pid; emit pilotIDChanged(); }
    void    setCheckListState       (CheckList cl)  { _checkListState = cl; emit checkListStateChanged(); }

signals:
    void    testFlightChanged       ();
    void    pilotIDChanged          ();
    void    checkListStateChanged   ();
    void    batteriesChanged        ();

private slots:
    void _activeVehicleChanged      (Vehicle* vehicle);
    void _armedChanged              (bool armed);

private:
    void        _sendLogMessage     ();

private:
#if defined(QT_DEBUG)
    bool        _testFlight     = true;
#else
    bool        _testFlight     = false;
#endif
    Vehicle*    _vehicle        = nullptr;
    QString     _pilotID;
    CheckList   _checkListState = NotSetup;
    QStringList _batteries;
};
