/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "CameraSpec.h"
#include "SettingsFact.h"

class PlanMasterController;

class CameraCalc : public CameraSpec
{
    Q_OBJECT

public:
    CameraCalc(PlanMasterController* masterController, const QString& settingsGroup, QObject* parent = nullptr);

    Q_PROPERTY(QString          xlatCustomCameraName        READ xlatCustomCameraName                                           CONSTANT)                                   ///< User visible camera name for custom camera setting
    Q_PROPERTY(QString          xlatManualCameraName        READ xlatManualCameraName                                           CONSTANT)                                   ///< User visible camera name for manual camera setting
    Q_PROPERTY(bool             isManualCamera              READ isManualCamera                                                 NOTIFY isManualCameraChanged)
    Q_PROPERTY(bool             isCustomCamera              READ isCustomCamera                                                 NOTIFY isCustomCameraChanged)
    Q_PROPERTY(QString          cameraBrand                 MEMBER _cameraBrand         WRITE setCameraBrand                    NOTIFY cameraBrandChanged)
    Q_PROPERTY(QString          cameraModel                 MEMBER _cameraModel         WRITE setCameraModel                    NOTIFY cameraModelChanged)
    Q_PROPERTY(QStringList      cameraBrandList             MEMBER _cameraBrandList                                             CONSTANT)
    Q_PROPERTY(QStringList      cameraModelList             MEMBER _cameraModelList                                             NOTIFY cameraModelListChanged)
    Q_PROPERTY(Fact*            valueSetIsDistance          READ valueSetIsDistance                                             CONSTANT)                                   ///< true: distance specified, resolution calculated
    Q_PROPERTY(Fact*            distanceToSurface           READ distanceToSurface                                              CONSTANT)                                   ///< Distance to surface for image foot print calculation
    Q_PROPERTY(Fact*            imageDensity                READ imageDensity                                                   CONSTANT)                                   ///< Image density on surface (cm/px)
    Q_PROPERTY(Fact*            frontalOverlap              READ frontalOverlap                                                 CONSTANT)
    Q_PROPERTY(Fact*            sideOverlap                 READ sideOverlap                                                    CONSTANT)
    Q_PROPERTY(Fact*            adjustedFootprintSide       READ adjustedFootprintSide                                          CONSTANT)                                   ///< Side footprint adjusted down for overlap
    Q_PROPERTY(Fact*            adjustedFootprintFrontal    READ adjustedFootprintFrontal                                       CONSTANT)                                   ///< Frontal footprint adjusted down for overlap

    // When we are creating a manual grid we still use CameraCalc to store the manual grid information. It's a bastardization of what
    // CameraCalc is meant for but it greatly simplifies code and persistance of manual grids.
    //  grid altitude -         distanceToSurface
    //  grid altitude mode -    distanceToSurfaceRelative
    //  trigger distance -      adjustedFootprintFrontal
    //  transect spacing -      adjustedFootprintSide
    Q_PROPERTY(bool             distanceToSurfaceRelative   READ distanceToSurfaceRelative WRITE setDistanceToSurfaceRelative   NOTIFY distanceToSurfaceRelativeChanged)

    // The following values are calculated from the camera properties
    Q_PROPERTY(double imageFootprintSide    READ imageFootprintSide     NOTIFY imageFootprintSideChanged)       ///< Size of image size side in meters
    Q_PROPERTY(double imageFootprintFrontal READ imageFootprintFrontal  NOTIFY imageFootprintFrontalChanged)    ///< Size of image size frontal in meters

    static QString xlatCustomCameraName     (void);
    static QString xlatManualCameraName     (void);
    static QString canonicalCustomCameraName(void);
    static QString canonicalManualCameraName(void);

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
    bool    isManualCamera              (void) const { return _cameraNameFact.rawValue().toString() == canonicalManualCameraName(); }
    bool    isCustomCamera              (void) const { return _cameraNameFact.rawValue().toString() == canonicalCustomCameraName(); }
    double  imageFootprintSide          (void) const { return _imageFootprintSide; }
    double  imageFootprintFrontal       (void) const { return _imageFootprintFrontal; }
    bool    distanceToSurfaceRelative   (void) const { return _distanceToSurfaceRelative; }

    void setDirty                       (bool dirty);
    void setDistanceToSurfaceRelative   (bool distanceToSurfaceRelative);
    void setCameraBrand                 (const QString& cameraBrand);
    void setCameraModel                 (const QString& cameraModel);

    void save(QJsonObject& json) const;
    bool load(const QJsonObject& json, QString& errorString);

    void _setCameraNameFromV3TransectLoad   (const QString& cameraName);

    static const char* cameraNameName;
    static const char* valueSetIsDistanceName;
    static const char* distanceToSurfaceName;
    static const char* distanceToSurfaceRelativeName;
    static const char* imageDensityName;
    static const char* frontalOverlapName;
    static const char* sideOverlapName;
    static const char* adjustedFootprintSideName;
    static const char* adjustedFootprintFrontalName;

signals:
    void dirtyChanged                       (bool dirty);
    void imageFootprintSideChanged          (double imageFootprintSide);
    void imageFootprintFrontalChanged       (double imageFootprintFrontal);
    void distanceToSurfaceRelativeChanged   (bool distanceToSurfaceRelative);
    void isManualCameraChanged              (void);
    void isCustomCameraChanged              (void);
    void cameraBrandChanged                 (void);
    void cameraModelChanged                 (void);
    void cameraModelListChanged             (void);
    void updateCameraStats                  (void);

private slots:
    void _recalcTriggerDistance             (void);
    void _adjustDistanceToSurfaceRelative   (void);
    void _setDirty                          (void);
    void _cameraNameChanged                 (void);

private:
    void    _setBrandModelFromCanonicalName (const QString& cameraName);
    void    _rebuildCameraModelList         (void);
    QString _validCanonicalCameraName       (const QString& cameraName);

    bool            _dirty                      = false;
    bool            _disableRecalc              = false;
    QString         _cameraBrand;
    QString         _cameraModel;
    QStringList     _cameraBrandList;
    QStringList     _cameraModelList;
    bool            _distanceToSurfaceRelative  = true;
    double          _imageFootprintSide         = 0;
    double          _imageFootprintFrontal      = 0;
    QVariantList    _knownCameraList;

    QMap<QString, FactMetaData*> _metaDataMap;

    SettingsFact _cameraNameFact;
    SettingsFact _valueSetIsDistanceFact;
    SettingsFact _distanceToSurfaceFact;
    SettingsFact _imageDensityFact;
    SettingsFact _frontalOverlapFact;
    SettingsFact _sideOverlapFact;
    SettingsFact _adjustedFootprintSideFact;
    SettingsFact _adjustedFootprintFrontalFact;

    // The following are deprecated usage and only included in order to convert older formats

    enum CameraSpecType {
        CameraSpecNone,
        CameraSpecCustom,
        CameraSpecKnown
    };

    static const char* _jsonCameraSpecTypeKey;
};
