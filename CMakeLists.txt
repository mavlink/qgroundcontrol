cmake_minimum_required (VERSION 2.6)

project (qgroundcontrol)

# marcos
macro(find_or_build_from_source PACKAGE PACKAGE_PATH IS_GIT_SUBMODULE)
    add_custom_target(${PACKAGE})
    if (NOT ${PACKAGE}_BUILD_FROM_SOURCE)
        find_package(${PACKAGE})
    endif()
    if (NOT ${PACKAGE}_FOUND)
        set(${PACKAGE}_BUILD_FROM_SOURCE TRUE)
        message(STATUS "could not find package ${PACKAGE}, building from source")
        add_custom_target(${PACKAGE}_BUILD DEPENDS ${PACKAGE}_BUILD.stamp)
        add_dependencies(${PACKAGE} ${PACKAGE}_BUILD)
        set(${PACKAGE}_FOUND TRUE)
        if (${IS_GIT_SUBMODULE})
            message(STATUS "${PACKAGE} detected as git submodule, will attempt to initialize it")
            list(APPEND GIT_SUBMODULES ${PACKAGE_PATH})
            add_dependencies(${PACKAGE}_BUILD GIT)
        endif()
    endif()
endmacro(find_or_build_from_source)

macro(set_default VAR DEFAULT)
    if (NOT DEFINED ${VAR})
        set(${VAR} ${DEFAULT})
    endif()
endmacro(set_default)

# check for out of source build
if("${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}")
    message(FATAL_ERROR "In-source builds are not allowed. For example run:
    rm CMakeCache.txt
    mkdir build
    cd build
    cmake ..
    make")
endif()

# settings
set(qgroundcontrol_VERSION_MAJOR 0)
set(qgroundcontrol_VERSION_MINOR 2)
set(qgroundcontrol_VERSION_PATCH 0)
set(qgroundcontrol_SOVERSION 0)

set_default(MAVLINK_BUILD_FROM_SOURCE FALSE)
set_default(STATIC_LINKING FALSE)
set_default(IN_SRC_BUILD FALSE)

# built variables
set(qgroundcontrol_VERSION ${qgroundcontrol_VERSION_MAJOR}.${qgroundcontrol_VERSION_MINOR}.${qgroundcontrol_VERSION_PATCH})
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMakeModules;${CMAKE_MODULE_PATH})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

# only find static libraries
if(STATIC_LINKING)
    message(WARNING "static linking is not yet fully functional and will have linking errors")
    if(WIN32)
        set(CMAKE_FIND_LIBRARY_SUFFIXES .lib .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
    else(WIN32)
        set(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
    endif(WIN32)
endif()

# set build type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
endif(NOT CMAKE_BUILD_TYPE)

# enable languages
enable_language(C)
enable_language(CXX)

# initialize variables
set(qgroundcontrol_LIBRARIES qgroundcontrolNavigation qgroundcontrolCommunication)
set(SCICOSLAB_BLOCKS stdBlocks;jsbsimBlocks;mavlinkBlocks)

# installer
include(InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${qgroundcontrol_VERSION}")
set(CPACK_GENERATOR "DEB")
set(CPACK_SOURCE_GENERATOR "TGZ;ZIP")
set(CPACK_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
set(CPACK_SET_DESTDIR TRUE)
set(CPACK_PACKAGE_CONTACT "James Goppert james.goppert@gmail.com")
set(CPACK_PACKAGE_DESCRITION_SUMMARY "
    QGroundControl

    A qt based ground-control program for unmanned systems.
    ")
set(CPACK_SOURCE_IGNORE_FILES ${CPACK_SOURCE_IGNORE_FILES}
    /.git/;/build/;~$;.*\\\\.bin$;.*\\\\.swp$)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/license.txt")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README")
set(CPACK_PACKAGE_VERSION_MAJOR ${qgroundcontrol_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${qgroundcontrol_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${qgroundcontrol_VERSION_PATCH})
include(CPack)

# add make dist target
add_custom_target(dist COMMAND ${CMAKE_MAKE_PROGRAM} package_source)

# git submodules
if(IS_DIRECTORY ${PROJECT_SOURCE_DIR}/.git)
    message(STATUS "git repository detected, will attempt to load submodules")
    set(FOUND_GIT_REPO TRUE)
    add_custom_command(OUTPUT GIT.stamp 
        COMMAND cd ${PROJECT_SOURCE_DIR} && git submodule init ${GIT_SUBMODULES}
        COMMAND cd ${PROJECT_SOURCE_DIR} && git submodule update ${GIT_SUBMODULES})
    add_custom_target(GIT DEPENDS GIT.stamp)
else()
    set(FOUND_GIT_REPO FALSE)
endif()

# find libraries with cmake modules
find_package(Qt4 COMPONENTS QtGui QtCore QtNetwork QtOpenGL QtSVG QtXML QtPhonon QtWebKit REQUIRED)
set(PHONON_FIND_QUIETLY FALSE)
find_packagE(Phonon)
find_package(SDL)
find_package(OpenGL)
find_package(OpenSceneGraph 2.8.3 COMPONENTS osgGA osgDB osgUtil osgViewer)
find_or_build_from_source(MAVLINK thirdParty/mavlink FOUND_GIT_REPO)

# build libraries from source if not found on system
if(MAVLINK_BUILD_FROM_SOURCE)
    set(MAVLINK_INCLUDE_DIRS
        ${PROJECT_SOURCE_DIR}/thirdParty/mavlink/include
        ${PROJECT_SOURCE_DIR}/thirdParty/mavlink/include/common
        ${PROJECT_SOURCE_DIR}/thirdParty/mavlink/include/pixhawk
        ${PROJECT_SOURCE_DIR}/thirdParty/mavlink/include/slugs
        ${PROJECT_SOURCE_DIR}/thirdParty/mavlink/include/ualberta
        ${PROJECT_SOURCE_DIR}/thirdParty/mavlink/include/ardupilotmega
        )

    add_custom_command(OUTPUT MAVLINK_BUILD.stamp 
        COMMAND touch MAVLINK_BUILD.stamp)
endif()

# data directory
if(IN_SRC_BUILD)
    message(STATUS "configuring for in source build")
    set(DATADIR ${PROJECT_SOURCE_DIR}/data)
    set(LIBDIR ${CMAKE_SOURCE_DIR}/data)
    set(BINDIR ${CMAKE_BINARY_DIR}/bin)
else()
    message(STATUS "configuring for install build")
    set(DATADIR ${CMAKE_INSTALL_PREFIX}/share/qgroundcontrol/data)
    set(LIBDIR ${CMAKE_INSTALL_PREFIX}/lib)
    set(BINDIR ${CMAKE_INSTALL_PREFIX}/bin)
endif()

# install data files
install(DIRECTORY "${PROJECT_SOURCE_DIR}/data"
    DESTINATION share/qgroundcontrol
    PATTERN "*.git*" EXCLUDE)

# pkgconfig
install(FILES
    qgroundcontrol.pc
    DESTINATION lib/pkgconfig
    )

# internal library list definition / dep summary
message(STATUS "=======================================")
message(STATUS "\tLIBRARY\t\t\tBUILDING")
message(STATUS "=======================================")
message(STATUS "\t\tMavlink\t\tYES")
if (OPENSCENEGRAPH_FOUND)
    list(APPEND qgroundcontrol_LIBRARIES qgroundcontrolVisualization)
    message(STATUS "\t\tOpenSceneGraph\tYES")
else()
    message(STATUS "\t\tOpenSceneGraph\t\tNO")
endif (OPENSCENEGRAPH_FOUND)
if (QT4_FOUND)
    message(STATUS "\t\tQT4\t\tYES")
else()
    message(STATUS "\t\tQT4\t\tNO")
endif (QT4_FOUND)
if (PHONON_FOUND)
    message(STATUS "\t\tPHONON\t\tYES")
else()
    message(STATUS "\t\tPHONON\t\tNO")
endif (PHONON_FOUND)
message(${PHONON_INCLUDE_DIR})

message(STATUS "=======================================")

# project wide flags
include (${QT_USE_FILE})
include_directories(
    src
    src/ui
    src/ui/linechart
    src/ui/uas
    src/ui/map
    src/ui/map3D
    src/uas
    src/comm
    include/ui
    src/input
    src/lib/qmapcontrol
    src/ui/mavlink
    src/ui/param
    src/ui/watchdog
    src/ui/map3D
    src/ui/designer
    src/lib/qextserialport
    src/lib/qwt
    lib/QMapControl
    ${SDL_INCLUDE_DIR}
    ${OPENGL_INCLUDE_DIR}
    ${PROJECT_BINARY_DIR}
    ${OPENSCENEGRAPH_INCLUDE_DIRS}
    ${QT_INCLUDE_DIRS}
    ${PHONON_INCLUDE_DIR}/phonon
    ${MAVLINK_INCLUDE_DIRS}
    )
link_libraries(
    ${OPENGL_LIBRARIES}
    ${OSG_LIBRARIES}
    ${QT_LIBRARIES}
    ${SDL_LIBRARY}
    ${PHONON_LIBS}
    )
add_definitions(-D_TTY_POSIX_)

set(qgroundcontrolUiSrc
    src/ui/MainWindow.ui
    src/ui/CommSettings.ui
    src/ui/SerialSettings.ui
    src/ui/UASControl.ui
    src/ui/UASList.ui
    src/ui/UASInfo.ui
    src/ui/Linechart.ui
    src/ui/UASView.ui
    src/ui/ParameterInterface.ui
    src/ui/WaypointList.ui
    src/ui/WaypointView.ui
    src/ui/ObjectDetectionView.ui
    src/ui/JoystickWidget.ui
    src/ui/DebugConsole.ui
    src/ui/MapWidget.ui
    src/ui/XMLCommProtocolWidget.ui
    src/ui/HDDisplay.ui
    src/ui/MAVLinkSettingsWidget.ui
    src/ui/AudioOutputWidget.ui
    src/ui/QGCSensorSettingsWidget.ui
    src/ui/watchdog/WatchdogControl.ui
    src/ui/watchdog/WatchdogProcessView.ui
    src/ui/watchdog/WatchdogView.ui
    src/ui/QGCFirmwareUpdate.ui
    src/ui/QGCPxImuFirmwareUpdate.ui
    src/ui/QGCDataPlot2D.ui
    src/ui/QGCRemoteControlView.ui
    src/ui/QMap3D.ui
    src/ui/QGCWebView.ui
    src/ui/map3D/QGCGoogleEarthView.ui
    src/ui/SlugsDataSensorView.ui
    src/ui/SlugsHilSim.ui
    src/ui/SlugsPIDControl.ui
    src/ui/SlugsVideoCamControl.ui
    src/ui/SlugsPadCameraControl.ui
    src/ui/uas/QGCUnconnectedInfoWidget.ui
    src/ui/designer/QGCToolWidget.ui
    src/ui/designer/QGCParamSlider.ui
    src/ui/designer/QGCActionButton.ui
    src/ui/QGCMAVLinkLogPlayer.ui
    src/ui/QGCWaypointListMulti.ui
    src/ui/mission/QGCCustomWaypointAction.ui
    src/ui/QGCUDPLinkConfiguration.ui
    src/ui/QGCSettingsWidget.ui
    )

set(qgroundcontrolHdrs
    src/MG.h
    src/Core.h
    src/uas/UASInterface.h
    src/uas/UAS.h
    src/uas/UASManager.h
    src/comm/LinkManager.h
    src/comm/LinkInterface.h
    src/comm/SerialLinkInterface.h
    src/comm/SerialLink.h
    src/comm/SerialSimulationLink.h
    src/comm/ProtocolInterface.h
    src/comm/MAVLinkProtocol.h
    src/comm/AS4Protocol.h
    src/ui/CommConfigurationWindow.h
    src/ui/SerialConfigurationWindow.h
    src/ui/MainWindow.h
    src/ui/uas/UASControlWidget.h
    src/ui/uas/UASListWidget.h
    src/ui/uas/UASInfoWidget.h
    src/ui/HUD.h
    src/ui/linechart/LinechartWidget.h
    src/ui/linechart/LinechartPlot.h
    src/ui/linechart/Scrollbar.h
    src/ui/linechart/ScrollZoomer.h
    src/configuration.h
    src/ui/uas/UASView.h
    src/ui/CameraView.h
    src/comm/MAVLinkSimulationLink.h
    src/comm/UDPLink.h
    src/ui/ParameterInterface.h
    src/ui/WaypointList.h
    src/Waypoint.h
    src/ui/WaypointView.h
    src/ui/ObjectDetectionView.h
    src/input/JoystickInput.h
    src/ui/JoystickWidget.h
    src/ui/DebugConsole.h
    src/ui/MapWidget.h
    src/ui/XMLCommProtocolWidget.h
    src/ui/mavlink/DomItem.h
    src/ui/mavlink/DomModel.h
    src/comm/MAVLinkXMLParser.h
    src/ui/HDDisplay.h
    src/ui/MAVLinkSettingsWidget.h
    src/ui/AudioOutputWidget.h
    src/GAudioOutput.h
    src/LogCompressor.h
    src/ui/QGCParamWidget.h
    src/ui/QGCSensorSettingsWidget.h
    src/ui/linechart/Linecharts.h
    src/uas/SlugsMAV.h
    src/uas/PxQuadMAV.h
    src/uas/ArduPilotMegaMAV.h
    src/comm/MAVLinkSyntaxHighlighter.h
    src/ui/watchdog/WatchdogControl.h
    src/ui/watchdog/WatchdogProcessView.h
    src/ui/watchdog/WatchdogView.h
    src/uas/UASWaypointManager.h
    src/ui/HSIDisplay.h
    src/QGC.h
    src/ui/QGCFirmwareUpdate.h
    src/ui/QGCPxImuFirmwareUpdate.h
    src/ui/QGCDataPlot2D.h
    src/ui/linechart/IncrementalPlot.h
    src/ui/map/Waypoint2DIcon.h
    src/ui/map/MAV2DIcon.h
    src/ui/QGCRemoteControlView.h
    src/ui/RadioCalibration/RadioCalibrationData.h
    src/ui/RadioCalibration/RadioCalibrationWindow.h
    src/ui/RadioCalibration/AirfoilServoCalibrator.h
    src/ui/RadioCalibration/SwitchCalibrator.h
    src/ui/RadioCalibration/CurveCalibrator.h
    src/ui/RadioCalibration/AbstractCalibrator.h
    src/comm/QGCMAVLink.h
    src/ui/QGCWebView.h
    src/ui/map3D/QGCWebPage.h
    src/ui/SlugsDataSensorView.h
    src/ui/SlugsHilSim.h
    src/ui/SlugsPIDControl.h
    src/ui/SlugsVideoCamControl.h
    src/ui/SlugsPadCameraControl.h
    src/ui/QGCMainWindowAPConfigurator.h
    src/comm/MAVLinkSwarmSimulationLink.h
    src/ui/uas/QGCUnconnectedInfoWidget.h
    src/ui/designer/QGCToolWidget.h
    src/ui/designer/QGCParamSlider.h
    src/ui/designer/QGCActionButton.h
    src/ui/designer/QGCToolWidgetItem.h
    src/ui/QGCMAVLinkLogPlayer.h
    src/comm/MAVLinkSimulationWaypointPlanner.h
    src/comm/MAVLinkSimulationMAV.h
    src/uas/QGCMAVLinkUASFactory.h
    src/ui/QGCWaypointListMulti.h
    src/ui/QGCUDPLinkConfiguration.h
    src/ui/QGCSettingsWidget.h
    )

set (qgroundcontrolMocSrc
    src/ui/AudioOutputWidget.cc
    src/ui/CameraView.cc
    src/ui/CommConfigurationWindow.cc
    src/ui/DebugConsole.cc
    src/ui/HDDisplay.cc
    src/ui/HSIDisplay.cc
    src/ui/HUD.cc
    src/ui/JoystickWidget.cc
    src/ui/MAVLinkSettingsWidget.cc
    src/ui/MainWindow.cc
    src/ui/MapWidget.cc
    src/ui/ObjectDetectionView.cc
    src/ui/ParameterInterface.cc
    src/ui/QGCDataPlot2D.cc
    src/ui/QGCFirmwareUpdate.cc
    src/ui/QGCMAVLinkLogPlayer.cc
    src/ui/QGCMainWindowAPConfigurator.cc
    src/ui/QGCParamWidget.cc
    src/ui/QGCPxImuFirmwareUpdate.cc
    src/ui/QGCRemoteControlView.cc
    src/ui/QGCSensorSettingsWidget.cc
    src/ui/QGCSettingsWidget.cc
    src/ui/QGCUDPLinkConfiguration.cc
    src/ui/QGCWaypointListMulti.cc
    src/ui/QGCWebView.cc
    src/ui/RadioCalibration/AbstractCalibrator.cc
    src/ui/RadioCalibration/AirfoilServoCalibrator.cc
    src/ui/RadioCalibration/CurveCalibrator.cc
    src/ui/RadioCalibration/RadioCalibrationData.cc
    src/ui/RadioCalibration/RadioCalibrationWindow.cc
    src/ui/RadioCalibration/SwitchCalibrator.cc
    src/ui/SerialConfigurationWindow.cc
    src/ui/SlugsDataSensorView.cc
    src/ui/SlugsHilSim.cc
    src/ui/SlugsPIDControl.cpp
    src/ui/SlugsPadCameraControl.cpp
    src/ui/SlugsVideoCamControl.cpp
    src/ui/WaypointList.cc
    src/ui/WaypointView.cc
    src/ui/XMLCommProtocolWidget.cc
    src/ui/designer/QGCActionButton.cc
    src/ui/designer/QGCParamSlider.cc
    src/ui/designer/QGCToolWidget.cc
    src/ui/designer/QGCToolWidgetItem.cc
    src/ui/linechart/IncrementalPlot.cc
    src/ui/linechart/LinechartPlot.cc
    src/ui/linechart/LinechartWidget.cc
    src/ui/linechart/Linecharts.cc
    src/ui/linechart/ScrollZoomer.cc
    src/ui/linechart/Scrollbar.cc
    src/ui/map/MAV2DIcon.cc
    src/ui/map/Waypoint2DIcon.cc
    src/ui/map3D/QGCWebPage.cc
    src/ui/mavlink/DomItem.cc
    src/ui/mavlink/DomModel.cc
    src/ui/uas/QGCUnconnectedInfoWidget.cc
    src/ui/uas/UASControlWidget.cc
    src/ui/uas/UASInfoWidget.cc
    src/ui/uas/UASListWidget.cc
    src/ui/uas/UASView.cc
    src/ui/watchdog/WatchdogControl.cc
    src/ui/watchdog/WatchdogProcessView.cc
    src/ui/watchdog/WatchdogView.cc
    )

set (qgroundcontrolSrc
    src/main.cc
    src/Core.cc
    src/GAudioOutput.cc
    src/LogCompressor.cc
    src/QGC.cc
    src/Waypoint.cc
    src/comm/AS4Protocol.cc
    src/comm/LinkManager.cc
    src/comm/MAVLinkProtocol.cc
    src/comm/MAVLinkSimulationLink.cc
    src/comm/MAVLinkSimulationMAV.cc
    src/comm/MAVLinkSimulationWaypointPlanner.cc
    src/comm/MAVLinkSwarmSimulationLink.cc
    src/comm/MAVLinkSyntaxHighlighter.cc
    src/comm/MAVLinkXMLParser.cc
    src/comm/SerialLink.cc
    src/comm/SerialSimulationLink.cc
    src/comm/UDPLink.cc
    src/input/JoystickInput.cc
    src/uas/ArduPilotMegaMAV.cc
    src/uas/PxQuadMAV.cc
    src/uas/QGCMAVLinkUASFactory.cc
    src/uas/SlugsMAV.cc
    src/uas/UAS.cc
    src/uas/UASManager.cc
    src/uas/UASWaypointManager.cc
    )

# process qt files
qt4_wrap_cpp(qgroundcontrolMoc ${qgroundcontrolMocSrc})
qt4_wrap_ui(qgroundcontrolUi ${qgroundcontrolUiSrc})
qt4_add_resources(qgroundcontrolRsc ${qgroundcontrolRscSrc})

#add executable
add_executable(qgroundcontrol
    ${qgroundcontrolSrc}
    ${qgroundcontrolMoc}
    ${qgroundcontrolUi}
    ${qgroundcontrolRsc}
    )
add_dependencies(qgroundcontrol MAVLINK)
install(TARGETS qgroundcontrol DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

# vim:ts=4:sw=4:expandtab
