find_package(Qt6 REQUIRED COMPONENTS Core Test)

target_sources(VehicleTest
    PRIVATE
        ComponentInformationCacheTest.cc
        ComponentInformationCacheTest.h
        ComponentInformationTranslationTest.cc
        ComponentInformationTranslationTest.h
)

target_link_libraries(VehicleTest
    PRIVATE
        Qt6::Test
        Utilities
    PUBLIC
        Qt6::Core
        qgcunittest
)

qt_add_resources(VehicleTest
    "VehicleTest_ComponentInformation_res"
    FILES
        TranslationTest.json
        TranslationTest_de_DE.ts
)

target_include_directories(VehicleTest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
