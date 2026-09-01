# ----------------------------------------------------------------------------
# QGroundControl Apple Platform Configuration (macOS and iOS)
# ----------------------------------------------------------------------------

include_guard(GLOBAL)

include(AppleXCFramework)

if(NOT APPLE)
    message(FATAL_ERROR "QGC: Invalid Platform: Apple.cmake included but platform is not Apple")
endif()

# .mm sources (e.g. GstIOSurfaceVideoBuffer.mm) need OBJCXX rules at root scope so
# CMAKE_OBJCXX_COMPILE_OBJECT is populated for the QGC target. Done here unconditionally
# for macOS + iOS rather than relying on a deferred enable_language inside Find modules.
enable_language(OBJC OBJCXX)

if(CMAKE_GENERATOR STREQUAL "Xcode" AND MACOS)
    set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${QGC_MACOS_ENTITLEMENTS_PATH}")
endif()

# ----------------------------------------------------------------------------
# macOS/iOS Bundle Configuration
# ----------------------------------------------------------------------------
if(IOS)
    set(_qgc_bundle_plist "${CMAKE_SOURCE_DIR}/deploy/ios/Info.plist.app.in")
    set(_qgc_bundle_identifier "${QGC_PACKAGE_NAME}")
    set(_qgc_bundle_short_version "${CMAKE_PROJECT_VERSION}")
else()
    set(_qgc_bundle_plist "${QGC_MACOS_PLIST_PATH}")
    set(_qgc_bundle_identifier "${QGC_MACOS_BUNDLE_ID}")
    set(_qgc_bundle_short_version "${CMAKE_PROJECT_VERSION_MAJOR}.${CMAKE_PROJECT_VERSION_MINOR}")
endif()

set_target_properties(
    ${CMAKE_PROJECT_NAME}
    PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${_qgc_bundle_plist}"
               MACOSX_BUNDLE_BUNDLE_NAME "${CMAKE_PROJECT_NAME}"
               MACOSX_BUNDLE_BUNDLE_VERSION "${CMAKE_PROJECT_VERSION}"
               MACOSX_BUNDLE_COPYRIGHT "${QGC_APP_COPYRIGHT}"
               MACOSX_BUNDLE_GUI_IDENTIFIER "${_qgc_bundle_identifier}"
               MACOSX_BUNDLE_INFO_STRING "${CMAKE_PROJECT_DESCRIPTION}"
               MACOSX_BUNDLE_LONG_VERSION_STRING
               "${CMAKE_PROJECT_VERSION_MAJOR}.${CMAKE_PROJECT_VERSION_MINOR}.${CMAKE_PROJECT_VERSION_PATCH}"
               MACOSX_BUNDLE_SHORT_VERSION_STRING "${_qgc_bundle_short_version}"
)

# ----------------------------------------------------------------------------
# Platform-Specific Configuration
# ----------------------------------------------------------------------------
if(MACOS)
    cmake_path(GET QGC_MACOS_ICON_PATH FILENAME _qgc_macos_bundle_icon)
    set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY MACOSX_BUNDLE_ICON_FILE "${_qgc_macos_bundle_icon}")

    set_source_files_properties("${QGC_MACOS_ICON_PATH}" PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set_source_files_properties("${QGC_MACOS_ENTITLEMENTS_PATH}" PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE "${QGC_MACOS_ENTITLEMENTS_PATH}" "${QGC_MACOS_ICON_PATH}")

    message(STATUS "QGC: macOS platform configuration applied")
elseif(IOS)
    option(QGC_IOS_APP_STORE_BUILD "Sign the iOS app for App Store distribution" OFF)
    set(QGC_IOS_DEVELOPMENT_TEAM
        ""
        CACHE STRING "Apple development team ID used for App Store signing"
    )
    set(QGC_IOS_PROVISIONING_PROFILE
        ""
        CACHE STRING "App Store provisioning profile name or UUID"
    )

    # Adds compatibility sources and embeds the framework after its target
    # exists.
    function(_qgc_ios_finalize_gstreamer_mobile target)
        if(NOT TARGET GStreamerMobileXcfw)
            return()
        endif()

        if(CMAKE_OSX_SYSROOT MATCHES "[Ss]imulator" AND "x86_64" IN_LIST CMAKE_OSX_ARCHITECTURES)
            target_sources(GStreamerMobileXcfw
                           PRIVATE "${CMAKE_SOURCE_DIR}/src/VideoManager/VideoReceiver/GStreamer/IOSPipe2Compat.c"
            )
        endif()

        if(CMAKE_GENERATOR MATCHES "Xcode")
            set_property(
                TARGET "${target}"
                APPEND
                PROPERTY XCODE_EMBED_FRAMEWORKS GStreamerMobileXcfw
            )
            set_target_properties("${target}" PROPERTIES XCODE_EMBED_FRAMEWORKS_CODE_SIGN_ON_COPY YES
                                                         XCODE_EMBED_FRAMEWORKS_REMOVE_HEADERS_ON_COPY YES
            )
            message(STATUS "QGC: GStreamerMobile will be embedded by Xcode")
        else()
            add_custom_command(
                TARGET "${target}"
                POST_BUILD
                COMMAND "${CMAKE_COMMAND}" -E copy_directory "$<TARGET_BUNDLE_DIR:GStreamerMobileXcfw>"
                        "$<TARGET_BUNDLE_DIR:${target}>/Frameworks/$<TARGET_BUNDLE_DIR_NAME:GStreamerMobileXcfw>"
                COMMENT "Embedding GStreamerMobile framework"
                VERBATIM
            )
            message(STATUS "QGC: GStreamerMobile will be embedded at build time (Ninja)")
        endif()
    endfunction()

    set(_qgc_ios_asset_catalog "${CMAKE_SOURCE_DIR}/deploy/ios/Images.xcassets")
    set(_qgc_ios_launch_screen "${CMAKE_SOURCE_DIR}/deploy/ios/QGCLaunchScreen.storyboard")

    set(CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "${QGC_IOS_DEPLOYMENT_TARGET}")
    set(CMAKE_XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "${QGC_IOS_TARGETED_DEVICE_FAMILY}")

    set_target_properties(
        ${CMAKE_PROJECT_NAME}
        PROPERTIES QT_IOS_LAUNCH_SCREEN "${_qgc_ios_launch_screen}"
                   XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${QGC_PACKAGE_NAME}"
                   XCODE_ATTRIBUTE_PRODUCT_NAME "${CMAKE_PROJECT_NAME}"
                   XCODE_ATTRIBUTE_CURRENT_PROJECT_VERSION ${CMAKE_PROJECT_VERSION}
                   XCODE_ATTRIBUTE_MARKETING_VERSION "${CMAKE_PROJECT_VERSION}"
                   XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME AppIcon
                   XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "${QGC_IOS_DEPLOYMENT_TARGET}"
                   XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "${QGC_IOS_TARGETED_DEVICE_FAMILY}"
                   XCODE_ATTRIBUTE_INFOPLIST_KEY_CFBundleDisplayName ${CMAKE_PROJECT_NAME}
                   XCODE_ATTRIBUTE_INFOPLIST_KEY_LSApplicationCategoryType public.app-category.navigation
                   XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS YES
    )

    if(QGC_IOS_APP_STORE_BUILD)
        if(NOT CMAKE_GENERATOR STREQUAL "Xcode")
            message(FATAL_ERROR "QGC_IOS_APP_STORE_BUILD requires the Xcode generator")
        endif()
        if(NOT QGC_IOS_DEVELOPMENT_TEAM OR NOT QGC_IOS_PROVISIONING_PROFILE)
            message(
                FATAL_ERROR "QGC_IOS_APP_STORE_BUILD requires QGC_IOS_DEVELOPMENT_TEAM and QGC_IOS_PROVISIONING_PROFILE"
            )
        endif()

        set_target_properties(
            ${CMAKE_PROJECT_NAME}
            PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_STYLE Manual
                       XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Apple Distribution"
                       XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED YES
                       XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED YES
                       XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${QGC_IOS_DEVELOPMENT_TEAM}"
                       XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER "${QGC_IOS_PROVISIONING_PROFILE}"
        )
    endif()

    if(CMAKE_GENERATOR MATCHES "Xcode")
        target_sources(${CMAKE_PROJECT_NAME} PRIVATE "${_qgc_ios_asset_catalog}")
        set_source_files_properties("${_qgc_ios_asset_catalog}" PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    else()
        if(CMAKE_OSX_SYSROOT MATCHES "[Ss]imulator")
            set(_qgc_ios_platform iphonesimulator)
        else()
            set(_qgc_ios_platform iphoneos)
        endif()

        file(GLOB_RECURSE _qgc_ios_bundle_inputs CONFIGURE_DEPENDS "${_qgc_ios_asset_catalog}/*")
        list(APPEND _qgc_ios_bundle_inputs "${_qgc_ios_launch_screen}"
             "${CMAKE_SOURCE_DIR}/deploy/ios/prepare-bundle.sh"
        )
        set_property(
            TARGET ${CMAKE_PROJECT_NAME}
            APPEND
            PROPERTY LINK_DEPENDS ${_qgc_ios_bundle_inputs}
        )

        add_custom_command(
            TARGET ${CMAKE_PROJECT_NAME}
            POST_BUILD
            COMMAND /bin/bash "${CMAKE_SOURCE_DIR}/deploy/ios/prepare-bundle.sh"
                    "$<TARGET_BUNDLE_DIR:${CMAKE_PROJECT_NAME}>" "${QGC_IOS_DEPLOYMENT_TARGET}" "${_qgc_ios_platform}"
            COMMENT "Compiling iOS launch screen and asset catalog"
            VERBATIM
        )
    endif()

    if(COMMAND qt_add_ios_ffmpeg_libraries)
        qt_add_ios_ffmpeg_libraries(${CMAKE_PROJECT_NAME})
    endif()

    # Ninja does not run Xcode's Embed Frameworks phase. Copy the linked Qt
    # FFmpeg frameworks into the application bundle explicitly.
    if(NOT CMAKE_GENERATOR MATCHES "Xcode")
        cmake_path(GET Qt6_DIR PARENT_PATH _qt_cmake_dir)
        cmake_path(GET _qt_cmake_dir PARENT_PATH _qt_lib_dir)
        set(_ffmpeg_xcframework_dir "${_qt_lib_dir}/ffmpeg")

        if(EXISTS "${_ffmpeg_xcframework_dir}")
            file(
                GLOB _ffmpeg_xcframeworks
                LIST_DIRECTORIES true
                "${_ffmpeg_xcframework_dir}/*.xcframework"
            )
            foreach(xcframework IN LISTS _ffmpeg_xcframeworks)
                cmake_path(GET xcframework STEM _framework_name)
                set(_xcframework_slice_args XCFRAMEWORK "${xcframework}" PLATFORM "${_qgc_ios_platform}" OUT_VAR
                                            _framework_source
                )
                if(CMAKE_OSX_ARCHITECTURES)
                    list(APPEND _xcframework_slice_args ARCHITECTURES ${CMAKE_OSX_ARCHITECTURES})
                endif()
                qgc_find_ios_xcframework_slice(${_xcframework_slice_args})
                add_custom_command(
                    TARGET ${CMAKE_PROJECT_NAME}
                    POST_BUILD
                    COMMAND "${CMAKE_COMMAND}" -E copy_directory "${_framework_source}"
                            "$<TARGET_BUNDLE_DIR:${CMAKE_PROJECT_NAME}>/Frameworks/${_framework_name}.framework"
                    COMMENT "Embedding ${_framework_name}.framework (${_qgc_ios_platform})"
                    VERBATIM
                )
            endforeach()
            message(STATUS "QGC: FFmpeg xcframeworks will be embedded at build time (Ninja)")
        else()
            message(STATUS "QGC: No FFmpeg xcframeworks found at ${_ffmpeg_xcframework_dir}")
        endif()

        set_property(
            TARGET ${CMAKE_PROJECT_NAME}
            APPEND
            PROPERTY BUILD_RPATH "@executable_path/Frameworks"
        )
    endif()

    # GStreamerMobileXcfw is created while processing add_subdirectory(src),
    # so register its compatibility source and embedding after the root
    # directory is complete.
    cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}" CALL _qgc_ios_finalize_gstreamer_mobile
                   "${CMAKE_PROJECT_NAME}"
    )

    message(STATUS "QGC: iOS platform configuration applied")
endif()
