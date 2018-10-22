cmake_minimum_required(VERSION 3.0)
cmake_policy(SET CMP0026 OLD) # allow use of the LOCATION target property

# store the current source directory for future use
set(QT_ANDROID_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR})

# check the JAVA_HOME environment variable
# (I couldn't find a way to set it from this script, it has to be defined outside)
set(JAVA_HOME $ENV{JAVA_HOME})
if(NOT JAVA_HOME)
    message(FATAL_ERROR "The JAVA_HOME environment variable is not set. Please set it to the root directory of the JDK.")
endif()

# make sure that the Android toolchain is used
if(NOT ANDROID)
    message(FATAL_ERROR "Trying to use the CMake Android package without the Android toolchain. Please use the provided toolchain (toolchain/android.toolchain.cmake)")
endif()

# find the Qt root directory
if(NOT Qt5Core_DIR)
    find_package(Qt5Core REQUIRED)
endif()
get_filename_component(QT_ANDROID_QT_ROOT "${Qt5Core_DIR}/../../.." ABSOLUTE)
message(STATUS "Found Qt for Android: ${QT_ANDROID_QT_ROOT}")

# find the Android SDK
if(NOT QT_ANDROID_SDK_ROOT)
    set(QT_ANDROID_SDK_ROOT $ENV{ANDROID_SDK})
    if(NOT QT_ANDROID_SDK_ROOT)
        message(FATAL_ERROR "Could not find the Android SDK. Please set either the ANDROID_SDK environment variable, or the QT_ANDROID_SDK_ROOT CMake variable to the root directory of the Android SDK")
    endif()
endif()
string(REPLACE "\\" "/" QT_ANDROID_SDK_ROOT ${QT_ANDROID_SDK_ROOT}) # androiddeployqt doesn't like backslashes in paths
message(STATUS "Found Android SDK: ${QT_ANDROID_SDK_ROOT}")

# find the Android NDK
if(NOT QT_ANDROID_NDK_ROOT)
    set(QT_ANDROID_NDK_ROOT $ENV{ANDROID_NDK})
    if(NOT QT_ANDROID_NDK_ROOT)
        set(QT_ANDROID_NDK_ROOT ${ANDROID_NDK})
        if(NOT QT_ANDROID_NDK_ROOT)
        message(FATAL_ERROR "Could not find the Android NDK. Please set either the ANDROID_NDK environment or CMake variable, or the QT_ANDROID_NDK_ROOT CMake variable to the root directory of the Android NDK")
        endif()
    endif()
endif()
string(REPLACE "\\" "/" QT_ANDROID_NDK_ROOT ${QT_ANDROID_NDK_ROOT}) # androiddeployqt doesn't like backslashes in paths
message(STATUS "Found Android NDK: ${QT_ANDROID_NDK_ROOT}")

include(CMakeParseArguments)

# define a macro to create an Android APK target
#
# example:
# add_qt_android_apk(my_app_apk my_app
#     NAME "My App"
#     VERSION_CODE 12
#     PACKAGE_NAME "org.mycompany.myapp"
#     PACKAGE_SOURCES ${CMAKE_CURRENT_LIST_DIR}/my-android-sources
#     BUILDTOOLS_REVISION "23.0.3"
#     KEYSTORE ${CMAKE_CURRENT_LIST_DIR}/mykey.keystore myalias
#     KEYSTORE_PASSWORD xxxx
#     DEPENDS a_linked_target "path/to/a_linked_library.so" ...
#     INSTALL
#)
# 
macro(add_qt_android_apk TARGET SOURCE_TARGET)

    # parse the macro arguments
    cmake_parse_arguments(ARG "INSTALL" "NAME;VERSION_CODE;PACKAGE_NAME;PACKAGE_SOURCES;KEYSTORE_PASSWORD;BUILDTOOLS_REVISION" "DEPENDS;KEYSTORE" ${ARGN})

    # extract the full path of the source target binary
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        get_property(QT_ANDROID_APP_PATH TARGET ${SOURCE_TARGET} PROPERTY DEBUG_LOCATION)
    else()
        get_property(QT_ANDROID_APP_PATH TARGET ${SOURCE_TARGET} PROPERTY LOCATION)
    endif()

    # define the application name
    if(ARG_NAME)
        set(QT_ANDROID_APP_NAME ${ARG_NAME})
    else()
        set(QT_ANDROID_APP_NAME ${SOURCE_TARGET})
    endif()

    # define the application package name
    if(ARG_PACKAGE_NAME)
        set(QT_ANDROID_APP_PACKAGE_NAME ${ARG_PACKAGE_NAME})
    else()
        set(QT_ANDROID_APP_PACKAGE_NAME org.qtproject.${SOURCE_TARGET})
    endif()

    # set the Android SDK build-tools revision
    if(ARG_BUILDTOOLS_REVISION)
        set(QT_ANDROID_SDK_BUILDTOOLS_REVISION ${ARG_BUILDTOOLS_REVISION})
    else()
        set(QT_ANDROID_SDK_BUILDTOOLS_REVISION "")
    endif()

    # define the application source package directory
    if(ARG_PACKAGE_SOURCES)
        set(QT_ANDROID_APP_PACKAGE_SOURCE_ROOT ${ARG_PACKAGE_SOURCES})
    else()
        # get version code from arguments, or generate a fixed one if not provided
        set(QT_ANDROID_APP_VERSION_CODE ${ARG_VERSION_CODE})
        if(NOT QT_ANDROID_APP_VERSION_CODE)
            set(QT_ANDROID_APP_VERSION_CODE 1)
        endif()

        # try to extract the app version from the target properties, or use the version code if not provided
        get_property(QT_ANDROID_APP_VERSION TARGET ${SOURCE_TARGET} PROPERTY VERSION)
        if(NOT QT_ANDROID_APP_VERSION)
            set(QT_ANDROID_APP_VERSION ${QT_ANDROID_APP_VERSION_CODE})
        endif()

        # create a subdirectory for the extra package sources
        set(QT_ANDROID_APP_PACKAGE_SOURCE_ROOT "${CMAKE_CURRENT_BINARY_DIR}/package")

        # generate a manifest from the template
        configure_file(${QT_ANDROID_SOURCE_DIR}/AndroidManifest.xml.in ${QT_ANDROID_APP_PACKAGE_SOURCE_ROOT}/AndroidManifest.xml @ONLY)
    endif()

    # set the list of dependant libraries
    if(ARG_DEPENDS)
        foreach(LIB ${ARG_DEPENDS})
            if(TARGET ${LIB})
                # item is a CMake target, extract the library path
                if(CMAKE_BUILD_TYPE STREQUAL "Debug")
                    get_property(LIB_PATH TARGET ${LIB} PROPERTY DEBUG_LOCATION)
                else()
                    get_property(LIB_PATH TARGET ${LIB} PROPERTY LOCATION)
                endif()
                set(LIB ${LIB_PATH})
            endif()
        if(EXTRA_LIBS)
            set(EXTRA_LIBS "${EXTRA_LIBS},${LIB}")
        else()
            set(EXTRA_LIBS "${LIB}")
        endif()
        endforeach()
        set(QT_ANDROID_APP_EXTRA_LIBS "\"android-extra-libs\": \"${EXTRA_LIBS}\",")
    endif()

    # make sure that the output directory for the Android package exists
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/libs/${ANDROID_ABI})

    # create the configuration file that will feed androiddeployqt
    configure_file(${QT_ANDROID_SOURCE_DIR}/qtdeploy.json.in ${CMAKE_CURRENT_BINARY_DIR}/qtdeploy.json @ONLY)

    # check if the apk must be signed
    if(ARG_KEYSTORE)
        set(SIGN_OPTIONS --release --sign ${ARG_KEYSTORE} --tsa http://timestamp.digicert.com)
        if(ARG_KEYSTORE_PASSWORD)
            set(SIGN_OPTIONS ${SIGN_OPTIONS} --storepass ${ARG_KEYSTORE_PASSWORD})
        endif()
    endif()

    # check if the apk must be installed to the device
    if(ARG_INSTALL)
        set(INSTALL_OPTIONS --reinstall)
    endif()

    # specify the Android API level
    if(ANDROID_NATIVE_API_LEVEL)
        set(TARGET_LEVEL_OPTIONS --android-platform android-${ANDROID_NATIVE_API_LEVEL})
    endif()

    # create a custom command that will run the androiddeployqt utility to prepare the Android package
    add_custom_target(
        ${TARGET}
        ALL
        DEPENDS ${SOURCE_TARGET}
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/libs/${ANDROID_ABI} # it seems that recompiled libraries are not copied if we don't remove them first
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/libs/${ANDROID_ABI}
        COMMAND ${CMAKE_COMMAND} -E copy ${QT_ANDROID_APP_PATH} ${CMAKE_CURRENT_BINARY_DIR}/libs/${ANDROID_ABI}
        COMMAND ${QT_ANDROID_QT_ROOT}/bin/androiddeployqt --verbose --output ${CMAKE_CURRENT_BINARY_DIR} --input ${CMAKE_CURRENT_BINARY_DIR}/qtdeploy.json --gradle ${TARGET_LEVEL_OPTIONS} ${INSTALL_OPTIONS} ${SIGN_OPTIONS}
    )

endmacro()
