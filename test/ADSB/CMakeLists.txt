target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        ADSBTest.cc
        ADSBTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# qt_add_resources(${CMAKE_PROJECT_NAME} "ADSBTest_res"
#     PREFIX "/"
#     FILES
#         ADSB_Simulator.py
# )
