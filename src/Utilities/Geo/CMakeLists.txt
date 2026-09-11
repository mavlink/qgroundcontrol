qt_add_library(QGCGeoMath STATIC QGCGeo.cc QGCGeo.h)

include(GeographicLib.cmake)

# geographiclib_ADDED is set only when CPM downloads; geographiclib_FOUND when
# resolved via find_package (QGC_USE_SYSTEM_LIBS). Either is success.
if(NOT TARGET GeographicLib::GeographicLib)
    message(FATAL_ERROR "QGC: geographiclib (required dependency) was not resolved")
endif()

# Only the CPM-built static target needs warning suppression; the system import
# target (GeographicLib::GeographicLib) carries no QGC-controlled warning flags.
if(TARGET GeographicLib_STATIC)
    qgc_disable_dependency_warnings(GeographicLib_STATIC)
endif()

target_link_libraries(
    QGCGeoMath
    PUBLIC Qt6::Core Qt6::Gui Qt6::Positioning
    PRIVATE GeographicLib::GeographicLib QGCLogging
)

target_include_directories(QGCGeoMath PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCGeoMath)

add_subdirectory(Formats)
