/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QList>
#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QPointF>
#include <QtCore/QString>
#include <QtCore/QVariantList>
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
    QML_ELEMENT
    Q_MOC_INCLUDE("LinkInterface.h")
    Q_MOC_INCLUDE("Vehicle.h")
    Q_MOC_INCLUDE("MAVLinkSystem.h")
    Q_MOC_INCLUDE("MAVLinkChartController.h")
    Q_MOC_INCLUDE("QmlObjectListModel.h")
    Q_PROPERTY(QmlObjectListModel   *systems        READ systems        NOTIFY systemsChanged)
    Q_PROPERTY(QGCMAVLinkSystem     *activeSystem   READ activeSystem   NOTIFY activeSystemChanged)
    Q_PROPERTY(QStringList          timeScales      READ timeScales     NOTIFY timeScalesChanged)
    Q_PROPERTY(QStringList          rangeList       READ rangeList      NOTIFY rangeListChanged)
    Q_PROPERTY(QStringList          systemNames     READ systemNames    NOTIFY systemsChanged)
    Q_PROPERTY(bool                 gpsXEnabled     READ gpsXEnabled    WRITE setGpsXEnabled    NOTIFY gpsXEnabledChanged)
    Q_PROPERTY(bool                 gpsYEnabled     READ gpsYEnabled    WRITE setGpsYEnabled    NOTIFY gpsYEnabledChanged)
    Q_PROPERTY(bool                 odomXEnabled    READ odomXEnabled   WRITE setOdomXEnabled   NOTIFY odomXEnabledChanged)
    Q_PROPERTY(bool                 odomYEnabled    READ odomYEnabled   WRITE setOdomYEnabled   NOTIFY odomYEnabledChanged)
    Q_PROPERTY(bool                 gpsXYEnabled    READ gpsXYEnabled                           NOTIFY gpsXYEnabledChanged)
    Q_PROPERTY(bool                 odomXYEnabled   READ odomXYEnabled                          NOTIFY odomXYEnabledChanged)
    Q_PROPERTY(bool                 xyPlotVisible   READ xyPlotVisible                          NOTIFY xyPlotVisibleChanged)
    Q_PROPERTY(QVariantList         gpsXYPoints     READ gpsXYPoints                            NOTIFY gpsXYPointsChanged)
    Q_PROPERTY(QVariantList         odomXYPoints    READ odomXYPoints                           NOTIFY odomXYPointsChanged)
    Q_PROPERTY(qreal                xyMinX          READ xyMinX                                 NOTIFY xyRangeChanged)
    Q_PROPERTY(qreal                xyMaxX          READ xyMaxX                                 NOTIFY xyRangeChanged)
    Q_PROPERTY(qreal                xyMinY          READ xyMinY                                 NOTIFY xyRangeChanged)
    Q_PROPERTY(qreal                xyMaxY          READ xyMaxY                                 NOTIFY xyRangeChanged)

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

    Q_INVOKABLE void setActiveSystem(int systemId);
    Q_INVOKABLE void setMessageInterval(int32_t rate) const;

    QmlObjectListModel *systems() const { return _systems; }
    QGCMAVLinkSystem *activeSystem() const { return _activeSystem; }
    QStringList systemNames() const { return _systemNames; }
    QStringList timeScales();
    QStringList rangeList();

    const QList<TimeScale_st*> &timeScaleSt() const { return _timeScaleSt; }
    const QList<Range_st*> &rangeSt() const { return _rangeSt; }

    bool gpsXEnabled() const { return _gpsXEnabled; }
    void setGpsXEnabled(bool enabled);
    bool gpsYEnabled() const { return _gpsYEnabled; }
    void setGpsYEnabled(bool enabled);
    bool odomXEnabled() const { return _odomXEnabled; }
    void setOdomXEnabled(bool enabled);
    bool odomYEnabled() const { return _odomYEnabled; }
    void setOdomYEnabled(bool enabled);
    bool gpsXYEnabled() const { return _gpsXEnabled && _gpsYEnabled; }
    bool odomXYEnabled() const { return _odomXEnabled && _odomYEnabled; }
    bool xyPlotVisible() const { return _xyPlotVisible; }

    QVariantList gpsXYPoints() const { return _gpsXYPoints; }
    QVariantList odomXYPoints() const { return _odomXYPoints; }

    qreal xyMinX() const { return _xyMinX; }
    qreal xyMaxX() const { return _xyMaxX; }
    qreal xyMinY() const { return _xyMinY; }
    qreal xyMaxY() const { return _xyMaxY; }

signals:
    void activeSystemChanged();
    void gpsXEnabledChanged();
    void gpsYEnabledChanged();
    void odomXEnabledChanged();
    void odomYEnabledChanged();
    void gpsXYEnabledChanged();
    void odomXYEnabledChanged();
    void gpsXYPointsChanged();
    void odomXYPointsChanged();
    void xyPlotVisibleChanged();
    void xyRangeChanged();
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
    void _processXYData(const mavlink_message_t &message);
    void _handleGpsRawInt(const mavlink_message_t &message);
    void _handleOdometry(const mavlink_message_t &message);
    void _appendGpsPoint(double x, double y);
    void _appendOdomPoint(double x, double y);
    void _postGpsEnablementChange(bool wasActive);
    void _postOdomEnablementChange(bool wasActive);
    void _updateXYPlotVisible();
    void _clearXYData();
    void _updateXYBounds();

    QStringList _timeScales;
    QStringList _rangeList;
    QStringList _systemNames;
    QList<TimeScale_st*>_timeScaleSt;
    QList<Range_st*> _rangeSt;
    QGCMAVLinkSystem *_activeSystem = nullptr;
    QTimer *_updateFrequencyTimer = nullptr;
    QmlObjectListModel *_systems = nullptr;     ///< List of QGCMAVLinkSystem
    QVariantList _gpsXYPoints;
    QVariantList _odomXYPoints;
    QList<QPointF> _gpsPoints;
    QList<QPointF> _odomPoints;
    qreal _xyMinX = -1.0;
    qreal _xyMaxX = 1.0;
    qreal _xyMinY = -1.0;
    qreal _xyMaxY = 1.0;
    bool _xyPlotVisible = false;
    bool _gpsXEnabled = false;
    bool _gpsYEnabled = false;
    bool _odomXEnabled = false;
    bool _odomYEnabled = false;
    static constexpr int _maxXYPointCount = 600;
};
