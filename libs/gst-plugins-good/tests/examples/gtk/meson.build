executable('gtksink', 'gtksink.c',
  dependencies: [gst_dep, gtk_dep, optional_deps],
  c_args: gst_plugins_good_args,
  include_directories: [configinc],
  install: false)

if build_gstgl
  executable('gtkglsink', 'gtkglsink.c',
    dependencies: [gst_dep, gstgl_dep, gtk_dep, x11_dep],
    c_args: gst_plugins_good_args,
    include_directories: [configinc],
    install: false)

  executable('glliveshader', 'glliveshader.c',
    dependencies: [gst_dep, gstgl_dep, gtk_dep, x11_dep],
    c_args: gst_plugins_good_args,
    include_directories: [configinc],
    install: false)
endif
