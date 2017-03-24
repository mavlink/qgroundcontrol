/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#ifndef CameraSection_H
#define CameraSection_H

#include "ComplexMissionItem.h"
#include "MissionItem.h"
#include "Fact.h"

Q_DECLARE_LOGGING_CATEGORY(CameraSectionLog)

class CameraSection : public QObject
{
    Q_OBJECT

public:
    CameraSection(QObject* parent = NULL);

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

    Q_PROPERTY(bool     available                       READ available                      WRITE setAvailable                  NOTIFY availableChanged)
    Q_PROPERTY(bool     settingsSpecified               MEMBER _settingsSpecified                                               NOTIFY settingsSpecifiedChanged)
    Q_PROPERTY(bool     specifyGimbal                   READ specifyGimbal                  WRITE setSpecifyGimbal              NOTIFY specifyGimbalChanged)
    Q_PROPERTY(Fact*    gimbalPitch                     READ gimbalPitch                                                        CONSTANT)
    Q_PROPERTY(Fact*    gimbalYaw                       READ gimbalYaw                                                          CONSTANT)
    Q_PROPERTY(Fact*    cameraAction                    READ cameraAction                                                       CONSTANT)
    Q_PROPERTY(Fact*    cameraPhotoIntervalTime         READ cameraPhotoIntervalTime                                            CONSTANT)
    Q_PROPERTY(Fact*    cameraPhotoIntervalDistance     READ cameraPhotoIntervalDistance                                        CONSTANT)

    bool    available                   (void) const { return _available; }
    void    setAvailable                (bool available);
    bool    specifyGimbal               (void) const { return _specifyGimbal; }
    Fact*   gimbalYaw                   (void) { return &_gimbalYawFact; }
    Fact*   gimbalPitch                 (void) { return &_gimbalPitchFact; }
    Fact*   cameraAction                (void) { return &_cameraActionFact; }
    Fact*   cameraPhotoIntervalTime     (void) { return &_cameraPhotoIntervalTimeFact; }
    Fact*   cameraPhotoIntervalDistance (void) { return &_cameraPhotoIntervalDistanceFact; }

    ///< @return The gimbal yaw specified by this item, NaN if not specified
    double specifiedGimbalYaw(void) const;

    /// Scans the loaded items for the section items
    ///     @param visualItems Item list
    ///     @param scanIndex Index to start scanning from
    /// @return true: camera section found
    bool scanForCameraSection(QmlObjectListModel* visualItems, int scanIndex);

    /// Appends the mission items associated with this section
    ///     @param items List to append to
    ///     @param missionItemParent QObject parent for created MissionItems
    ///     @param nextSequenceNumber Sequence number for first item
    void appendMissionItems(QList<MissionItem*>& items, QObject* missionItemParent, int nextSequenceNumber);

    void setSpecifyGimbal   (bool specifyGimbal);
    bool dirty              (void) const { return _dirty; }
    void setDirty           (bool dirty);

    /// Returns the number of mission items represented by this section.
    ///     Signals: missionItemCountChanged on change
    int missionItemCount(void) const;

signals:
    void availableChanged           (bool available);
    void settingsSpecifiedChanged   (bool settingsSpecified);
    void dirtyChanged               (bool dirty);
    bool specifyGimbalChanged       (bool specifyGimbal);
    void missionItemCountChanged    (int missionItemCount);
    void specifiedGimbalYawChanged  (double gimbalYaw);

private slots:
    void _setDirty(void);
    void _setDirtyAndUpdateMissionItemCount(void);
    void _updateSpecifiedGimbalYaw(void);

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

#endif
