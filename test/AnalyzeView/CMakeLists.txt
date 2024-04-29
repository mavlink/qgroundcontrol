find_package(Qt6 REQUIRED COMPONENTS Core)

qt_add_library(AnalyzeViewTest
	STATIC
		LogDownloadTest.cc
		LogDownloadTest.h
)

target_link_libraries(AnalyzeViewTest
        PRIVATE
        	Qt6::Core
                AnalyzeView
	PUBLIC
		qgcunittest
)

target_include_directories(AnalyzeViewTest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

