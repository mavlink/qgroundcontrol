gtk_sources = [
  'gstgtkbasesink.c',
  'gstgtksink.c',
  'gstgtkutils.c',
  'gstplugin.c',
  'gtkgstbasewidget.c',
  'gtkgstwidget.c',
]

gtk_defines = []
optional_deps = []

gtk_dep = dependency('gtk+-3.0', required : get_option('gtk3'))
if gtk_dep.found()
  if build_gstgl and gstgl_dep.found() and gtk_dep.version().version_compare('>=3.15.0')
    have_gtk3_gl_windowing = false

    if gst_gl_have_window_x11 and gst_gl_have_platform_glx
      # FIXME: automagic
      gtk_x11_dep = dependency('gtk+-x11-3.0', required : false)
      if gtk_x11_dep.found()
        optional_deps += gtk_x11_dep
        have_gtk3_gl_windowing = true
      endif
    endif

    if gst_gl_have_window_wayland and gst_gl_have_platform_egl
      # FIXME: automagic
      gtk_wayland_dep = dependency('gtk+-wayland-3.0', required : false)
      if gtk_wayland_dep.found()
        optional_deps += gtk_wayland_dep
        have_gtk3_gl_windowing = true
      endif
    endif

    if have_gtk3_gl_windowing
      gtk_sources += [
        'gstgtkglsink.c',
        'gtkgstglwidget.c',
      ]
      optional_deps += gstgl_dep
      gtk_defines += ['-DGST_USE_UNSTABLE_API', '-DHAVE_GTK3_GL']
    endif
  endif

  gstgtk = library('gstgtk',
    gtk_sources,
    c_args : gst_plugins_good_args + gtk_defines,
    link_args : noseh_link_args,
    include_directories : [configinc],
    dependencies : [gtk_dep, gstvideo_dep, gstbase_dep, libm] + optional_deps,
    install : true,
    install_dir : plugins_install_dir,
  )
  pkgconfig.generate(gstgtk, install_dir : plugins_pkgconfig_install_dir)
  plugins += [gstgtk]
endif

