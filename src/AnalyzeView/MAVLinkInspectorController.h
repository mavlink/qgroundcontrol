/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

/// @file
/// @brief MAVLink message inspector and charting controller
/// @author Gus Grubba <gus@auterion.com>

#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QTimer>
#include <QtCore/QLoggingCategory>
#include <QtQmlIntegration/QtQmlIntegration>

#include "MAVLinkLib.h"
#include "QmlObjectListModel.h"

Q_DECLARE_LOGGING_CATEGORY(MAVLinkInspectorControllerLog)

class MAVLinkChartController;
class Vehicle;
class LinkInterface;
class QGCMAVLinkSystem;

//-----------------------------------------------------------------------------
/// MAVLink message inspector controller (provides the logic for UI display)
class MAVLinkInspectorController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_MOC_INCLUDE("LinkInterface.h")
    Q_MOC_INCLUDE("Vehicle.h")
    Q_MOC_INCLUDE("MAVLinkSystem.h")
    Q_MOC_INCLUDE("MAVLinkChartController.h")

public:
    MAVLinkInspectorController();
    ~MAVLinkInspectorController();

    Q_PROPERTY(QStringList          systemNames     READ systemNames    NOTIFY systemsChanged)
    Q_PROPERTY(QmlObjectListModel*  systems         READ systems        NOTIFY systemsChanged)
    Q_PROPERTY(QmlObjectListModel*  charts          READ charts         NOTIFY chartsChanged)
    Q_PROPERTY(QGCMAVLinkSystem*    activeSystem    READ activeSystem   NOTIFY activeSystemChanged)
    Q_PROPERTY(QStringList          timeScales      READ timeScales     NOTIFY timeScalesChanged)
    Q_PROPERTY(QStringList          rangeList       READ rangeList      NOTIFY rangeListChanged)

    Q_INVOKABLE MAVLinkChartController* createChart     ();
    Q_INVOKABLE void                    deleteChart     (MAVLinkChartController* chart);
    Q_INVOKABLE void                    setActiveSystem (int systemId);
    Q_INVOKABLE void                    setMessageInterval(int32_t rate);

    QmlObjectListModel* systems     () { return &_systems;     }
    QmlObjectListModel* charts      () { return &_charts;       }
    QGCMAVLinkSystem*   activeSystem() { return _activeSystem; }
    QStringList         systemNames () { return _systemNames;  }
    QStringList         timeScales  ();
    QStringList         rangeList   ();

    class TimeScale_st : public QObject {
    public:
        TimeScale_st(QObject* parent, const QString& l, uint32_t t);
        QString     label;
        uint32_t    timeScale;
    };

    class Range_st : public QObject {
    public:
        Range_st(QObject* parent, const QString& l, qreal r);
        QString     label;
        qreal       range;
    };

    const QList<TimeScale_st*>&     timeScaleSt         () { return _timeScaleSt; }
    const QList<Range_st*>&         rangeSt             () { return _rangeSt; }

signals:
    void systemsChanged     ();
    void chartsChanged      ();
    void activeSystemChanged();
    void timeScalesChanged  ();
    void rangeListChanged   ();

private slots:
    void _receiveMessage    (LinkInterface* link, mavlink_message_t message);
    void _vehicleAdded      (Vehicle* vehicle);
    void _vehicleRemoved    (Vehicle* vehicle);
    void _setActiveVehicle  (Vehicle* vehicle);
    void _refreshFrequency  ();

private:
    QGCMAVLinkSystem* _findVehicle (uint8_t id);

private:

    uint8_t             _selectedSystemID() const;
    uint8_t             _selectedComponentID() const;

    QStringList         _timeScales;
    QStringList         _rangeList;
    QGCMAVLinkSystem*   _activeSystem           = nullptr;
    QTimer              _updateFrequencyTimer;
    QStringList         _systemNames;
    QmlObjectListModel  _systems;                           ///< List of QGCMAVLinkSystem
    QmlObjectListModel  _charts;                            ///< List of MAVLinkCharts
    QList<TimeScale_st*>_timeScaleSt;
    QList<Range_st*>    _rangeSt;

};
