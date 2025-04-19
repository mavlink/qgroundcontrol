if(NOT ANDROID)
    return()
endif()

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        AndroidInit.cc
        AndroidInterface.cc
        AndroidInterface.h
        AndroidSerial.cc
        AndroidSerial.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(qtandroidserialport)
