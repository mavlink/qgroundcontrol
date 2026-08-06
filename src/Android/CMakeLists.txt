# ============================================================================
# Android Module
# Android platform-specific initialization and interfaces
# ============================================================================

# Only build on Android platforms
if(NOT ANDROID)
    return()
endif()

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        AndroidEvents.cc
        AndroidEvents.h
        AndroidInit.cc
        AndroidInterface.cc
        AndroidInterface.h
        AndroidSerial.cc
        AndroidSerial.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ----------------------------------------------------------------------------
# Android Serial Port Support
# ----------------------------------------------------------------------------
add_subdirectory(qtandroidserialport)

# ----------------------------------------------------------------------------
# Qt Android Extensions (2GIS) — selective compile
# https://github.com/2gis/qtandroidextensions
#
# Only the wake/WiFi locker classes and their QLocks + QJniHelpers
# dependencies are compiled. The full library cannot be built with Qt6
# because some source files (Mobility/) use `using namespace QJniHelpers;`
# with unqualified QJniObject, which is ambiguous with Qt6's QJniObject.
# ----------------------------------------------------------------------------
CPMAddPackage(
    NAME qtandroidextensions
    GITHUB_REPOSITORY 2gis/qtandroidextensions
    GIT_TAG qt6-beta1
    DOWNLOAD_ONLY YES
)

qgc_require_cpm_added(qtandroidextensions)

set(_qae_root ${qtandroidextensions_SOURCE_DIR})
set(_qjni_dir ${_qae_root}/QJniHelpers)
set(_qtah_dir ${_qae_root}/QtAndroidHelpers)

# MOC for QLock_p.h (has Q_OBJECT)
qt6_wrap_cpp(_qlocks_moc ${_qtah_dir}/QLocks/QLock_p.h)

# qtandroidextensions disables AUTOMOC in its own CMakeLists; compiling its sources in
# QGC's AUTOMOC-ON target makes the scanner emit MOC output into the CPM cache with
# broken paths, so build it as a separate AUTOMOC-OFF static lib.
add_library(qtandroidextensions_lockers STATIC
    # QJniHelpers
    ${_qjni_dir}/QJniHelpers.cpp
    ${_qjni_dir}/QAndroidQPAPluginGap.cpp
    ${_qjni_dir}/QJniLangUtils.cpp
    # QtAndroidHelpers — lockers only
    ${_qtah_dir}/QAndroidPartialWakeLocker.cpp
    ${_qtah_dir}/QAndroidWiFiLocker.cpp
    # QLocks infrastructure
    ${_qtah_dir}/QLocks/QLockedObject.cpp
    ${_qtah_dir}/QLocks/QLock.cpp
    ${_qtah_dir}/QLocks/QLockHandler.cpp
    ${_qlocks_moc}
)
set_target_properties(qtandroidextensions_lockers PROPERTIES AUTOMOC OFF)

target_include_directories(qtandroidextensions_lockers
    PUBLIC
        ${_qae_root}        # <QJniHelpers/QJniHelpers.h>, <QtAndroidHelpers/...>
    PRIVATE
        ${_qjni_dir}        # internal includes within QJniHelpers
        ${_qtah_dir}        # internal includes within QtAndroidHelpers (QLocks/...)
)

# Suppress deprecated 'detectJavaVM' alias (namespace-scope variable defined in
# a header — causes duplicate-symbol errors under LTO).
target_compile_definitions(qtandroidextensions_lockers PRIVATE QTANDROIDEXTENSIONS_NO_DEPRECATES)

target_link_libraries(qtandroidextensions_lockers PUBLIC Qt6::Core PRIVATE Qt6::CorePrivate)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE qtandroidextensions_lockers)

# Deploy Java sources to QGC_ANDROID_EXTRA_JAVA_SOURCES_DIR (added to build.gradle's source set)
file(GLOB _qjnihelpers_java "${_qjni_dir}/ru.dublgis.qjnihelpers/*.java")
file(GLOB _qtandroidhelpers_java "${_qtah_dir}/ru.dublgis.androidhelpers/*.java")
file(COPY ${_qjnihelpers_java}
    DESTINATION "${QGC_ANDROID_EXTRA_JAVA_SOURCES_DIR}/ru/dublgis/qjnihelpers"
)
file(COPY ${_qtandroidhelpers_java}
    DESTINATION "${QGC_ANDROID_EXTRA_JAVA_SOURCES_DIR}/ru/dublgis/androidhelpers"
)

# ----------------------------------------------------------------------------
# Qt Android Tools (FalsinSoft)
# https://github.com/FalsinSoft/QtAndroidTools
# ----------------------------------------------------------------------------
CPMAddPackage(
    NAME QtAndroidTools
    GITHUB_REPOSITORY FalsinSoft/QtAndroidTools
    GIT_TAG 47ba0fcf75c91a4e3b01ee34f718bf59014892db
    DOWNLOAD_ONLY YES
)

qgc_require_cpm_added(QtAndroidTools)

set(_qtat_dir ${QtAndroidTools_SOURCE_DIR}/QtAndroidTools)

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${_qtat_dir}/QAndroidScreen.cpp
        ${_qtat_dir}/QAndroidScreen.h
)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${_qtat_dir})

# Deploy Java sources to QGC_ANDROID_EXTRA_JAVA_SOURCES_DIR (added to build.gradle's source set)
file(COPY "${_qtat_dir}/src/com/falsinsoft/qtandroidtools/AndroidScreen.java"
    DESTINATION "${QGC_ANDROID_EXTRA_JAVA_SOURCES_DIR}/com/falsinsoft/qtandroidtools"
)
