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
cmake_path(GET QGC_MACOS_ICON_PATH FILENAME MACOSX_BUNDLE_ICON_FILE)

if(IOS)
    set(_qgc_bundle_plist "${CMAKE_SOURCE_DIR}/deploy/ios/Info.plist.app.in")
else()
    set(_qgc_bundle_plist "${QGC_MACOS_PLIST_PATH}")
endif()

set_target_properties(
    ${CMAKE_PROJECT_NAME}
    PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${_qgc_bundle_plist}"
               MACOSX_BUNDLE_BUNDLE_NAME "${CMAKE_PROJECT_NAME}"
               MACOSX_BUNDLE_BUNDLE_VERSION "${CMAKE_PROJECT_VERSION}"
               MACOSX_BUNDLE_COPYRIGHT "${QGC_APP_COPYRIGHT}"
               MACOSX_BUNDLE_GUI_IDENTIFIER "${QGC_MACOS_BUNDLE_ID}"
               MACOSX_BUNDLE_ICON_FILE "${MACOSX_BUNDLE_ICON_FILE}"
               MACOSX_BUNDLE_INFO_STRING "${CMAKE_PROJECT_DESCRIPTION}"
               MACOSX_BUNDLE_LONG_VERSION_STRING
               "${CMAKE_PROJECT_VERSION_MAJOR}.${CMAKE_PROJECT_VERSION_MINOR}.${CMAKE_PROJECT_VERSION_PATCH}"
               MACOSX_BUNDLE_SHORT_VERSION_STRING "${CMAKE_PROJECT_VERSION_MAJOR}.${CMAKE_PROJECT_VERSION_MINOR}"
)

# ----------------------------------------------------------------------------
# Platform-Specific Configuration
# ----------------------------------------------------------------------------
if(MACOS)
    # macOS-specific configuration
    set(_app_icon_macos "${QGC_MACOS_ICON_PATH}")
    set_source_files_properties(${_app_icon_macos} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

    set(_app_entitlements_macos "${QGC_MACOS_ENTITLEMENTS_PATH}")
    set_source_files_properties(${_app_entitlements_macos} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

    target_sources(${CMAKE_PROJECT_NAME} PRIVATE "${_app_entitlements_macos}" "${_app_icon_macos}")

    message(STATUS "QGC: macOS platform configuration applied")
elseif(IOS)
    # _qgc_ios_embed_gstreamer_mobile(target)
    # Adds the Ninja-only post-build copy after the framework target exists.
    function(_qgc_ios_embed_gstreamer_mobile target)
        if(NOT TARGET GStreamerMobileXcfw)
            return()
        endif()

        add_custom_command(
            TARGET "${target}"
            POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_directory "$<TARGET_FILE_DIR:GStreamerMobileXcfw>"
                    "$<TARGET_BUNDLE_DIR:${target}>/Frameworks/$<TARGET_FILE_BASE_NAME:GStreamerMobileXcfw>.framework"
            COMMENT "Embedding GStreamerMobile framework"
            VERBATIM
        )
        message(STATUS "QGC: GStreamerMobile will be embedded at build time (Ninja)")
    endfunction()

    # iOS-specific configuration
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
                   XCODE_ATTRIBUTE_MARKETING_VERSION "${CMAKE_PROJECT_VERSION_MAJOR}.${CMAKE_PROJECT_VERSION_MINOR}"
                   XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME "AppIcon"
                   XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "${QGC_IOS_DEPLOYMENT_TARGET}"
                   XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "${QGC_IOS_TARGETED_DEVICE_FAMILY}"
                   XCODE_ATTRIBUTE_INFOPLIST_KEY_CFBundleDisplayName ${CMAKE_PROJECT_NAME}
                   XCODE_ATTRIBUTE_INFOPLIST_KEY_LSApplicationCategoryType "public.app-category.navigation"
                   XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "YES"
    )

    if(QGC_IOS_APP_STORE_BUILD)
        if(NOT CMAKE_GENERATOR STREQUAL "Xcode")
            message(FATAL_ERROR "QGC_IOS_APP_STORE_BUILD requires the Xcode generator")
        endif()
        if(NOT QGC_IOS_DEVELOPMENT_TEAM OR NOT QGC_IOS_PROVISIONING_PROFILE)
            message(
                FATAL_ERROR
                    "QGC_IOS_APP_STORE_BUILD requires QGC_IOS_DEVELOPMENT_TEAM and QGC_IOS_PROVISIONING_PROFILE"
            )
        endif()

        set_target_properties(
            ${CMAKE_PROJECT_NAME}
            PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Manual"
                       XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Apple Distribution"
                       XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "YES"
                       XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "YES"
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
    # FFmpeg frameworks and the project-built GStreamerMobile framework into
    # the application bundle explicitly.
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
                set(_xcframework_slice_args
                    XCFRAMEWORK "${xcframework}"
                    PLATFORM "${_qgc_ios_platform}"
                    OUT_VAR _framework_source
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

        # GStreamerMobileXcfw is created while processing add_subdirectory(src),
        # so register its post-build copy after the root directory is complete.
        cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}" CALL _qgc_ios_embed_gstreamer_mobile
                       "${CMAKE_PROJECT_NAME}"
        )
    endif()

    message(STATUS "QGC: iOS platform configuration applied")
endif()
