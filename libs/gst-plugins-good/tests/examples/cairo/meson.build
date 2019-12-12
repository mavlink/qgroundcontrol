if cairo_dep.found()
  executable('cairo_overlay', 'cairo_overlay.c',
    dependencies: [gstvideo_dep, gst_dep, cairo_dep, libm],
    c_args : gst_plugins_good_args,
    include_directories : [configinc],
    install: false)
endif
