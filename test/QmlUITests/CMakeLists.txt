# ============================================================================
# QML UI Smoke Tests
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/QmlUITestBase.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/QmlUITestBase.h
        ${CMAKE_CURRENT_SOURCE_DIR}/VehicleConfigUITestBase.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/VehicleConfigUITestBase.h
        ${CMAKE_CURRENT_SOURCE_DIR}/TopLevelViewsTest.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/TopLevelViewsTest.h
        ${CMAKE_CURRENT_SOURCE_DIR}/PX4VehicleConfigUITest.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/PX4VehicleConfigUITest.h
        ${CMAKE_CURRENT_SOURCE_DIR}/APMVehicleConfigUITest.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/APMVehicleConfigUITest.h
        ${CMAKE_CURRENT_SOURCE_DIR}/ToolbarIndicatorUITest.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/ToolbarIndicatorUITest.h
        ${CMAKE_CURRENT_SOURCE_DIR}/PlanViewUITest.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/PlanViewUITest.h
        ${CMAKE_CURRENT_SOURCE_DIR}/PlanViewLayerUITest.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/PlanViewLayerUITest.h
        ${CMAKE_CURRENT_SOURCE_DIR}/PX4SensorsCalibrationUITest.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/PX4SensorsCalibrationUITest.h
        ${CMAKE_CURRENT_SOURCE_DIR}/APMSensorsCalibrationUITest.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/APMSensorsCalibrationUITest.h
        ${CMAKE_CURRENT_SOURCE_DIR}/APMFreshFlashUITest.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/APMFreshFlashUITest.h
        ${CMAKE_CURRENT_SOURCE_DIR}/PX4AirframeSetupUITest.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/PX4AirframeSetupUITest.h
        ${CMAKE_CURRENT_SOURCE_DIR}/AppCloseWarningUITest.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/AppCloseWarningUITest.h
        ${CMAKE_CURRENT_SOURCE_DIR}/FlyViewProximityRadarUITest.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/FlyViewProximityRadarUITest.h
        ${CMAKE_CURRENT_SOURCE_DIR}/FlyViewROIUITest.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/FlyViewROIUITest.h
        ${CMAKE_CURRENT_SOURCE_DIR}/FlyViewGeoUITest.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/FlyViewGeoUITest.h
        # Manual perf benchmark: standalone-registered, deliberately NOT added
        # via add_qgc_test so no ctest/CI sweep ever runs it
        ${CMAKE_CURRENT_SOURCE_DIR}/GeoMapPerfUITest.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/GeoMapPerfUITest.h
        ${CMAKE_CURRENT_SOURCE_DIR}/NTRIPSettingsUITest.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/NTRIPSettingsUITest.h
        ${CMAKE_CURRENT_SOURCE_DIR}/OnboardLogUITest.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/OnboardLogUITest.h
        ${CMAKE_CURRENT_SOURCE_DIR}/ParameterDiffDialogUITest.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/ParameterDiffDialogUITest.h
        ${CMAKE_CURRENT_SOURCE_DIR}/RemoteIDSettingsUITest.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/RemoteIDSettingsUITest.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Viewer3DUITest.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/Viewer3DUITest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(TopLevelViewsTest LABELS Integration NoSanitizer TIMEOUT 120)
add_qgc_test(PX4VehicleConfigUITest LABELS Integration NoSanitizer TIMEOUT 180)
add_qgc_test(APMVehicleConfigUITest LABELS Integration NoSanitizer TIMEOUT 360)
add_qgc_test(ToolbarIndicatorUITest LABELS Integration NoSanitizer TIMEOUT 180)
add_qgc_test(PlanViewUITest LABELS Integration NoSanitizer TIMEOUT 120)
# StockUI: asserts stock UI elements that custom builds intentionally replace/omit
add_qgc_test(PlanViewLayerUITest LABELS Integration NoSanitizer StockUI TIMEOUT 120)
add_qgc_test(PX4SensorsCalibrationUITest LABELS Integration NoSanitizer TIMEOUT 180)
add_qgc_test(APMSensorsCalibrationUITest LABELS Integration NoSanitizer TIMEOUT 240)
add_qgc_test(APMFreshFlashUITest LABELS Integration NoSanitizer TIMEOUT 180)
add_qgc_test(PX4AirframeSetupUITest LABELS Integration NoSanitizer StockUI TIMEOUT 120)
add_qgc_test(AppCloseWarningUITest LABELS Integration NoSanitizer TIMEOUT 120)
add_qgc_test(FlyViewProximityRadarUITest LABELS Integration NoSanitizer TIMEOUT 120)
add_qgc_test(FlyViewROIUITest LABELS Integration NoSanitizer TIMEOUT 120)
add_qgc_test(FlyViewROIAPMUITest LABELS Integration NoSanitizer TIMEOUT 120)
add_qgc_test(NTRIPSettingsUITest LABELS Integration NoSanitizer TIMEOUT 120)
add_qgc_test(OnboardLogUITest LABELS Integration NoSanitizer TIMEOUT 180)
add_qgc_test(ParameterDiffDialogUITest LABELS Integration NoSanitizer TIMEOUT 180)
add_qgc_test(RemoteIDSettingsUITest LABELS Integration NoSanitizer TIMEOUT 120)

# Qt Quick 3D requires an RHI backend, which the default software Quick backend
# cannot provide. On Linux the offscreen platform exposes GLX (CI runs ctest
# under xvfb-run with Mesa llvmpipe), so drop the forced software backend and
# request RHI OpenGL — View3D then genuinely renders. On other hosts (e.g.
# macOS offscreen) the software backend stays and the tests assert the
# documented View3D fallback warnings instead.
set(_viewer3d_env_mod "")
if(LINUX)
    set(_viewer3d_env_mod ENV_MODIFICATION
        "QT_QUICK_BACKEND=unset:"
        "QSG_RHI_BACKEND=set:opengl"
    )
endif()
add_qgc_test(Viewer3DUITest LABELS Integration NoSanitizer StockUI TIMEOUT 120 ${_viewer3d_env_mod})
add_qgc_test(FlyViewGeoUITest LABELS Integration NoSanitizer TIMEOUT 120 ${_viewer3d_env_mod})
