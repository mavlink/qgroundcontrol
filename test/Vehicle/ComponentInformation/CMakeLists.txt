target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        ComponentInformationCacheTest.cc
        ComponentInformationCacheTest.h
        ComponentInformationTranslationTest.cc
        ComponentInformationTranslationTest.h
)

# qt_add_resources(${CMAKE_PROJECT_NAME}
#     "VehicleTest_ComponentInformation_res"
#     FILES
#         TranslationTest.json
#         TranslationTest_de_DE.ts
# )

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
