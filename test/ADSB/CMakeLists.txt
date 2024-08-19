find_package(Qt6 REQUIRED COMPONENTS Core Test)

qt_add_library(ADSBTest
    STATIC
        ADSBTest.cc
        ADSBTest.h
)

target_link_libraries(ADSBTest
    PRIVATE
        Qt6::Core
        Qt6::Test
        ADSB
        QmlControls
    PUBLIC
        qgcunittest
)

target_include_directories(ADSBTest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

qt_add_resources(ADSBTest "ADSBTest_res"
    PREFIX "/"
    FILES
        ADSB_Simulator.py
)
