# ----------------------------------------------------------------------------
# QGroundControl Apple Platform Configuration (macOS and iOS)
# ----------------------------------------------------------------------------

if(NOT APPLE)
    message(FATAL_ERROR "QGC: Invalid Platform: Apple.cmake included but platform is not Apple")
endif()

if(CMAKE_GENERATOR STREQUAL "Xcode")
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

set_target_properties(${CMAKE_PROJECT_NAME}
    PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${QGC_MACOS_PLIST_PATH}"
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
    # iOS-specific configuration
    enable_language(OBJC)

    set(QT_IOS_LAUNCH_SCREEN "${CMAKE_SOURCE_DIR}/deploy/ios/QGCLaunchScreen.xib")

    # set(CMAKE_XCODE_ATTRIBUTE_ARCHS
    # set(CMAKE_XCODE_ATTRIBUTE_ENABLE_BITCODE
    set(CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "14.0")
    set(CMAKE_XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2") # iPhone,iPad
    set(CMAKE_XCODE_ATTRIBUTE_INFOPLIST_FILE "${CMAKE_SOURCE_DIR}/deploy/ios/iOS-Info.plist")

    set_target_properties(${CMAKE_PROJECT_NAME}
        PROPERTIES
            QT_IOS_LAUNCH_SCREEN "${CMAKE_SOURCE_DIR}/deploy/ios/QGCLaunchScreen.xib"
            XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${QGC_PACKAGE_NAME}"
            XCODE_ATTRIBUTE_PRODUCT_NAME "${CMAKE_PROJECT_NAME}"
            XCODE_ATTRIBUTE_CURRENT_PROJECT_VERSION ${CMAKE_PROJECT_VERSION}
            XCODE_ATTRIBUTE_MARKETING_VERSION "${CMAKE_PROJECT_VERSION_MAJOR}.${CMAKE_PROJECT_VERSION_MINOR}"
            XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME "AppIcon"
            XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "14.0"
            XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2" # iPhone,iPad
            XCODE_ATTRIBUTE_INFOPLIST_KEY_CFBundleDisplayName ${CMAKE_PROJECT_NAME}
            XCODE_ATTRIBUTE_INFOPLIST_KEY_LSApplicationCategoryType "public.app-category.mycategory"
            XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "YES"
    )

    # Add FFmpeg libraries for iOS if needed
    # set(QT_NO_FFMPEG_XCODE_EMBED_FRAMEWORKS_CODE_SIGN_ON_COPY ON)
    qt_add_ios_ffmpeg_libraries(${CMAKE_PROJECT_NAME})

    message(STATUS "QGC: iOS platform configuration applied")
endif()
