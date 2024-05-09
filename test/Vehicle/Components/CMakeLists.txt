find_package(Qt6 REQUIRED COMPONENTS Core Test)

qt_add_library(VehicleComponentsTest
	STATIC
		ComponentInformationCacheTest.cc
		ComponentInformationCacheTest.h
		ComponentInformationTranslationTest.cc
		ComponentInformationTranslationTest.h
)

target_link_libraries(VehicleComponentsTest
	PRIVATE
		Qt6::Test
		Utilities
		VehicleComponents
	PUBLIC
		Qt6::Core
		qgcunittest
)

qt_add_resources(VehicleComponentsTest
	"VehicleComponentsTest_res"
	FILES
		TranslationTest.json
		TranslationTest_de_DE.ts
)

target_include_directories(VehicleComponentsTest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
