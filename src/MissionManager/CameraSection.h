/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "Section.h"
#include "ComplexMissionItem.h"
#include "MissionItem.h"
#include "Fact.h"

class CameraSection : public Section
{
    Q_OBJECT

public:
    CameraSection(Vehicle* vehicle, QObject* parent = NULL);

    // These nume values must match the json meta data
    enum CameraAction {
        CameraActionNone,
        TakePhotosIntervalTime,
        TakePhotoIntervalDistance,
        StopTakingPhotos,
        TakeVideo,
        StopTakingVideo
    };
    Q_ENUMS(CameraAction)

    Q_PROPERTY(bool     specifyGimbal                   READ specifyGimbal                  WRITE setSpecifyGimbal              NOTIFY specifyGimbalChanged)
    Q_PROPERTY(Fact*    gimbalPitch                     READ gimbalPitch                                                        CONSTANT)
    Q_PROPERTY(Fact*    gimbalYaw                       READ gimbalYaw                                                          CONSTANT)
    Q_PROPERTY(Fact*    cameraAction                    READ cameraAction                                                       CONSTANT)
    Q_PROPERTY(Fact*    cameraPhotoIntervalTime         READ cameraPhotoIntervalTime                                            CONSTANT)
    Q_PROPERTY(Fact*    cameraPhotoIntervalDistance     READ cameraPhotoIntervalDistance                                        CONSTANT)

    bool    specifyGimbal               (void) const { return _specifyGimbal; }
    Fact*   gimbalYaw                   (void) { return &_gimbalYawFact; }
    Fact*   gimbalPitch                 (void) { return &_gimbalPitchFact; }
    Fact*   cameraAction                (void) { return &_cameraActionFact; }
    Fact*   cameraPhotoIntervalTime     (void) { return &_cameraPhotoIntervalTimeFact; }
    Fact*   cameraPhotoIntervalDistance (void) { return &_cameraPhotoIntervalDistanceFact; }

    void setSpecifyGimbal   (bool specifyGimbal);

    ///< @return The gimbal yaw specified by this item, NaN if not specified
    double specifiedGimbalYaw(void) const;

    // Overrides from Section
    bool available          (void) const override { return _available; }
    bool dirty              (void) const override { return _dirty; }
    void setAvailable       (bool available) override;
    void setDirty           (bool dirty) override;
    bool scanForSection     (QmlObjectListModel* visualItems, int scanIndex) override;
    void appendSectionItems (QList<MissionItem*>& items, QObject* missionItemParent, int& seqNum) override;
    int  itemCount          (void) const override;
    bool settingsSpecified  (void) const override {return _settingsSpecified; }

signals:
    bool specifyGimbalChanged       (bool specifyGimbal);
    void specifiedGimbalYawChanged  (double gimbalYaw);

private slots:
    void _setDirty(void);
    void _setDirtyAndUpdateItemCount(void);
    void _updateSpecifiedGimbalYaw(void);
    void _specifyGimbalChanged(bool specifyGimbal);
    void _updateSettingsSpecified(void);
    void _cameraActionChanged(void);

private:
    bool    _available;
    bool    _settingsSpecified;
    bool    _specifyGimbal;
    Fact    _gimbalYawFact;
    Fact    _gimbalPitchFact;
    Fact    _cameraActionFact;
    Fact    _cameraPhotoIntervalDistanceFact;
    Fact    _cameraPhotoIntervalTimeFact;
    bool    _dirty;

    static QMap<QString, FactMetaData*> _metaDataMap;

    static const char* _gimbalPitchName;
    static const char* _gimbalYawName;
    static const char* _cameraActionName;
    static const char* _cameraPhotoIntervalDistanceName;
    static const char* _cameraPhotoIntervalTimeName;
};
