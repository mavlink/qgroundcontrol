osxvideo_sources = ['osxvideosink.m', 'cocoawindow.m']

have_osxvideo = false
if host_system != 'darwin'
  subdir_done()
endif

osxvideo_opengl_dep = dependency('appleframeworks', modules : ['OpenGL'], required : get_option('osxvideo'))
osxvideo_cocoa_dep = dependency('appleframeworks', modules : ['Cocoa'], required : get_option('osxvideo'))
have_objc = add_languages('objc', required : get_option('osxvideo'))

if have_objc and osxvideo_opengl_dep.found() and osxvideo_cocoa_dep.found()

  # lockFocusIfCanDraw and OpenGL are deprecated in mac os 10.14
  # Could also have used GL_SILENCE_DEPRECATION for the latter
  osxvideo_gst_plugins_good_objc_args = gst_plugins_good_args + ['-Wno-deprecated-declarations']

  gstosxvideo = library('gstosxvideo',
    osxvideo_sources,
    c_args : gst_plugins_good_args,
    objc_args : osxvideo_gst_plugins_good_objc_args,
    include_directories : [configinc],
    dependencies : [gstvideo_dep, osxvideo_opengl_dep, osxvideo_cocoa_dep],
    install : true,
    install_dir : plugins_install_dir)
  pkgconfig.generate(gstosxvideo, install_dir : plugins_pkgconfig_install_dir)
  plugins += [gstosxvideo]
endif
