
qt_add_library(AnalyzeViewTest
	STATIC
		LogDownloadTest.cc LogDownloadTest.h
)

target_link_libraries(AnalyzeViewTest
        PRIVATE
                AnalyzeView
	PUBLIC
		qgc
		qgcunittest
)

target_include_directories(AnalyzeViewTest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

