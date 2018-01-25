/****************************************************************************
 *
 *   (c) 2017 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
    Q_PROPERTY(quint32  windSpeed       READ windSpeed      NOTIFY weatherChanged)
    Q_PROPERTY(quint32  windGusting     READ windGusting    NOTIFY weatherChanged)
    Q_PROPERTY(qint32   temperature     READ temperature    NOTIFY weatherChanged)
    Q_PROPERTY(float    humidity        READ humidity       NOTIFY weatherChanged)
    Q_PROPERTY(quint32  visibility      READ visibility     NOTIFY weatherChanged)
    Q_PROPERTY(quint32  precipitation   READ precipitation  NOTIFY weatherChanged)

    virtual bool    valid           ()  = 0;    ///< Current weather data is valid
    virtual QString condition       ()  = 0;    ///< The overall weather condition.
    virtual QString icon            ()  = 0;    ///< 2:1 Aspect ratio icon url ready to be used by an Image QML Item
    virtual quint32 windHeading     ()  = 0;    ///< The heading in [°].
    virtual quint32 windSpeed       ()  = 0;    ///< The speed in [°].
    virtual quint32 windGusting     ()  = 0;
    virtual qint32  temperature     ()  = 0;    ///< The temperature in [°C].
    virtual float   humidity        ()  = 0;
    virtual quint32 visibility      ()  = 0;    ///< Visibility in [m].
    virtual quint32 precipitation   ()  = 0;    ///< The probability of precipitation in [%].

    /**
     * Set region of interest that should be queried. When finished, the weatherChanged() signal will be emmited.
     * @param center Center coordinate for ROI
     */
    virtual void setROI (const QGeoCoordinate& center) = 0;

signals:
    void weatherChanged  ();
};
