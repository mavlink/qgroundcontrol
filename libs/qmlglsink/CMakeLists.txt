
find_package(Qt5Gui ${QT_VERSION} CONFIG REQUIRED Private)
find_package(OpenGL REQUIRED)

add_library(qmlglsink
    gstreamer/subprojects/gst-plugins-good/ext/qt/gstplugin.cc
    gstreamer/subprojects/gst-plugins-good/ext/qt/gstqsgtexture.cc
    gstreamer/subprojects/gst-plugins-good/ext/qt/gstqtelement.cc
    gstreamer/subprojects/gst-plugins-good/ext/qt/gstqtglutility.cc
    gstreamer/subprojects/gst-plugins-good/ext/qt/gstqtoverlay.cc
    gstreamer/subprojects/gst-plugins-good/ext/qt/gstqtsink.cc
    gstreamer/subprojects/gst-plugins-good/ext/qt/gstqtsrc.cc
    gstreamer/subprojects/gst-plugins-good/ext/qt/qtglrenderer.cc
    gstreamer/subprojects/gst-plugins-good/ext/qt/qtitem.cc
    gstreamer/subprojects/gst-plugins-good/ext/qt/qtwindow.cc

    gstreamer/subprojects/gst-plugins-good/ext/qt/gstqsgtexture.h
    gstreamer/subprojects/gst-plugins-good/ext/qt/gstqtelements.h
    gstreamer/subprojects/gst-plugins-good/ext/qt/gstqtgl.h
    gstreamer/subprojects/gst-plugins-good/ext/qt/gstqtglutility.h
    gstreamer/subprojects/gst-plugins-good/ext/qt/gstqtoverlay.h
    gstreamer/subprojects/gst-plugins-good/ext/qt/gstqtsink.h
    gstreamer/subprojects/gst-plugins-good/ext/qt/gstqtsrc.h
    gstreamer/subprojects/gst-plugins-good/ext/qt/qtglrenderer.h
    gstreamer/subprojects/gst-plugins-good/ext/qt/qtitem.h
    gstreamer/subprojects/gst-plugins-good/ext/qt/qtwindow.h
)

if(LINUX)
	target_compile_definitions(qmlglsink PUBLIC HAVE_QT_X11 HAVE_QT_EGLFS HAVE_QT_WAYLAND)

	find_package(Qt5 ${QT_VERSION}
		COMPONENTS
			X11Extras
		REQUIRED
		HINTS
			${QT_LIBRARY_HINTS}
	)

	target_link_libraries(qmlglsink
		PUBLIC
			Qt5::X11Extras
	)

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
		Qt5::Core
		Qt5::OpenGL
		Qt5::GuiPrivate
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
