/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#ifndef CameraMetaData_H
#define CameraMetaData_H

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
                   QObject*         parent = NULL);

    Q_PROPERTY(QString  name                MEMBER _name                CONSTANT)   ///< Camera name
    Q_PROPERTY(double   sensorWidth         MEMBER _sensorWidth         CONSTANT)   ///< Sensor size in millimeters
    Q_PROPERTY(double   sensorHeight        MEMBER _sensorHeight        CONSTANT)   ///< Sensor size in millimeters
    Q_PROPERTY(double   imageWidth          MEMBER _imageWidth          CONSTANT)   ///< Image size in pixels
    Q_PROPERTY(double   imageHeight         MEMBER _imageHeight         CONSTANT)   ///< Image size in pixels
    Q_PROPERTY(double   focalLength         MEMBER _focalLength         CONSTANT)   ///< Focal length in millimeters
    Q_PROPERTY(bool     landscape           MEMBER _landscape           CONSTANT)   ///< true: camera is in landscape orientation
    Q_PROPERTY(bool     fixedOrientation    MEMBER _fixedOrientation    CONSTANT)   ///< true: camera is in fixed orientation
    Q_PROPERTY(double   minTriggerInterval  MEMBER _minTriggerInterval  CONSTANT)   ///< Minimum time in seconds between each photo taken, 0 for not specified

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

#endif
