/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "CameraSpec.h"

class Vehicle;

class CameraCalc : public CameraSpec
{
    Q_OBJECT

public:
    CameraCalc(Vehicle* vehicle, QObject* parent = NULL);

    Q_PROPERTY(QString          cameraName                  READ cameraName WRITE setCameraName                                 NOTIFY cameraNameChanged)
    Q_PROPERTY(QString          customCameraName            READ customCameraName                                               CONSTANT)                                   ///< Camera name for custom camera setting
    Q_PROPERTY(QString          manualCameraName            READ manualCameraName                                               CONSTANT)                                   ///< Camera name for manual camera setting
    Q_PROPERTY(bool             isManualCamera              READ isManualCamera                                                 NOTIFY cameraNameChanged)                   ///< true: using manual camera
    Q_PROPERTY(Fact*            valueSetIsDistance          READ valueSetIsDistance                                             CONSTANT)                                   ///< true: distance specified, resolution calculated
    Q_PROPERTY(Fact*            distanceToSurface           READ distanceToSurface                                              CONSTANT)                                   ///< Distance to surface for image foot print calculation
    Q_PROPERTY(Fact*            imageDensity                READ imageDensity                                                   CONSTANT)                                   ///< Image density on surface (cm/px)
    Q_PROPERTY(Fact*            frontalOverlap              READ frontalOverlap                                                 CONSTANT)
    Q_PROPERTY(Fact*            sideOverlap                 READ sideOverlap                                                    CONSTANT)
    Q_PROPERTY(Fact*            adjustedFootprintSide       READ adjustedFootprintSide                                          CONSTANT)                                   ///< Side footprint adjusted down for overlap
    Q_PROPERTY(Fact*            adjustedFootprintFrontal    READ adjustedFootprintFrontal                                       CONSTANT)                                   ///< Frontal footprint adjusted down for overlap
    Q_PROPERTY(bool             distanceToSurfaceRelative   READ distanceToSurfaceRelative WRITE setDistanceToSurfaceRelative   NOTIFY distanceToSurfaceRelativeChanged)

    // The following values are calculated from the camera properties
    Q_PROPERTY(double imageFootprintSide    READ imageFootprintSide     NOTIFY imageFootprintSideChanged)       ///< Size of image size side in meters
    Q_PROPERTY(double imageFootprintFrontal READ imageFootprintFrontal  NOTIFY imageFootprintFrontalChanged)    ///< Size of image size frontal in meters

    static QString customCameraName(void);
    static QString manualCameraName(void);
    QString cameraName(void) const { return _cameraName; }
    void setCameraName(QString cameraName);

    Fact* valueSetIsDistance        (void) { return &_valueSetIsDistanceFact; }
    Fact* distanceToSurface         (void) { return &_distanceToSurfaceFact; }
    Fact* imageDensity              (void) { return &_imageDensityFact; }
    Fact* frontalOverlap            (void) { return &_frontalOverlapFact; }
    Fact* sideOverlap               (void) { return &_sideOverlapFact; }
    Fact* adjustedFootprintSide     (void) { return &_adjustedFootprintSideFact; }
    Fact* adjustedFootprintFrontal  (void) { return &_adjustedFootprintFrontalFact; }

    const Fact* valueSetIsDistance          (void) const { return &_valueSetIsDistanceFact; }
    const Fact* distanceToSurface           (void) const { return &_distanceToSurfaceFact; }
    const Fact* imageDensity                (void) const { return &_imageDensityFact; }
    const Fact* frontalOverlap              (void) const { return &_frontalOverlapFact; }
    const Fact* sideOverlap                 (void) const { return &_sideOverlapFact; }
    const Fact* adjustedFootprintSide       (void) const { return &_adjustedFootprintSideFact; }
    const Fact* adjustedFootprintFrontal    (void) const { return &_adjustedFootprintFrontalFact; }

    bool    dirty                       (void) const { return _dirty; }
    bool    isManualCamera              (void) { return cameraName() == manualCameraName(); }
    double  imageFootprintSide          (void) const { return _imageFootprintSide; }
    double  imageFootprintFrontal       (void) const { return _imageFootprintFrontal; }
    bool    distanceToSurfaceRelative   (void) const { return _distanceToSurfaceRelative; }

    void setDirty                       (bool dirty);
    void setDistanceToSurfaceRelative   (bool distanceToSurfaceRelative);

    void save(QJsonObject& json) const;
    bool load(const QJsonObject& json, QString& errorString);

signals:
    void cameraNameChanged                  (QString cameraName);
    void dirtyChanged                       (bool dirty);
    void imageFootprintSideChanged          (double imageFootprintSide);
    void imageFootprintFrontalChanged       (double imageFootprintFrontal);
    void distanceToSurfaceRelativeChanged   (bool distanceToSurfaceRelative);

private slots:
    void _recalcTriggerDistance             (void);
    void _adjustDistanceToSurfaceRelative   (void);

private:
    Vehicle*        _vehicle;
    bool            _dirty;
    QString         _cameraName;
    bool            _disableRecalc;
    bool            _distanceToSurfaceRelative;

    QMap<QString, FactMetaData*> _metaDataMap;

    Fact _valueSetIsDistanceFact;
    Fact _distanceToSurfaceFact;
    Fact _imageDensityFact;
    Fact _frontalOverlapFact;
    Fact _sideOverlapFact;
    Fact _adjustedFootprintSideFact;
    Fact _adjustedFootprintFrontalFact;

    double _imageFootprintSide;
    double _imageFootprintFrontal;

    QVariantList _knownCameraList;

    static const char* _valueSetIsDistanceName;
    static const char* _distanceToSurfaceName;
    static const char* _distanceToSurfaceRelativeName;
    static const char* _imageDensityName;
    static const char* _frontalOverlapName;
    static const char* _sideOverlapName;
    static const char* _adjustedFootprintSideName;
    static const char* _adjustedFootprintFrontalName;
    static const char* _jsonCameraNameKey;

    // The following are deprecated usage and only included in order to convert older formats

    enum CameraSpecType {
        CameraSpecNone,
        CameraSpecCustom,
        CameraSpecKnown
    };

    static const char* _jsonCameraSpecTypeKey;
};
