/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QObject>
#include <QGeoCoordinate>

class AirspaceManager;
class QmlObjectListModel;
class AirspaceWeatherInfoProvider;
class AirspaceAdvisoryProvider;
class AirspaceRulesetsProvider;

class AirspaceController : public QObject
{
    Q_OBJECT
public:
    AirspaceController(QObject* parent = NULL);
    ~AirspaceController() = default;

    Q_PROPERTY(QmlObjectListModel*          polygons            READ polygons           CONSTANT)   ///< List of AirspacePolygonRestriction objects
    Q_PROPERTY(QmlObjectListModel*          circles             READ circles            CONSTANT)   ///< List of AirspaceCircularRestriction objects
    Q_PROPERTY(QString                      providerName        READ providerName       CONSTANT)
    Q_PROPERTY(AirspaceWeatherInfoProvider* weatherInfo         READ weatherInfo        CONSTANT)
    Q_PROPERTY(AirspaceAdvisoryProvider*    advisories          READ advisories         CONSTANT)
    Q_PROPERTY(AirspaceRulesetsProvider*    rulesets            READ rulesets           CONSTANT)
    Q_PROPERTY(bool                         airspaceVisible     READ airspaceVisible    WRITE setairspaceVisible    NOTIFY airspaceVisibleChanged)

    Q_INVOKABLE void setROI                             (QGeoCoordinate center, double radius);

    QmlObjectListModel*             polygons            ();
    QmlObjectListModel*             circles             ();
    QString                         providerName        ();
    AirspaceWeatherInfoProvider*    weatherInfo         ();
    AirspaceAdvisoryProvider*       advisories          ();
    AirspaceRulesetsProvider*       rulesets            ();
    bool                            airspaceVisible     () { return _airspaceVisible; }

    void                            setairspaceVisible  (bool set) { _airspaceVisible = set; emit airspaceVisibleChanged(); }

signals:
    void airspaceVisibleChanged                         ();

private:
    AirspaceManager*    _manager;
    bool                _airspaceVisible;
};
