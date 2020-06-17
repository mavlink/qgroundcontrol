/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

/**
 * @file AirspaceWeatherInfoProvider.h
 * Weather information provided by the Airspace Managemement
 */

#include "QGCGeoBoundingCube.h"
#include <QObject>
#include <QGeoCoordinate>

class AirspaceWeatherInfoProvider : public QObject
{
    Q_OBJECT
public:
    AirspaceWeatherInfoProvider(QObject *parent = nullptr);
    virtual ~AirspaceWeatherInfoProvider() {}

    Q_PROPERTY(bool     valid           READ valid          NOTIFY weatherChanged)
    Q_PROPERTY(QString  condition       READ condition      NOTIFY weatherChanged)
    Q_PROPERTY(QString  icon            READ icon           NOTIFY weatherChanged)
    Q_PROPERTY(quint32  windHeading     READ windHeading    NOTIFY weatherChanged)
    Q_PROPERTY(float    windSpeed       READ windSpeed      NOTIFY weatherChanged)
    Q_PROPERTY(quint32  windGusting     READ windGusting    NOTIFY weatherChanged)
    Q_PROPERTY(float    temperature     READ temperature    NOTIFY weatherChanged)
    Q_PROPERTY(float    humidity        READ humidity       NOTIFY weatherChanged)
    Q_PROPERTY(float    visibility      READ visibility     NOTIFY weatherChanged)
    Q_PROPERTY(float    precipitation   READ precipitation  NOTIFY weatherChanged)

    virtual bool    valid           ()  = 0;    ///< Current weather data is valid
    virtual QString condition       ()  = 0;    ///< The overall weather condition.
    virtual QString icon            ()  = 0;    ///< 2:1 Aspect ratio icon url ready to be used by an Image QML Item
    virtual quint32 windHeading     ()  = 0;    ///< The heading in [°].
    virtual float   windSpeed       ()  = 0;    ///< The speed in [°].
    virtual quint32 windGusting     ()  = 0;
    virtual float   temperature     ()  = 0;    ///< The temperature in [°C].
    virtual float   humidity        ()  = 0;
    virtual float   visibility      ()  = 0;    ///< Visibility in [m].
    virtual float   precipitation   ()  = 0;    ///< The probability of precipitation in [%].

    /**
     * Set region of interest that should be queried. When finished, the weatherChanged() signal will be emmited.
     * @param center Center coordinate for ROI
     */
    virtual void setROI (const QGCGeoBoundingCube& roi, bool reset = false) = 0;

signals:
    void weatherChanged  ();
};
