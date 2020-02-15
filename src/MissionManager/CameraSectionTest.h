/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "SectionTest.h"
#include "CameraSection.h"

/// Unit test for CameraSection
class CameraSectionTest : public SectionTest
{
    Q_OBJECT
    
public:
    CameraSectionTest(void);

    void init(void) override;
    void cleanup(void) override;

    static SimpleMissionItem* createValidStopVideoItem   (Vehicle* vehicle, QObject* parent);
    static SimpleMissionItem* createValidStopDistanceItem(Vehicle* vehicle, QObject* parent);
    static SimpleMissionItem* createValidStopTimeItem    (Vehicle* vehicle, QObject* parent);

private slots:
    void _testDirty                                 (void);
    void _testSettingsAvailable                     (void);
    void _checkAvailable                            (void);
    void _testItemCount                             (void);
    void _testAppendSectionItems                    (void);
    void _testScanForGimbalSection                  (void);
    void _testScanForPhotoIntervalTimeSection       (void);
    void _testScanForPhotoIntervalDistanceSection   (void);
    void _testScanForStartVideoSection              (void);
    void _testScanForStopVideoSection               (void);
    void _testScanForStopPhotoSection               (void);
    void _testScanForCameraModeSection              (void);
    void _testScanForTakePhotoSection               (void);
    void _testScanForMultipleItems                  (void);
    void _testSpecifiedGimbalValuesChanged          (void);

private:
    void _createSpy(CameraSection* cameraSection, MultiSignalSpy** cameraSpy);
    void _validateItemScan(SimpleMissionItem* validItem);
    void _resetSection(void);

    enum {
        specifyGimbalChangedIndex = 0,
        specifiedGimbalYawChangedIndex,
        specifiedGimbalPitchChangedIndex,
        specifyCameraModeChangedIndex,
        maxSignalIndex,
    };

    enum {
        specifyGimbalChangedMask =          1 << specifyGimbalChangedIndex,
        specifiedGimbalYawChangedMask =     1 << specifiedGimbalYawChangedIndex,
        specifiedGimbalPitchChangedMask =   1 << specifiedGimbalPitchChangedIndex,
        specifyCameraModeChangedMask =      1 << specifyCameraModeChangedIndex,
    };

    static const size_t cCameraSignals = maxSignalIndex;
    const char*         rgCameraSignals[cCameraSignals];

    MultiSignalSpy*     _spyCamera;
    MultiSignalSpy*     _spySection;
    CameraSection*      _cameraSection;
    SimpleMissionItem*  _validGimbalItem;
    SimpleMissionItem*  _validDistanceItem;
    SimpleMissionItem*  _validTimeItem;
    SimpleMissionItem*  _validStartVideoItem;
    SimpleMissionItem*  _validStopVideoItem;
    SimpleMissionItem*  _validStopDistanceItem;
    SimpleMissionItem*  _validStopTimeItem;
    SimpleMissionItem*  _validCameraPhotoModeItem;
    SimpleMissionItem*  _validCameraVideoModeItem;
    SimpleMissionItem*  _validCameraSurveyPhotoModeItem;
    SimpleMissionItem*  _validTakePhotoItem;
};
