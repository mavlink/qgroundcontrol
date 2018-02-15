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
class AirspaceRestrictionProvider;

class AirspaceController : public QObject
{
    Q_OBJECT
public:
    AirspaceController(QObject* parent = NULL);
    ~AirspaceController() = default;

    Q_PROPERTY(QString                      providerName        READ providerName       CONSTANT)
    Q_PROPERTY(AirspaceWeatherInfoProvider* weatherInfo         READ weatherInfo        CONSTANT)
    Q_PROPERTY(AirspaceAdvisoryProvider*    advisories          READ advisories         CONSTANT)
    Q_PROPERTY(AirspaceRulesetsProvider*    ruleSets            READ ruleSets           CONSTANT)
    Q_PROPERTY(AirspaceRestrictionProvider* airspaces           READ airspaces          CONSTANT)
    Q_PROPERTY(bool                         airspaceVisible     READ airspaceVisible    WRITE setairspaceVisible    NOTIFY airspaceVisibleChanged)

    Q_INVOKABLE void setROI                             (QGeoCoordinate center, double radius);

    QString                         providerName        ();
    AirspaceWeatherInfoProvider*    weatherInfo         ();
    AirspaceAdvisoryProvider*       advisories          ();
    AirspaceRulesetsProvider*       ruleSets            ();
    AirspaceRestrictionProvider*    airspaces           ();
    bool                            airspaceVisible     () { return _airspaceVisible; }

    void                            setairspaceVisible  (bool set) { _airspaceVisible = set; emit airspaceVisibleChanged(); }

signals:
    void airspaceVisibleChanged                         ();

private:
    AirspaceManager*    _manager;
    bool                _airspaceVisible;
};
