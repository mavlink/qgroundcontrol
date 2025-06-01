target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        ShapeTest.cc
        ShapeTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# qt_add_resources(${CMAKE_PROJECT_NAME} "Test_Utilities_Shape_res"
#     PREFIX "/unittest/Shape"
#     FILES
#         pline.dbf
#         pline.prj
#         pline.shp
#         pline.shx
#         polygon.dbf
#         polygon.prj
#         polygon.shp
#         polygon.shx
#         polyline.kml
#         polygon.kml
# )
