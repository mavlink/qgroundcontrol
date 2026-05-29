# ----------------------------------------------------------------------------
# QGroundControl Apple Platform Configuration (macOS and iOS)
# ----------------------------------------------------------------------------

if(NOT APPLE)
    message(FATAL_ERROR "QGC: Invalid Platform: Apple.cmake included but platform is not Apple")
endif()

# .mm sources (e.g. GstIOSurfaceVideoBuffer.mm) need OBJCXX rules at root scope so
# CMAKE_OBJCXX_COMPILE_OBJECT is populated for the QGC target. Done here unconditionally
# for macOS + iOS rather than relying on a deferred enable_language inside Find modules.
enable_language(OBJC OBJCXX)

if(CMAKE_GENERATOR STREQUAL "Xcode" AND MACOS)
    # set(CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM
    # set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_STYLE
    # set(CMAKE_XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER
    # set(CMAKE_XCODE_ATTRIBUTE_INSTALL_PATH
    # set(CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT
    # set(CMAKE_XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS
    # set(CMAKE_XCODE_ATTRIBUTE_LD_ENTRY_POINT
    # set(CMAKE_XCODE_ATTRIBUTE_MARKETING_VERSION
    # set(CMAKE_XCODE_ATTRIBUTE_CURRENT_PROJECT_VERSION
    set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${QGC_MACOS_ENTITLEMENTS_PATH}")
endif()

# ----------------------------------------------------------------------------
# macOS/iOS Bundle Configuration
# ----------------------------------------------------------------------------
cmake_path(GET QGC_MACOS_ICON_PATH FILENAME MACOSX_BUNDLE_ICON_FILE)

if(IOS)
    set(_qgc_bundle_plist "${CMAKE_SOURCE_DIR}/deploy/ios/iOSBundleInfo.plist.in")
else()
    set(_qgc_bundle_plist "${QGC_MACOS_PLIST_PATH}")
endif()

set_target_properties(${CMAKE_PROJECT_NAME}
    PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "${_qgc_bundle_plist}"
        MACOSX_BUNDLE_BUNDLE_NAME "${CMAKE_PROJECT_NAME}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${CMAKE_PROJECT_VERSION}"
        MACOSX_BUNDLE_COPYRIGHT "${QGC_APP_COPYRIGHT}"
        MACOSX_BUNDLE_GUI_IDENTIFIER "${QGC_MACOS_BUNDLE_ID}"
        MACOSX_BUNDLE_ICON_FILE "${MACOSX_BUNDLE_ICON_FILE}"
        MACOSX_BUNDLE_INFO_STRING "${CMAKE_PROJECT_DESCRIPTION}"
        MACOSX_BUNDLE_LONG_VERSION_STRING "${CMAKE_PROJECT_VERSION_MAJOR}.${CMAKE_PROJECT_VERSION_MINOR}.${CMAKE_PROJECT_VERSION_PATCH}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${CMAKE_PROJECT_VERSION_MAJOR}.${CMAKE_PROJECT_VERSION_MINOR}"
)

# ----------------------------------------------------------------------------
# Platform-Specific Configuration
# ----------------------------------------------------------------------------
if(MACOS)
    # macOS-specific configuration
    set(app_icon_macos "${QGC_MACOS_ICON_PATH}")
    set_source_files_properties(${app_icon_macos} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

    set(app_entitlements_macos "${QGC_MACOS_ENTITLEMENTS_PATH}")
    set_source_files_properties(${app_entitlements_macos} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

    target_sources(${CMAKE_PROJECT_NAME}
        PRIVATE
            "${app_entitlements_macos}"
            "${app_icon_macos}"
    )

    message(STATUS "QGC: macOS platform configuration applied")
elseif(IOS)
    function(_qgc_ios_embed_gstreamer_mobile target)
        if(NOT TARGET GStreamerMobile)
            return()
        endif()
        add_custom_command(TARGET "${target}" POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "$<TARGET_FILE_DIR:GStreamerMobile>"
                "$<TARGET_BUNDLE_DIR:${target}>/Frameworks/gstreamer_mobile.framework"
            COMMENT "Embedding gstreamer_mobile.framework"
            VERBATIM
        )
        message(STATUS "QGC: GStreamerMobile will be embedded at build time (Ninja)")
    endfunction()
    # iOS-specific configuration

    # set(CMAKE_XCODE_ATTRIBUTE_ARCHS
    # set(CMAKE_XCODE_ATTRIBUTE_ENABLE_BITCODE
    set(CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "${QGC_IOS_DEPLOYMENT_TARGET}")
    set(CMAKE_XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "${QGC_IOS_TARGETED_DEVICE_FAMILY}")
    set(CMAKE_XCODE_ATTRIBUTE_INFOPLIST_FILE "${CMAKE_SOURCE_DIR}/deploy/ios/iOS-Info.plist")

    set_target_properties(${CMAKE_PROJECT_NAME}
        PROPERTIES
            QT_IOS_LAUNCH_SCREEN "${CMAKE_SOURCE_DIR}/deploy/ios/QGCLaunchScreen.xib"
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

    # Add FFmpeg libraries for iOS if needed
    # set(QT_NO_FFMPEG_XCODE_EMBED_FRAMEWORKS_CODE_SIGN_ON_COPY ON)
    if(COMMAND qt_add_ios_ffmpeg_libraries)
        qt_add_ios_ffmpeg_libraries(${CMAKE_PROJECT_NAME})
    endif()

    # With Ninja generator, Xcode's "Embed Frameworks" build phase doesn't run.
    # Manually copy FFmpeg xcframeworks into the bundle and set rpath so dyld
    # finds them on device instead of the CI runner's absolute Qt path.
    if(NOT CMAKE_GENERATOR MATCHES "Xcode")
        cmake_path(GET Qt6_DIR PARENT_PATH _qt_cmake_dir)
        cmake_path(GET _qt_cmake_dir PARENT_PATH _qt_lib_dir)
        set(_ffmpeg_xcfw_dir "${_qt_lib_dir}/ffmpeg")

        if(EXISTS "${_ffmpeg_xcfw_dir}")
            file(GLOB _xcframeworks LIST_DIRECTORIES true "${_ffmpeg_xcfw_dir}/*.xcframework")
            foreach(_xcfw IN LISTS _xcframeworks)
                cmake_path(GET _xcfw STEM _fw_name)
                foreach(_slice ios-arm64 ios-arm64_arm64e)
                    set(_fw_src "${_xcfw}/${_slice}/${_fw_name}.framework")
                    if(EXISTS "${_fw_src}")
                        add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
                            COMMAND ${CMAKE_COMMAND} -E copy_directory
                                "${_fw_src}"
                                "$<TARGET_BUNDLE_DIR:${CMAKE_PROJECT_NAME}>/Frameworks/${_fw_name}.framework"
                            COMMENT "Embedding ${_fw_name}.framework"
                            VERBATIM
                        )
                        break()
                    endif()
                endforeach()
            endforeach()
            message(STATUS "QGC: FFmpeg xcframeworks will be embedded at build time (Ninja)")
        else()
            message(STATUS "QGC: No FFmpeg xcframeworks found at ${_ffmpeg_xcfw_dir}")
        endif()

        set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
            BUILD_RPATH "@executable_path/Frameworks"
        )

        # GStreamerMobile is created by find_package(GStreamerMobile) inside
        # add_subdirectory(src), which hasn't run yet. Defer the post-build
        # copy until after all subdirectories are processed.
        set(_qgc_target "${CMAKE_PROJECT_NAME}")
        cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}"
            CALL _qgc_ios_embed_gstreamer_mobile "${_qgc_target}")
    endif()

    message(STATUS "QGC: iOS platform configuration applied")
endif()
