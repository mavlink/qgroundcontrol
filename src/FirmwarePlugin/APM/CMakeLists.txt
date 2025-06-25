# 定义一个变量保存 APM 固件插件相关的源码和头文件
set(APM_PLUGIN_SOURCES
        APMFirmwarePluginFactory.cc
        APMFirmwarePlugin.cc
        APMParameterMetaData.cc
        ArduCopterFirmwarePlugin.cc
        ArduPlaneFirmwarePlugin.cc
        ArduRoverFirmwarePlugin.cc
        ArduSubFirmwarePlugin.cc
)

set(APM_PLUGIN_HEADERS
        APMFirmwarePluginFactory.h
        APM.h
        APMFirmwarePlugin.h
        APMParameterMetaData.h
        ArduCopterFirmwarePlugin.h
        ArduPlaneFirmwarePlugin.h
        ArduRoverFirmwarePlugin.h
        ArduSubFirmwarePlugin.h
)

# 根据宏是否禁用决定是否创建库
if(NOT QGC_DISABLE_APM_PLUGIN_FACTORY AND NOT QGC_DISABLE_APM_PLUGIN)

    add_library(APMFirmwarePlugin STATIC
            ${APM_PLUGIN_SOURCES}
            ${APM_PLUGIN_HEADERS}  # 头文件可以写在这里，方便 IDE 识别
    )

    target_include_directories(APMFirmwarePlugin PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

    # 这里根据需要，给静态库添加依赖库，比如 Qt
    # target_link_libraries(APMFirmwarePlugin PUBLIC Qt6::Core ...)

    # 主程序链接这个库
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE APMFirmwarePlugin)

else()
    # 如果禁用了插件，则不创建库，源码不加入主程序
    message(STATUS "APM Plugin disabled, skipping APMFirmwarePlugin library creation")
endif()

# 仍然可以单独添加其他需要的头文件路径
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# 其他宏定义依然保持
if(QGC_DISABLE_APM_MAVLINK)
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE QGC_NO_ARDUPILOT_DIALECT)
endif()

#if(NOT QGC_DISABLE_APM_PLUGIN_FACTORY)
#    target_sources(${CMAKE_PROJECT_NAME}
#        PRIVATE
#            APMFirmwarePluginFactory.cc
#            APMFirmwarePluginFactory.h
#    )
#endif()
#
#if(NOT QGC_DISABLE_APM_PLUGIN)
#    target_sources(${CMAKE_PROJECT_NAME}
#        PRIVATE
#            APM.h
#            APMFirmwarePlugin.cc
#            APMFirmwarePlugin.h
#            APMParameterMetaData.cc
#            APMParameterMetaData.h
#            ArduCopterFirmwarePlugin.cc
#            ArduCopterFirmwarePlugin.h
#            ArduPlaneFirmwarePlugin.cc
#            ArduPlaneFirmwarePlugin.h
#            ArduRoverFirmwarePlugin.cc
#            ArduRoverFirmwarePlugin.h
#            ArduSubFirmwarePlugin.cc
#            ArduSubFirmwarePlugin.h
#    )
#endif()
#
#target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
#
#if(QGC_DISABLE_APM_MAVLINK)
#    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE QGC_NO_ARDUPILOT_DIALECT)
#endif()
#
## CPMAddPackage(
##     NAME ArduPilotParams
##     GITHUB_REPOSITORY ArduPilot/ParameterRepository
##     GIT_TAG main
## )
