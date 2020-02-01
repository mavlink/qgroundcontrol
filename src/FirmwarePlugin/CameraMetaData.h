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

/// Set of meta data which describes a camera available on the vehicle
class CameraMetaData : public QObject
{
    Q_OBJECT

public:
    CameraMetaData(const QString&   name,
                   double           sensorWidth,
                   double           sensorHeight,
                   double           imageWidth,
                   double           imageHeight,
                   double           focalLength,
                   bool             landscape,
                   bool             fixedOrientation,
                   double           minTriggerInterval,
                   QObject*         parent = nullptr);

    Q_PROPERTY(QString  name                READ name               CONSTANT)   ///< Camera name
    Q_PROPERTY(double   sensorWidth         READ sensorWidth        CONSTANT)   ///< Sensor size in millimeters
    Q_PROPERTY(double   sensorHeight        READ sensorHeight       CONSTANT)   ///< Sensor size in millimeters
    Q_PROPERTY(double   imageWidth          READ imageWidth         CONSTANT)   ///< Image size in pixels
    Q_PROPERTY(double   imageHeight         READ imageHeight        CONSTANT)   ///< Image size in pixels
    Q_PROPERTY(double   focalLength         READ focalLength        CONSTANT)   ///< Focal length in millimeters
    Q_PROPERTY(bool     landscape           READ landscape          CONSTANT)   ///< true: camera is in landscape orientation
    Q_PROPERTY(bool     fixedOrientation    READ fixedOrientation   CONSTANT)   ///< true: camera is in fixed orientation
    Q_PROPERTY(double   minTriggerInterval  READ minTriggerInterval CONSTANT)   ///< Minimum time in seconds between each photo taken, 0 for not specified

    QString name                (void) const { return _name; }
    double  sensorWidth         (void) const { return _sensorWidth; }
    double  sensorHeight        (void) const { return _sensorHeight; }
    double  imageWidth          (void) const { return _imageWidth; }
    double  imageHeight         (void) const { return _imageHeight; }
    double  focalLength         (void) const { return _focalLength; }
    bool    landscape           (void) const { return _landscape; }
    bool    fixedOrientation    (void) const { return _fixedOrientation; }
    double  minTriggerInterval  (void) const { return _minTriggerInterval; }

private:
    QString _name;
    double  _sensorWidth;
    double  _sensorHeight;
    double  _imageWidth;
    double  _imageHeight;
    double  _focalLength;
    bool    _landscape;
    bool    _fixedOrientation;
    double  _minTriggerInterval;
};
