
find_package(Qt6Gui ${QT_VERSION}  REQUIRED )
find_package(Qt6 ${QT_VERSION}  REQUIRED )
find_package(OpenGL REQUIRED)

add_library(qmlglsink
	gst-plugins-good/ext/qt6/gstplugin.cc
    gst-plugins-good/ext/qt6/gstqml6glsink.cc
    gst-plugins-good/ext/qt6/gstqsg6glnode.cc
    gst-plugins-good/ext/qt6/gstqt6element.cc
    gst-plugins-good/ext/qt6/gstqt6glutility.cc
    gst-plugins-good/ext/qt6/qt6glitem.cc
    gst-plugins-good/ext/qt6/gstqt6gl.h
)

if(LINUX)
	target_compile_definitions(qmlglsink PUBLIC HAVE_QT_X11 HAVE_QT_EGLFS HAVE_QT_WAYLAND HAVE_QT_QPA_HEADER)
elseif(APPLE)
	target_compile_definitions(qmlglsink PUBLIC HAVE_QT_MAC)
elseif(IOS)
	target_compile_definitions(qmlglsink PUBLIC HAVE_QT_MAC)
elseif(WIN32)
	target_compile_definitions(qmlglsink PUBLIC HAVE_QT_WIN32 HAVE_QT_QPA_HEADER)

	target_link_libraries(qmlglsink PUBLIC user32.lib)
	target_link_libraries(qmlglsink PUBLIC OpenGL::GL)
elseif(ANDROID)
	target_compile_definitions(qmlglsink PUBLIC HAVE_QT_ANDROID)
endif()

target_link_libraries(qmlglsink
	PUBLIC
		Qt6::Core
		Qt6::OpenGL
		Qt6::GuiPrivate
		${GST_LINK_LIBRARIES}
)

target_include_directories(qmlglsink PRIVATE ${GST_INCLUDE_DIRS})
if (MSVC)
	target_include_directories(qmlglsink PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/windows)
else()
	target_compile_options(qmlglsink
			PRIVATE
			-Wno-unused-parameter
			-Wno-implicit-fallthrough
			)
endif()
