if(NOT ANDROID)
    return()
endif()

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        qserialport.cpp
        qserialport.h
        qserialport_android.cpp
        qserialport_p.h
        qserialportglobal.h
        qserialportinfo.cpp
        qserialportinfo.h
        qserialportinfo_p.h
        qserialportinfo_android.cpp
        qtserialportexports.h
        qtserialportversion.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE QIODEVICE_DEBUG)
