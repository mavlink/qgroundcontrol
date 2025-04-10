set(QGC_SOURCES
    ${QGC_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/FTPManagerTest.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/FTPManagerTest.h
    ${CMAKE_CURRENT_SOURCE_DIR}/InitialConnectTest.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/InitialConnectTest.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MAVLinkLogManagerTest.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/MAVLinkLogManagerTest.h
    ${CMAKE_CURRENT_SOURCE_DIR}/RequestMessageTest.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/RequestMessageTest.h
    ${CMAKE_CURRENT_SOURCE_DIR}/SendMavCommandWithHandlerTest.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/SendMavCommandWithHandlerTest.h
    ${CMAKE_CURRENT_SOURCE_DIR}/SendMavCommandWithSignallingTest.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/SendMavCommandWithSignallingTest.h
    ${CMAKE_CURRENT_SOURCE_DIR}/VehicleLinkManagerTest.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/VehicleLinkManagerTest.h
    CACHE INTERNAL "" FORCE
)

set(QGC_DIRECTORIES
    ${QGC_DIRECTORIES}
    ${CMAKE_CURRENT_SOURCE_DIR}
    CACHE INTERNAL "" FORCE
)

add_subdirectory(ComponentInformation)
