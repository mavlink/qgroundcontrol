/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtQmlIntegration/QtQmlIntegration>

#include "MAVLinkLib.h"

Q_DECLARE_LOGGING_CATEGORY(MAVLinkInspectorControllerLog)

class LinkInterface;
class MAVLinkChartController;
class QGCMAVLinkSystem;
class QmlObjectListModel;
class QTimer;
class Vehicle;

/// MAVLink message inspector controller (provides the logic for UI display)
class MAVLinkInspectorController : public QObject
{
    Q_OBJECT
    // QML_ELEMENT
    Q_MOC_INCLUDE("LinkInterface.h")
    Q_MOC_INCLUDE("Vehicle.h")
    Q_MOC_INCLUDE("MAVLinkSystem.h")
    Q_MOC_INCLUDE("MAVLinkChartController.h")
    Q_MOC_INCLUDE("QmlObjectListModel.h")
    Q_PROPERTY(QmlObjectListModel   *systems        READ systems        NOTIFY systemsChanged)
    Q_PROPERTY(QmlObjectListModel   *charts         READ charts         NOTIFY chartsChanged)
    Q_PROPERTY(QGCMAVLinkSystem     *activeSystem   READ activeSystem   NOTIFY activeSystemChanged)
    Q_PROPERTY(QStringList          timeScales      READ timeScales     NOTIFY timeScalesChanged)
    Q_PROPERTY(QStringList          rangeList       READ rangeList      NOTIFY rangeListChanged)
    Q_PROPERTY(QStringList          systemNames     READ systemNames    NOTIFY systemsChanged)

    struct TimeScale_st
    {
        TimeScale_st(const QString &label_, uint32_t timeScale_);

        QString label;
        uint32_t timeScale;
    };

    struct Range_st
    {
        Range_st(const QString &label_, qreal range_);

        QString label;
        qreal range;
    };

public:
    explicit MAVLinkInspectorController(QObject *parent = nullptr);
    ~MAVLinkInspectorController();

    Q_INVOKABLE MAVLinkChartController *createChart();
    Q_INVOKABLE void deleteChart(MAVLinkChartController *chart);
    Q_INVOKABLE void setActiveSystem(int systemId);
    Q_INVOKABLE void setMessageInterval(int32_t rate) const;

    QmlObjectListModel *systems() const { return _systems; }
    QmlObjectListModel *charts() const { return _charts; }
    QGCMAVLinkSystem *activeSystem() const { return _activeSystem; }
    QStringList systemNames() const { return _systemNames; }
    QStringList timeScales();
    QStringList rangeList();

    const QList<TimeScale_st*> &timeScaleSt() const { return _timeScaleSt; }
    const QList<Range_st*> &rangeSt() const { return _rangeSt; }

signals:
    void activeSystemChanged();
    void chartsChanged();
    void rangeListChanged();
    void systemsChanged();
    void timeScalesChanged();

private slots:
    void _receiveMessage(LinkInterface *link, const mavlink_message_t &message);
    void _refreshFrequency();
    void _setActiveVehicle(Vehicle *vehicle);
    void _vehicleAdded(Vehicle *vehicle);
    void _vehicleRemoved(const Vehicle *vehicle);

private:
    QGCMAVLinkSystem *_findVehicle(uint8_t id);
    uint8_t _selectedSystemID() const;
    uint8_t _selectedComponentID() const;

    QStringList _timeScales;
    QStringList _rangeList;
    QStringList _systemNames;
    QList<TimeScale_st*>_timeScaleSt;
    QList<Range_st*> _rangeSt;
    QGCMAVLinkSystem *_activeSystem = nullptr;
    QTimer *_updateFrequencyTimer = nullptr;
    QmlObjectListModel *_systems = nullptr;     ///< List of QGCMAVLinkSystem
    QmlObjectListModel *_charts = nullptr;      ///< List of MAVLinkCharts
};
