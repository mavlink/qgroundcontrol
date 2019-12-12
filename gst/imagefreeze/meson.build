gstimagefreeze = library('gstimagefreeze',
  'gstimagefreeze.c',
  c_args : gst_plugins_good_args,
  include_directories : [configinc, libsinc],
  dependencies : glib_deps + [gst_dep],
  install : true,
  install_dir : plugins_install_dir,
)
pkgconfig.generate(gstimagefreeze, install_dir : plugins_pkgconfig_install_dir)
plugins += [gstimagefreeze]
