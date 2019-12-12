sources = [
  'gstplugin.cc',
  'gstqsgtexture.cc',
  'gstqtglutility.cc',
  'gstqtsink.cc',
  'gstqtsrc.cc',
  'qtitem.cc',
  'qtwindow.cc',
]

moc_headers = [
  'qtitem.h',
  'qtwindow.h',
  'gstqsgtexture.h',
]

# FIXME: -Dqt5=enabled is silently ignored if a c++ compiler is not found
if have_cxx and build_gstgl
  qt5_mod = import('qt5')
  qt5qml_dep = dependency('qt5', modules : ['Core', 'Gui', 'Qml', 'Quick'],
                          required: get_option('qt5'), static: host_machine.system() == 'ios')

  # On Linux, distros often have the Qt5 pkg-config files and moc in separate
  # packages, so the user may not have both installed. Check for moc and ensure
  # that it's installed.
  # We don't do this check on other OSes because they need to be able to simply
  # point the `QMAKE` env var to `qmake` to build against a particular Qt5.
  if host_system == 'linux'
    moc = find_program('moc-qt5', 'moc', required : get_option('qt5'))
  else
    # We only check if `moc` was found, and then discard it, so we can fake it.
    # This is also a good unit test of the fact that we *don't* use it.
    moc = declare_dependency()
  endif
  if qt5qml_dep.found() and moc.found()
    optional_deps = []
    qt_defines = []
    have_qpa_include = false
    have_qt_windowing = false

    # Attempt to find the QPA header either through pkg-config (preferred) or qmake
    # This semi-matches what meson does internally with the qt5 module
    # FIXME Add a way to get some of this information out of the qt5 module
    if not have_qpa_include
      # FIXME: automagic
      qt5core_dep = dependency('Qt5Core', required: false)
      if qt5core_dep.found() and qt5core_dep.type_name() == 'pkgconfig'
        qt_version = qt5core_dep.version()
        qt_include_dir = qt5core_dep.get_pkgconfig_variable('includedir')
        qpa_include_path = join_paths(qt_include_dir, 'QtGui', qt_version, 'QtGui')
        if cxx.has_header('qpa/qplatformnativeinterface.h',
                dependencies : qt5core_dep,
                args : '-I' + qpa_include_path)
          qt_defines += '-DHAVE_QT_QPA_HEADER'
          qt_defines += '-I' + qpa_include_path
          have_qpa_include = true
          message('Found QPA header using pkg-config')
        endif
      endif
    endif
    if not have_qpa_include
      qmake = find_program('qmake-qt5', 'qmake')
      if qmake.found()
        qt_version = run_command(qmake, '-query', 'QT_VERSION').stdout().strip()
        qt_include_dir = run_command(qmake, '-query', 'QT_INSTALL_HEADERS').stdout().strip()
        qpa_include_path = join_paths(qt_include_dir, 'QtGui', qt_version, 'QtGui')
        if cxx.has_header('qpa/qplatformnativeinterface.h',
                args : '-I' + qpa_include_path)
          qt_defines += '-DHAVE_QT_QPA_HEADER'
          qt_defines += '-I' + qpa_include_path
          have_qpa_include = true
          message('Found QPA header using qmake')
        endif
      endif
    endif

    # Try to come up with all the platform/winsys combinations that will work

    if gst_gl_have_window_x11 and gst_gl_have_platform_glx
      # FIXME: automagic
      qt5x11extras = dependency('qt5', modules : ['X11Extras'], required : false)
      if qt5x11extras.found()
        optional_deps += qt5x11extras
        qt_defines += ['-DHAVE_QT_X11']
        have_qt_windowing = true
      endif
    endif

    if gst_gl_have_platform_egl
      # Embedded linux (e.g. i.MX6) with or without windowing support
      qt_defines += ['-DHAVE_QT_EGLFS']
      egl_dep = dependency('egl', required : false)
      optional_deps += egl_dep
      have_qt_windowing = true
      if have_qpa_include
        # Wayland windowing
        if gst_gl_have_window_wayland
          # FIXME: automagic
          qt5waylandextras = dependency('qt5', modules : ['WaylandClient'], required : false)
          if qt5waylandextras.found()
            optional_deps += qt5waylandextras
            qt_defines += ['-DHAVE_QT_WAYLAND']
            have_qt_windowing = true
          endif
        endif
        # Android windowing
        if gst_gl_have_window_android
          # FIXME: automagic
          qt5androidextras = dependency('qt5', modules : ['AndroidExtras'], required : false)
          # for gl functions in QtGui/qopenglfunctions.h
          # FIXME: automagic
          glesv2_dep = cc.find_library('GLESv2', required : false)
          if glesv2_dep.found() and qt5androidextras.found()
            optional_deps += [qt5androidextras, glesv2_dep]
            qt_defines += ['-DHAVE_QT_ANDROID']
            have_qt_windowing = true
            # Needed for C++11 support in Cerbero. People building with Android
            # in some other way need to add the necessary bits themselves.
            optional_deps += dependency('gnustl', required : false)
          endif
        endif
      endif
    endif

    if gst_gl_have_platform_wgl and gst_gl_have_window_win32
      # for wglMakeCurrent()
      # FIXME: automagic
      opengl32_dep = cc.find_library('opengl32', required : false)
      if opengl32_dep.found()
        qt_defines += ['-DHAVE_QT_WIN32']
        optional_deps += opengl32_dep
        have_qt_windowing = true
      endif
    endif

    if gst_gl_have_window_cocoa and gst_gl_have_platform_cgl
      # FIXME: automagic
      qt5macextras = dependency('qt5', modules : ['MacExtras'], required : false)
      if qt5macextras.found()
        qt_defines += ['-DHAVE_QT_MAC']
        optional_deps += qt5macextras
        have_qt_windowing = true
      endif
    endif

    if gst_gl_have_window_eagl and gst_gl_have_platform_eagl
      if host_machine.system() == 'ios'
        qt_defines += ['-DHAVE_QT_IOS']
        have_qt_windowing = true
      endif
    endif

    if have_qt_windowing
      # Build it!
      moc_files = qt5_mod.preprocess(moc_headers : moc_headers)
      gstqmlgl = library('gstqmlgl', sources, moc_files,
        cpp_args : gst_plugins_good_args + qt_defines,
        link_args : noseh_link_args,
        include_directories: [configinc, libsinc],
        dependencies : [glib_deps, gst_dep, gstvideo_dep, gstgl_dep, qt5qml_dep, optional_deps],
        override_options : ['cpp_std=c++11'],
        install: true,
        install_dir : plugins_install_dir)
      pkgconfig.generate(gstqmlgl, install_dir : plugins_pkgconfig_install_dir)
      plugins += [gstqmlgl]
    endif
  endif
endif
