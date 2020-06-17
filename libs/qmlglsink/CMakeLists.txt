
find_package(Qt5Gui ${QT_VERSION} CONFIG REQUIRED Private)

add_library(qmlglsink
    gst-plugins-good/ext/qt/gstplugin.cc
    gst-plugins-good/ext/qt/gstqtglutility.cc
    gst-plugins-good/ext/qt/gstqsgtexture.cc
    gst-plugins-good/ext/qt/gstqtsink.cc
    gst-plugins-good/ext/qt/gstqtsrc.cc
    gst-plugins-good/ext/qt/qtwindow.cc
    gst-plugins-good/ext/qt/qtitem.cc

    gst-plugins-good/ext/qt/gstqsgtexture.h
    gst-plugins-good/ext/qt/gstqtgl.h
    gst-plugins-good/ext/qt/gstqtglutility.h
    gst-plugins-good/ext/qt/gstqtsink.h
    gst-plugins-good/ext/qt/gstqtsrc.h
    gst-plugins-good/ext/qt/qtwindow.h
    gst-plugins-good/ext/qt/qtitem.h
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

	# TODO: use FindOpenGL?
	target_link_libraries(qmlglsink PUBLIC opengl32.lib user32.lib)
	# LIBS += opengl32.lib user32.lib
elseif(ANDROID)
	target_compile_definitions(qmlglsink PUBLIC HAVE_QT_ANDROID)
endif()

target_link_libraries(qmlglsink
	PUBLIC
		Qt5::Core
		Qt5::OpenGL
                Qt5::GuiPrivate
)

target_compile_options(qmlglsink
	PRIVATE
		-Wno-unused-parameter
		-Wno-implicit-fallthrough
)
