/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "SettingsFact.h"

class CameraSpec : public QObject
{
    Q_OBJECT

public:
    CameraSpec(const QString& settingsGroup, QObject* parent = nullptr);

    const CameraSpec& operator=(const CameraSpec& other);

    // These properties are persisted to Json
    Q_PROPERTY(Fact* sensorWidth        READ sensorWidth        CONSTANT)   ///< Sensor size in millimeters
    Q_PROPERTY(Fact* sensorHeight       READ sensorHeight       CONSTANT)   ///< Sensor size in millimeters
    Q_PROPERTY(Fact* imageWidth         READ imageWidth         CONSTANT)   ///< Image size in pixels
    Q_PROPERTY(Fact* imageHeight        READ imageHeight        CONSTANT)   ///< Image size in pixels
    Q_PROPERTY(Fact* focalLength        READ focalLength        CONSTANT)   ///< Focal length in millimeters
    Q_PROPERTY(Fact* landscape          READ landscape          CONSTANT)   ///< true: camera is in landscape orientation
    Q_PROPERTY(Fact* fixedOrientation   READ fixedOrientation   CONSTANT)   ///< true: camera is in fixed orientation
    Q_PROPERTY(Fact* minTriggerInterval READ minTriggerInterval CONSTANT)   ///< Minimum time in seconds between each photo taken, 0 for not specified

    SettingsFact* sensorWidth       (void) { return &_sensorWidthFact; }
    SettingsFact* sensorHeight      (void) { return &_sensorHeightFact; }
    SettingsFact* imageWidth        (void) { return &_imageWidthFact; }
    SettingsFact* imageHeight       (void) { return &_imageHeightFact; }
    SettingsFact* focalLength       (void) { return &_focalLengthFact; }
    SettingsFact* landscape         (void) { return &_landscapeFact; }
    SettingsFact* fixedOrientation  (void) { return &_fixedOrientationFact; }
    SettingsFact* minTriggerInterval(void) { return &_minTriggerIntervalFact; }

    bool dirty      (void) const { return _dirty; }
    void setDirty   (bool dirty);

    void save(QJsonObject& json) const;
    bool load(const QJsonObject& json, QString& errorString);

signals:
    void dirtyChanged(bool dirty);

private:
    bool _dirty;

    QMap<QString, FactMetaData*> _metaDataMap;

    SettingsFact _sensorWidthFact;
    SettingsFact _sensorHeightFact;
    SettingsFact _imageWidthFact;
    SettingsFact _imageHeightFact;
    SettingsFact _focalLengthFact;
    SettingsFact _landscapeFact;
    SettingsFact _fixedOrientationFact;
    SettingsFact _minTriggerIntervalFact;

    static const char* _sensorWidthName;
    static const char* _sensorHeightName;
    static const char* _imageWidthName;
    static const char* _imageHeightName;
    static const char* _focalLengthName;
    static const char* _landscapeName;
    static const char* _fixedOrientationName;
    static const char* _minTriggerIntervalName;
};
