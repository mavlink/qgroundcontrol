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
    CameraMetaData(const QString&   canonicalName,
                   const QString&   brand,
                   const QString&   model,
                   double           sensorWidth,
                   double           sensorHeight,
                   double           imageWidth,
                   double           imageHeight,
                   double           focalLength,
                   bool             landscape,
                   bool             fixedOrientation,
                   double           minTriggerInterval,
                   const QString&   deprecatedTranslatedName,
                   QObject*         parent = nullptr);

    Q_PROPERTY(QString  canonicalName               MEMBER canonicalName            CONSTANT)
    Q_PROPERTY(QString  deprecatedTranslatedName    MEMBER deprecatedTranslatedName CONSTANT)
    Q_PROPERTY(QString  brand                       MEMBER brand                    CONSTANT)
    Q_PROPERTY(QString  model                       MEMBER model                    CONSTANT)
    Q_PROPERTY(double   sensorWidth                 MEMBER sensorWidth              CONSTANT)
    Q_PROPERTY(double   sensorHeight                MEMBER sensorHeight             CONSTANT)
    Q_PROPERTY(double   imageWidth                  MEMBER imageWidth               CONSTANT)
    Q_PROPERTY(double   imageHeight                 MEMBER imageHeight              CONSTANT)
    Q_PROPERTY(double   focalLength                 MEMBER focalLength              CONSTANT)
    Q_PROPERTY(bool     landscape                   MEMBER landscape                CONSTANT)
    Q_PROPERTY(bool     fixedOrientation            MEMBER fixedOrientation         CONSTANT)
    Q_PROPERTY(double   minTriggerInterval          MEMBER minTriggerInterval       CONSTANT)

    QString canonicalName;                  ///< Canonical name saved in plan files. Not translated.
    QString brand;                          ///< Camera brand. Used for grouping.
    QString model;                          ///< Camerar model
    double  sensorWidth;                    ///< Sensor size in millimeters
    double  sensorHeight;                   ///< Sensor size in millimeters
    double  imageWidth;                     ///< Image size in pixels
    double  imageHeight;                    ///< Image size in pixels
    double  focalLength;                    ///< Focal length in millimeters
    bool    landscape;                      ///< true: camera is in landscape orientation
    bool    fixedOrientation;               ///< true: camera is in fixed orientation
    double  minTriggerInterval;             ///< Minimum time in seconds between each photo taken, 0 for not specified

    /// In older builds camera names were incorrect marked for translation. This leads to plan files which have are language
    /// dependant which is not a good thing. Newer plan files use the canonical name which is not translated. In order to support
    /// loading older plan files we continue to include the incorrect translation so we can match against them as needed.
    /// Newly added CameraMetaData entries should leave this value empty.
    QString deprecatedTranslatedName;
};
