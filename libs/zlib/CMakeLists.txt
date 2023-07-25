add_library(z STATIC)
target_link_libraries(z PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/windows/lib/zlibstat.lib)
target_include_directories(z PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/windows/include)