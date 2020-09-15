/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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

#define VIDEO_CAPTURE_STATUS_INTERVAL 0.2   //-- Send capture status every 5 seconds

class PlanMasterController;
class CameraSectionTest;


class CameraSection : public Section
{
    Q_OBJECT

public:
    CameraSection(PlanMasterController* masterController, QObject* parent = nullptr);

    // These enum values must match the json meta data

    enum CameraAction {
        CameraActionNone,
        TakePhotosIntervalTime,
        TakePhotoIntervalDistance,
        StopTakingPhotos,
        TakeVideo,
        StopTakingVideo,
        TakePhoto
    };
    Q_ENUM(CameraAction)

    Q_PROPERTY(bool     specifyGimbal                   READ specifyGimbal                  WRITE setSpecifyGimbal              NOTIFY specifyGimbalChanged)
    Q_PROPERTY(Fact*    gimbalPitch                     READ gimbalPitch                                                        CONSTANT)
    Q_PROPERTY(Fact*    gimbalYaw                       READ gimbalYaw                                                          CONSTANT)
    Q_PROPERTY(Fact*    cameraAction                    READ cameraAction                                                       CONSTANT)
    Q_PROPERTY(Fact*    cameraPhotoIntervalTime         READ cameraPhotoIntervalTime                                            CONSTANT)
    Q_PROPERTY(Fact*    cameraPhotoIntervalDistance     READ cameraPhotoIntervalDistance                                        CONSTANT)
    Q_PROPERTY(bool     cameraModeSupported             READ cameraModeSupported                                                CONSTANT)   ///< true: cameraMode is supported by this vehicle
    Q_PROPERTY(bool     specifyCameraMode               READ specifyCameraMode              WRITE setSpecifyCameraMode          NOTIFY specifyCameraModeChanged)
    Q_PROPERTY(Fact*    cameraMode                      READ cameraMode                                                         CONSTANT)   ///< MAV_CMD_SET_CAMERA_MODE.param2

    bool    specifyGimbal               (void) const { return _specifyGimbal; }
    Fact*   gimbalYaw                   (void) { return &_gimbalYawFact; }
    Fact*   gimbalPitch                 (void) { return &_gimbalPitchFact; }
    Fact*   cameraAction                (void) { return &_cameraActionFact; }
    Fact*   cameraPhotoIntervalTime     (void) { return &_cameraPhotoIntervalTimeFact; }
    Fact*   cameraPhotoIntervalDistance (void) { return &_cameraPhotoIntervalDistanceFact; }
    bool    cameraModeSupported         (void) const;
    bool    specifyCameraMode           (void) const { return _specifyCameraMode; }
    Fact*   cameraMode                  (void) { return &_cameraModeFact; }

    void setSpecifyGimbal       (bool specifyGimbal);
    void setSpecifyCameraMode   (bool specifyCameraMode);

    ///< Signals specifiedGimbalYawChanged
    ///< @return The gimbal yaw specified by this item, NaN if not specified
    double specifiedGimbalYaw(void) const;

    ///< Signals specifiedGimbalPitchChanged
    ///< @return The gimbal pitch specified by this item, NaN if not specified
    double specifiedGimbalPitch(void) const;

    static bool scanStopTakingPhotos(QmlObjectListModel* visualItems, int scanIndex, bool removeScannedItems);
    static bool scanStopTakingVideo(QmlObjectListModel* visualItems, int scanIndex, bool removeScannedItems);
    static void appendStopTakingPhotos(QList<MissionItem*>& items, int& seqNum, QObject* missionItemParent);
    static void appendStopTakingVideo(QList<MissionItem*>& items, int& seqNum, QObject* missionItemParent);
    static int  stopTakingPhotosCommandCount(void) { return 2; }
    static int  stopTakingVideoCommandCount(void) { return 1; }

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
    bool specifyCameraModeChanged   (bool specifyCameraMode);
    void specifiedGimbalYawChanged  (double gimbalYaw);
    void specifiedGimbalPitchChanged(double gimbalPitch);

private slots:
    void _setDirty(void);
    void _setDirtyAndUpdateItemCount(void);
    void _updateSpecifiedGimbalYaw(void);
    void _updateSpecifiedGimbalPitch(void);
    void _specifyChanged(void);
    void _updateSettingsSpecified(void);
    void _cameraActionChanged(void);
    void _dirtyIfSpecified(void);

private:
    bool _scanGimbal(QmlObjectListModel* visualItems, int scanIndex);
    bool _scanTakePhoto(QmlObjectListModel* visualItems, int scanIndex);
    bool _scanTakePhotosIntervalTime(QmlObjectListModel* visualItems, int scanIndex);
    bool _scanTriggerStartDistance(QmlObjectListModel* visualItems, int scanIndex);
    bool _scanTriggerStopDistance(QmlObjectListModel* visualItems, int scanIndex);
    bool _scanTakeVideo(QmlObjectListModel* visualItems, int scanIndex);
    bool _scanSetCameraMode(QmlObjectListModel* visualItems, int scanIndex);

    bool    _available;
    bool    _settingsSpecified;
    bool    _specifyGimbal;
    bool    _specifyCameraMode;
    Fact    _gimbalYawFact;
    Fact    _gimbalPitchFact;
    Fact    _cameraActionFact;
    Fact    _cameraPhotoIntervalDistanceFact;
    Fact    _cameraPhotoIntervalTimeFact;
    Fact    _cameraModeFact;
    bool    _dirty;

    static QMap<QString, FactMetaData*> _metaDataMap;

    static const char* _gimbalPitchName;
    static const char* _gimbalYawName;
    static const char* _cameraActionName;
    static const char* _cameraPhotoIntervalDistanceName;
    static const char* _cameraPhotoIntervalTimeName;
    static const char* _cameraModeName;

    friend CameraSectionTest;
};
