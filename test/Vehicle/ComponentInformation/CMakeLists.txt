find_package(Qt6 REQUIRED COMPONENTS Core Test)

target_sources(qgctest
    PRIVATE
        ComponentInformationCacheTest.cc
        ComponentInformationCacheTest.h
        ComponentInformationTranslationTest.cc
        ComponentInformationTranslationTest.h
)

target_link_libraries(qgctest
    PRIVATE
        Qt6::Test
    PUBLIC
        Qt6::Core
)

qt_add_resources(qgctest
    "VehicleTest_ComponentInformation_res"
    FILES
        TranslationTest.json
        TranslationTest_de_DE.ts
)

target_include_directories(qgctest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
