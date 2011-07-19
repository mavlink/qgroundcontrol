cmake_minimum_required (VERSION 2.6)

project (qgroundcontrol)

# marcos
macro(find_or_build_from_source PACKAGE PACKAGE_PATH)
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
set(qgroundcontrol_VERSION_MINOR 8)
set(qgroundcontrol_VERSION_PATCH 3)
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
    message(WARNING "static linking is not yet tested and may have linking errors")
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

# installer
include(InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}-${qgroundcontrol_VERSION}-cpack")
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${qgroundcontrol_VERSION}-cpack")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${PROJECT_NAME}-${qgroundcontrol_VERSION}")
set(CPACK_GENERATOR "DEB")
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
set(CPACK_SET_DESTDIR TRUE)
set(CPACK_PACKAGE_CONTACT "James Goppert james.goppert@gmail.com")
set(CPACK_PACKAGE_DESCRITION_SUMMARY "
    QGroundControl

    A qt based ground-control program for unmanned systems.
    ")
set(CPACK_SOURCE_IGNORE_FILES ${CPACK_SOURCE_IGNORE_FILES}
    "/.git/";"/build/";"~$";".*\\\\.bin$";".*\\\\.swp$"
    )
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/license.txt")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README")
set(CPACK_PACKAGE_VERSION_MAJOR ${qgroundcontrol_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${qgroundcontrol_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${qgroundcontrol_VERSION_PATCH})
include(CPack)

# add make dist target
add_custom_target(dist COMMAND ${CMAKE_MAKE_PROGRAM} package_source)

# find libraries with cmake modules
find_package(Qt4 COMPONENTS QtGui QtCore QtNetwork QtOpenGL QtSVG QtXML QtPhonon QtWebKit REQUIRED)
set(PHONON_FIND_QUIETLY FALSE)
find_package(Phonon)
find_package(SDL REQUIRED)
if (UNIX)
    find_package(Flite)
endif(UNIX)
find_package(OpenGL REQUIRED)
find_package(OpenSceneGraph 2.9.9 COMPONENTS osgGA osgDB osgUtil osgViewer)
if ("OSG_LIBRARY-NOTFOUND" STREQUAL "${OSG_LIBRARY}")
    set(OPENSCENEGRAPH_FOUND FALSE)
else()
    set(OPENSCENEGRAPH_FOUND TRUE)
endif()

find_or_build_from_source(MAVLINK thirdParty/mavlink)
find_or_build_from_source(QSERIALPORT thirdParty/qserialport)

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

if(QSERIALPORT_BUILD_FROM_SOURCE)
    # qserialport headers without Q_OBJECT
    # r !grep -RL Q_OBJECT thirdParty/qserialport/include
    set (qserialportHdrs
        thirdParty/qserialport/include/QtSerialPort/qserialport_export.h
        thirdParty/qserialport/include/QtSerialPort/qserialport
        thirdParty/qserialport/include/QtSerialPort/qportsettings.h
        )
    # qserialport headers with Q_OBJECT
    # r !grep -Rl Q_OBJECT thirdParty/qserialport
    set (qserialportMocSrc
        thirdParty/qserialport/include/QtSerialPort/qserialport.h
        thirdParty/qserialport/include/QtSerialPort/qserialportnative.h
        )
    # qserialport src
    set (qserialportSrc
        thirdParty/qserialport/src/common/qserialport.cpp
        thirdParty/qserialport/src/common/qportsettings.cpp
    )

    # qserialport native code
    if (WIN32)
        list(APPEND qserialportHdrs
            thirdParty/qserialport/src/win32/commdcbhelper.h
            )
        list(APPEND qserialportMocSrc
            thirdParty/qserialport/src/win32/qwincommevtnotifier.h
            thirdParty/qserialport/src/win32/wincommevtbreaker.h
        )
        list(APPEND qserialportSrc
            thirdParty/qserialport/src/win32/qserialportnative_win32.cpp
            thirdParty/qserialport/src/win32/commdcbhelper.cpp
            thirdParty/qserialport/src/win32/qwincommevtnotifier.cpp
            #thirdParty/qserialport/src/win32/qserialportnative_wince.cpp
            #thirdParty/qserialport/src/win32/wincommevtbreaker.cpp
            )
    elseif(UNIX OR APPLE)
        list(APPEND qserialportHdrs
            thirdParty/qserialport/src/posix/termioshelper.h
            )
        list(APPEND qserialportSrc    
            thirdParty/qserialport/src/posix/termioshelper.cpp
            thirdParty/qserialport/src/posix/qserialportnative_posix.cpp
            )
    else()
        message(FATAL_ERROR "unknown OS")
    endif()

    # qserialport linking
    qt4_wrap_cpp(qserialportMoc ${qserialportMocSrc})
    add_library(qserialport ${qserialportMoc} ${qserialportSrc})
    target_link_libraries(qserialport ${QT_LIBRARIES})

    set(QSERIALPORT_INCLUDE_DIRS
        ${PROJECT_SOURCE_DIR}/thirdParty/qserialport/include
        ${PROJECT_SOURCE_DIR}/thirdParty/qserialport/include/QtSerialPort
        ${PROJECT_SOURCE_DIR}/thirdParty/qserialport/src
        )
    set(QSERIALPORT_LIBRARIES qserialport)
    add_custom_command(OUTPUT qserialport_BUILD.stamp 
        COMMAND touch qserialport_BUILD.stamp)
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

# include directories
set(qgroundcontrolIncludes
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
    ${PROJECT_BINARY_DIR}
    )
set (qgroundcontrolLibs)

# dependency summary
message(STATUS "=======================================")
message(STATUS "\tDEPENDENCY\t\tFOUND")
message(STATUS "=======================================")

# common dependencies
message(STATUS "\t\tMAVLINK\t\t${MAVLINK_FOUND}")
if (MAVLINK_FOUND)
    list(APPEND qgroundcontrolIncludes ${MAVLINK_INCLUDE_DIRS})
endif()

message(STATUS "\t\tqserialport\t${QSERIALPORT_FOUND}")
if (QSERIALPORT_FOUND)
    list(APPEND qgroundcontrolIncludes ${QSERIALPORT_INCLUDE_DIRS})
    list(APPEND qgroundcontrolLibs ${QSERIALPORT_LIBRARIES})
endif()

message(STATUS "\t\tOpenSceneGraph\t${OPENSCENEGRAPH_FOUND}")
if (OPENSCENEGRAPH_FOUND)
    list(APPEND qgroundcontrolIncludes ${OPENSCENEGRAPH_INCLUDE_DIRS})
    list(APPEND qgroundcontrolLibs ${OPENSCENEGRAPH_LIBRARIES})
endif()

message(STATUS "\t\tQT4\t\t${QT4_FOUND}")
if (QT4_FOUND)
    list(APPEND qgroundcontrolIncludes ${QT_INCLUDE_DIRS})
    list(APPEND qgroundcontrolLibs ${QT_LIBRARIES})
endif()

message(STATUS "\t\tPHONON\t\t${PHONON_FOUND}")
if (PHONON_FOUND)
    list(APPEND qgroundcontrolIncludes ${PHONON_INCLUDES})
    list(APPEND qgroundcontrolLibs ${PHONON_LIBS})
endif()

message(STATUS "\t\tSDL\t\t${SDL_FOUND}")
if (SDL_FOUND)
    list(APPEND qgroundcontrolIncludes ${SDL_INCLUDE_DIR})
    list(APPEND qgroundcontrolLibs ${SDL_LIBRARY})
endif()

message(STATUS "\t\tOPENGL\t\t${OPENGL_FOUND}")
if (OPENGL_FOUND)
    list(APPEND qgroundcontrolIncludes ${OPENGL_INCLUDE_DIR})
    list(APPEND qgroundcontrolLibs ${OPENGL_LIBRARIES})
endif()

# unix only dependencies
if (UNIX)
    message(STATUS "\t\tFLITE\t\t${FLITE_FOUND}")
    if (FLITE_FOUND)
        list(APPEND qgroundcontrolIncludes ${FLITE_INCLUDE_DIR})
        list(APPEND qgroundcontrolLibs ${FLITE_LIBRARIES})
    endif ()
endif()

# set include directories
include_directories(${qgroundcontrolIncludes})

message(STATUS "=======================================")

# project flags
include (${QT_USE_FILE})
set (commonLibs
        )
add_definitions(-D_TTY_POSIX_)

# qgrouncontrol forms
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
    src/ui/designer/QGCCommandButton.ui
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
    #src/ui/map3D/QGCGoogleEarthView.ui
    src/ui/SlugsDataSensorView.ui
    src/ui/SlugsHilSim.ui
    #src/ui/SlugsPIDControl.ui
    #src/ui/SlugsVideoCamControl.ui
    src/ui/SlugsPadCameraControl.ui
    src/ui/uas/QGCUnconnectedInfoWidget.ui
    src/ui/designer/QGCToolWidget.ui
    src/ui/designer/QGCParamSlider.ui
    src/ui/designer/QGCActionButton.ui
    src/ui/QGCMAVLinkLogPlayer.ui
    src/ui/QGCWaypointListMulti.ui
    src/ui/mission/QGCCustomWaypointAction.ui
    src/ui/QGCUDPLinkConfiguration.ui
    src/ui/UASControlParameters.ui
    src/ui/QGCSettingsWidget.ui
    )

# qgroundcontrol headers without Q_OBJECT
# r !grep -RL Q_OBJECT src | grep "^.*\.[h|hpp]$" | sed -e "s/^/\t/g"
set (qgroundcontrolHdrs 
	src/QGC.h
	src/configuration.h
	src/comm/OpalRT.h
	src/comm/ParameterList.h
	src/comm/Parameter.h
	src/comm/QGCParamID.h
	src/comm/QGCMAVLink.h
	src/MG.h
	src/ui/map3D/WebImage.h
	src/ui/map3D/PixhawkCheetahGeode.h
	src/ui/map3D/WaypointGroupNode.h
	src/ui/map3D/ImageWindowGeode.h
	src/ui/map3D/Imagery.h
	src/ui/map3D/QGCGlut.h
	src/ui/map3D/HUDScaleGeode.h
	src/ui/map3D/Texture.h
	src/ui/map3D/GCManipulator.h
	src/ui/map3D/Q3DWidgetFactory.h
	src/ui/map3D/TextureCache.h
	src/ui/map3D/QOSGWidget.h
	src/ui/map/Waypoint2DIcon.h
	src/ui/map/MAV2DIcon.h
	src/ui/OgreWidget.h
	src/ui/generated/ObjectDetectionView.h
	src/ui/generated/MAVLinkSettingsWidget.h
	src/ui/generated/SerialSettings.h
	src/ui/generated/WaypointView.h
	src/ui/generated/LineChart.h
	src/ui/generated/UASList.h
	src/ui/generated/UASInfo.h
	src/ui/generated/MainWindow.h
	src/ui/generated/DebugConsole.h
	src/ui/generated/XMLCommProtocolWidget.h
	src/ui/generated/WatchdogView.h
	src/ui/generated/WatchdogControl.h
	src/ui/generated/UASControl.h
	src/ui/generated/WatchdogProcessView.h
	src/ui/generated/ParameterInterface.h
	src/ui/generated/HDDisplay.h
	src/ui/generated/WaypointList.h
	src/ui/generated/JoystickWidget.h
	src/ui/generated/QGCSensorSettingsWidget.h
	src/ui/generated/MapWidget.h
	src/ui/generated/AudioOutputWidget.h
	src/ui/generated/UASView.h
	src/ui/generated/CommSettings.h
	src/input/Freenect.h
)

# qgroundcontrol headers with Q_OBJECT
# r !grep -Rl Q_OBJECT src | grep "^.*\.[h|hpp]$" | sed "s/^/\t/g"
set(qgroundcontrolMocSrc 
	src/Core.h
    src/uas/ArduPilotMegaMAV.h
    src/uas/PxQuadMAV.h
    src/uas/QGCMAVLinkUASFactory.h
    src/uas/QGCUASParamManager.h
    src/uas/SlugsMAV.h
    src/uas/UAS.h
    src/uas/UASInterface.h
    src/uas/UASManager.h
    src/uas/UASWaypointManager.h
	src/Waypoint.h
	src/LogCompressor.h
    src/GAudioOutput.h
	src/comm/AS4Protocol.h
	src/comm/MAVLinkSwarmSimulationLink.h
	src/comm/ProtocolInterface.h
	#src/comm/OpalLink.h
	src/comm/MAVLinkProtocol.h
	src/comm/SerialLinkInterface.h
	src/comm/SerialInterface.h
	src/comm/UDPLink.h
	src/comm/LinkManager.h
	src/comm/LinkInterface.h
	src/comm/MAVLinkSimulationLink.h
	src/comm/SerialSimulationLink.h
	src/comm/MAVLinkSimulationWaypointPlanner.h
	src/comm/MAVLinkSimulationMAV.h
	#src/comm/QGCNMEAProtocol.h
	src/comm/SerialLink.h
    src/ui/uas/UASControlParameters.h
	src/ui/QGCSettingsWidget.h
	#src/ui/map3D/WebImageCache.h
	#src/ui/map3D/QGCGoogleEarthView.h
	src/ui/map3D/QMap3D.h
	#src/ui/map3D/Pixhawk3DWidget.h
	#src/ui/map3D/Q3DWidget.h
	src/ui/map3D/QGCWebPage.h
	src/ui/ObjectDetectionView.h
	src/ui/SerialConfigurationWindow.h
    src/ui/designer/QGCCommandButton.h
	src/ui/QGCFirmwareUpdate.h
	src/ui/CommConfigurationWindow.h
	src/ui/MAVLinkSettingsWidget.h
	src/ui/SlugsDataSensorView.h
	src/ui/WaypointView.h
	src/ui/QGCPxImuFirmwareUpdate.h
	src/ui/QGCWebView.h
	src/ui/QGCDataPlot2D.h
	src/ui/HSIDisplay.h
	src/ui/SlugsPadCameraControl.h
	src/ui/QGCMainWindowAPConfigurator.h
	src/ui/MainWindow.h
	#src/ui/SlugsVideoCamControl.h
	src/ui/DebugConsole.h
	src/ui/XMLCommProtocolWidget.h
	src/ui/uas/UASListWidget.h
	src/ui/uas/UASInfoWidget.h
	src/ui/uas/QGCUnconnectedInfoWidget.h
	src/ui/uas/UASControlWidget.h
	src/ui/uas/UASView.h
	#src/ui/SlugsPIDControl.h
	src/ui/HUD.h
	src/ui/RadioCalibration/RadioCalibrationWindow.h
	src/ui/RadioCalibration/RadioCalibrationData.h
	src/ui/RadioCalibration/SwitchCalibrator.h
	src/ui/RadioCalibration/AbstractCalibrator.h
	src/ui/RadioCalibration/CurveCalibrator.h
	src/ui/RadioCalibration/AirfoilServoCalibrator.h
	src/ui/ParameterInterface.h
	src/ui/linechart/LinechartWidget.h
	src/ui/linechart/IncrementalPlot.h
	src/ui/linechart/Scrollbar.h
	src/ui/linechart/Linecharts.h
	src/ui/linechart/ScrollZoomer.h
	src/ui/linechart/LinechartPlot.h
	src/ui/HDDisplay.h
	src/ui/watchdog/WatchdogView.h
	src/ui/watchdog/WatchdogControl.h
	src/ui/watchdog/WatchdogProcessView.h
	src/ui/QGCMAVLinkLogPlayer.h
	src/ui/QGCUDPLinkConfiguration.h
	#src/ui/OpalLinkConfigurationWindow.h
	src/ui/SlugsHilSim.h
	src/ui/WaypointList.h
	src/ui/JoystickWidget.h
	src/ui/QGCWaypointListMulti.h
	src/ui/CameraView.h
	src/ui/QGCSensorSettingsWidget.h
	src/ui/designer/QGCToolWidgetItem.h
	src/ui/designer/QGCParamSlider.h
	src/ui/designer/QGCActionButton.h
	src/ui/designer/QGCToolWidget.h
	src/ui/QGCParamWidget.h
	src/ui/MapWidget.h
	src/ui/QGCRemoteControlView.h
	src/ui/AudioOutputWidget.h
	#src/standalone/mavlinkgen/MAVLinkGen.h
	src/input/JoystickInput.h
    )

# qgroundcontrol source
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
    src/comm/SerialLink.cc
    src/comm/SerialSimulationLink.cc
    src/comm/UDPLink.cc
    src/input/JoystickInput.cc
    src/uas/ArduPilotMegaMAV.cc
    src/uas/PxQuadMAV.cc
    src/uas/QGCMAVLinkUASFactory.cc
    src/uas/QGCUASParamManager.cc
    src/uas/SlugsMAV.cc
    src/uas/UAS.cc
    src/uas/UASManager.cc
    src/uas/UASWaypointManager.cc
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
    src/ui/designer/QGCCommandButton.cc
    src/ui/QGCFirmwareUpdate.cc
    src/ui/QGCMAVLinkLogPlayer.cc
    src/ui/QGCMainWindowAPConfigurator.cc
    src/ui/QGCParamWidget.cc
    src/ui/QGCPxImuFirmwareUpdate.cc
    src/ui/QGCRemoteControlView.cc
    src/ui/QGCSensorSettingsWidget.cc
    src/ui/uas/UASControlParameters.cpp
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
    #src/ui/SlugsPIDControl.cpp
    src/ui/SlugsPadCameraControl.cpp
    #src/ui/SlugsVideoCamControl.cpp
    src/ui/WaypointList.cc
    src/ui/WaypointView.cc
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
    src/ui/uas/QGCUnconnectedInfoWidget.cc
    src/ui/uas/UASControlWidget.cc
    src/ui/uas/UASInfoWidget.cc
    src/ui/uas/UASListWidget.cc
    src/ui/uas/UASView.cc
    src/ui/watchdog/WatchdogControl.cc
    src/ui/watchdog/WatchdogProcessView.cc
    src/ui/watchdog/WatchdogView.cc
    )

# qgroundcontrol resource files
set(qgroundcontrolRscSrc mavground.qrc)

# qgroundcontrol linking
qt4_wrap_cpp(qgroundcontrolMoc ${qgroundcontrolMocSrc})
qt4_wrap_ui(qgroundcontrolUi ${qgroundcontrolUiSrc})
qt4_add_resources(qgroundcontrolRsc ${qgroundcontrolRscSrc})
add_executable(qgroundcontrol
    ${qgroundcontrolSrc}
    ${qgroundcontrolMoc}
    ${qgroundcontrolUi}
    ${qgroundcontrolRsc}
    )
add_dependencies(qgroundcontrol MAVLINK)
target_link_libraries(qgroundcontrol
    ${qgroundcontrolLibs}
    qextserialport qmapcontrol qwt)

# qgroundcontrol install
install(TARGETS qgroundcontrol DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

# qwt library
#----------------------------------------------------------------------------

# qwt headers without Q_OBJECT
# r !grep -L Q_OBJECT src/lib/qwt | grep "^.*\.[h|hpp]$" | sed "s/^/\t/g"
set (qwtHdrs 
    src/lib/qwt/qwt_abstract_scale_draw.h
    src/lib/qwt/qwt_abstract_scale.h
    src/lib/qwt/qwt_array.h
    src/lib/qwt/qwt_arrow_button.h
    src/lib/qwt/qwt_clipper.h
    src/lib/qwt/qwt_color_map.h
    src/lib/qwt/qwt_compass_rose.h
    src/lib/qwt/qwt_curve_fitter.h
    src/lib/qwt/qwt_data.h
    src/lib/qwt/qwt_dial_needle.h
    src/lib/qwt/qwt_double_interval.h
    src/lib/qwt/qwt_double_range.h
    src/lib/qwt/qwt_double_rect.h
    src/lib/qwt/qwt_event_pattern.h
    src/lib/qwt/qwt_global.h
    src/lib/qwt/qwt.h
    src/lib/qwt/qwt_interval_data.h
    src/lib/qwt/qwt_layout_metrics.h
    src/lib/qwt/qwt_legend_itemmanager.h
    src/lib/qwt/qwt_math.h
    src/lib/qwt/qwt_paint_buffer.h
    src/lib/qwt/qwt_painter.h
    src/lib/qwt/qwt_picker_machine.h
    src/lib/qwt/qwt_plot_curve.h
    src/lib/qwt/qwt_plot_dict.h
    src/lib/qwt/qwt_plot_grid.h
    src/lib/qwt/qwt_plot_item.h
    src/lib/qwt/qwt_plot_layout.h
    src/lib/qwt/qwt_plot_marker.h
    src/lib/qwt/qwt_plot_printfilter.h
    src/lib/qwt/qwt_plot_rasteritem.h
    src/lib/qwt/qwt_plot_scaleitem.h
    src/lib/qwt/qwt_plot_spectrogram.h
    src/lib/qwt/qwt_plot_svgitem.h
    src/lib/qwt/qwt_polygon.h
    src/lib/qwt/qwt_raster_data.h
    src/lib/qwt/qwt_rect.h
    src/lib/qwt/qwt_round_scale_draw.h
    src/lib/qwt/qwt_scale_div.h
    src/lib/qwt/qwt_scale_draw.h
    src/lib/qwt/qwt_scale_engine.h
    src/lib/qwt/qwt_scale_map.h
    src/lib/qwt/qwt_spline.h
    src/lib/qwt/qwt_symbol.h
    src/lib/qwt/qwt_text_engine.h
    src/lib/qwt/qwt_text.h
    src/lib/qwt/qwt_valuelist.h
    )

# qwt headers with Q_OBJECT
# r !grep -l Q_OBJECT src/lib/qwt | grep "^.*\.[h|hpp]$" | sed "s/^/\t/g"
set (qwtMocSrc 
    src/lib/qwt/qwt_abstract_slider.h
    src/lib/qwt/qwt_analog_clock.h
    src/lib/qwt/qwt_compass.h
    src/lib/qwt/qwt_counter.h
    src/lib/qwt/qwt_dial.h
    src/lib/qwt/qwt_dyngrid_layout.h
    src/lib/qwt/qwt_knob.h
    src/lib/qwt/qwt_legend.h
    src/lib/qwt/qwt_legend_item.h
    src/lib/qwt/qwt_magnifier.h
    src/lib/qwt/qwt_panner.h
    src/lib/qwt/qwt_picker.h
    src/lib/qwt/qwt_plot_canvas.h
    src/lib/qwt/qwt_plot.h
    src/lib/qwt/qwt_plot_magnifier.h
    src/lib/qwt/qwt_plot_panner.h
    src/lib/qwt/qwt_plot_picker.h
    src/lib/qwt/qwt_plot_zoomer.h
    src/lib/qwt/qwt_scale_widget.h
    src/lib/qwt/qwt_slider.h
    src/lib/qwt/qwt_text_label.h
    src/lib/qwt/qwt_thermo.h
    src/lib/qwt/qwt_wheel.h
    )

# qwt source
set (qwtSrc
    src/lib/qwt/qwt_plot_magnifier.cpp
    src/lib/qwt/qwt_plot_curve.cpp
    src/lib/qwt/qwt_panner.cpp
    src/lib/qwt/qwt_round_scale_draw.cpp
    src/lib/qwt/qwt_clipper.cpp
    src/lib/qwt/qwt_plot_printfilter.cpp
    src/lib/qwt/qwt_data.cpp
    src/lib/qwt/qwt_text_engine.cpp
    src/lib/qwt/qwt_dial.cpp
    src/lib/qwt/qwt_plot_rasteritem.cpp
    src/lib/qwt/qwt_spline.cpp
    src/lib/qwt/qwt_rect.cpp
    src/lib/qwt/qwt_plot_canvas.cpp
    src/lib/qwt/qwt_magnifier.cpp
    src/lib/qwt/qwt_analog_clock.cpp
    src/lib/qwt/qwt_knob.cpp
    src/lib/qwt/qwt_counter.cpp
    src/lib/qwt/qwt_plot_axis.cpp
    src/lib/qwt/qwt_interval_data.cpp
    src/lib/qwt/qwt_scale_map.cpp
    src/lib/qwt/qwt_slider.cpp
    src/lib/qwt/qwt_compass_rose.cpp
    src/lib/qwt/qwt_plot_print.cpp
    src/lib/qwt/qwt_plot_layout.cpp
    src/lib/qwt/qwt_abstract_scale_draw.cpp
    src/lib/qwt/qwt_abstract_slider.cpp
    src/lib/qwt/qwt_picker.cpp
    src/lib/qwt/qwt_raster_data.cpp
    src/lib/qwt/qwt_picker_machine.cpp
    src/lib/qwt/qwt_plot_marker.cpp
    src/lib/qwt/qwt_scale_draw.cpp
    src/lib/qwt/qwt_thermo.cpp
    src/lib/qwt/qwt_layout_metrics.cpp
    src/lib/qwt/qwt_dial_needle.cpp
    src/lib/qwt/qwt_plot_xml.cpp
    src/lib/qwt/qwt_legend.cpp
    src/lib/qwt/qwt_plot_picker.cpp
    src/lib/qwt/qwt_event_pattern.cpp
    src/lib/qwt/qwt_curve_fitter.cpp
    src/lib/qwt/qwt_double_range.cpp
    src/lib/qwt/qwt_painter.cpp
    src/lib/qwt/qwt_double_interval.cpp
    src/lib/qwt/qwt_math.cpp
    src/lib/qwt/qwt_plot_item.cpp
    src/lib/qwt/qwt_plot_zoomer.cpp
    src/lib/qwt/qwt_symbol.cpp
    src/lib/qwt/qwt_scale_div.cpp
    src/lib/qwt/qwt_color_map.cpp
    src/lib/qwt/qwt_plot.cpp
    src/lib/qwt/qwt_plot_spectrogram.cpp
    src/lib/qwt/qwt_paint_buffer.cpp
    src/lib/qwt/qwt_plot_dict.cpp
    src/lib/qwt/qwt_scale_widget.cpp
    src/lib/qwt/qwt_text.cpp
    src/lib/qwt/qwt_dyngrid_layout.cpp
    src/lib/qwt/qwt_abstract_scale.cpp
    src/lib/qwt/qwt_plot_svgitem.cpp
    src/lib/qwt/qwt_arrow_button.cpp
    src/lib/qwt/qwt_double_rect.cpp
    src/lib/qwt/qwt_compass.cpp
    src/lib/qwt/qwt_wheel.cpp
    src/lib/qwt/qwt_legend_item.cpp
    src/lib/qwt/qwt_plot_scaleitem.cpp
    src/lib/qwt/qwt_text_label.cpp
    src/lib/qwt/qwt_plot_panner.cpp
    src/lib/qwt/qwt_scale_engine.cpp
    src/lib/qwt/qwt_plot_grid.cpp
    )

# qwt linking
qt4_wrap_cpp(qwtMoc ${qwtMocSrc})
add_library(qwt ${qwtMoc} ${qwtSrc})
target_link_libraries(qwt ${QT_LIBRARIES})

# qextserialport library
#----------------------------------------------------------------------------

# qextserialport headers without Q_OBJECT
# r !grep -RL Q_OBJECT src/lib/qextserialport | grep "^.*\.[h|hpp]$" | sed "s/^/\t/g"
set (qextserialportHdrs
    src/lib/qextserialport/posix_qextserialport.h
    src/lib/qextserialport/qextserialenumerator.h
    )
# qextserialport headers with Q_OBJECT
# r !grep -Rl Q_OBJECT src/lib/qextserialport | grep "^.*\.[h|hpp]$" | sed "s/^/\t/g"
set (qextserialportMocSrc
    src/lib/qextserialport/qextserialbase.h
    src/lib/qextserialport/qextserialport.h
    #src/lib/qextserialport/win_qextserialport.h
)
# qextserialport src
set (qextserialportSrc
    src/lib/qextserialport/posix_qextserialport.cpp
    src/lib/qextserialport/qextserialport.cpp
    src/lib/qextserialport/qextserialbase.cpp
    #src/lib/qextserialport/win_qextserialport.cpp
    src/lib/qextserialport/qextserialenumerator.cpp
    )

# qextserial linking
qt4_wrap_cpp(qextserialportMoc ${qextserialportMocSrc})
add_library(qextserialport ${qextserialportMoc} ${qextserialportSrc})
target_link_libraries(qextserialport ${QT_LIBRARIES})

# qmapcontrol library
#----------------------------------------------------------------------------

# qmapcontrol headers without Q_OBJECT
# r !grep -RL Q_OBJECT lib/QMapControl | grep "^.*\.[h|hpp]$" | sed "s/^/\t/g"
set (qmapcontrolHdrs
	lib/QMapControl/src/circlepoint.h
	lib/QMapControl/src/wmsmapadapter.h
	lib/QMapControl/src/gps_position.h
	lib/QMapControl/src/fixedimageoverlay.h
	lib/QMapControl/src/imagepoint.h
	lib/QMapControl/qmapcontrol.h
    )

# qmapcontorl headers with Q_OBJECT
# r !grep -Rl Q_OBJECT lib/QMapControl | grep "^.*\.[h|hpp]$" | sed "s/^/\t/g"
set (qmapcontrolMocSrc
	lib/QMapControl/src/googlemapadapter.h
	lib/QMapControl/src/mapnetwork.h
	lib/QMapControl/src/mapadapter.h
	lib/QMapControl/src/geometrylayer.h
	lib/QMapControl/src/linestring.h
	lib/QMapControl/src/mapcontrol.h
	lib/QMapControl/src/tilemapadapter.h
	lib/QMapControl/src/curve.h
	lib/QMapControl/src/imagemanager.h
	lib/QMapControl/src/layer.h
	lib/QMapControl/src/maplayer.h
	lib/QMapControl/src/geometry.h
	lib/QMapControl/src/googlesatmapadapter.h
	lib/QMapControl/src/point.h
	lib/QMapControl/src/osmmapadapter.h
	lib/QMapControl/src/layermanager.h
	lib/QMapControl/src/openaerialmapadapter.h
	lib/QMapControl/src/emptymapadapter.h
	lib/QMapControl/src/yahoomapadapter.h
    )

# qmapcontrol source
set (qmapcontrolSrc
    lib/QMapControl/src/point.cpp
    lib/QMapControl/src/imagepoint.cpp
    lib/QMapControl/src/yahoomapadapter.cpp
    lib/QMapControl/src/layermanager.cpp
    lib/QMapControl/src/circlepoint.cpp
    lib/QMapControl/src/imagemanager.cpp
    lib/QMapControl/src/maplayer.cpp
    lib/QMapControl/src/geometrylayer.cpp
    lib/QMapControl/src/mapadapter.cpp
    lib/QMapControl/src/mapnetwork.cpp
    lib/QMapControl/src/linestring.cpp
    lib/QMapControl/src/osmmapadapter.cpp
    lib/QMapControl/src/fixedimageoverlay.cpp
    lib/QMapControl/src/layer.cpp
    lib/QMapControl/src/openaerialmapadapter.cpp
    lib/QMapControl/src/geometry.cpp
    lib/QMapControl/src/mapcontrol.cpp
    lib/QMapControl/src/tilemapadapter.cpp
    lib/QMapControl/src/googlemapadapter.cpp
    lib/QMapControl/src/wmsmapadapter.cpp
    lib/QMapControl/src/googlesatmapadapter.cpp
    lib/QMapControl/src/curve.cpp
    lib/QMapControl/src/emptymapadapter.cpp
    lib/QMapControl/src/gps_position.cpp
    )

# qmapcontrol linking
qt4_wrap_cpp(qmapcontrolMoc ${qmapcontrolMocSrc})
add_library(qmapcontrol ${qmapcontrolMoc} ${qmapcontrolSrc})
target_link_libraries(qmapcontrol ${QT_LIBRARIES})

# vim:ts=4:sw=4:expandtab
