gstmultipart = library('gstmultipart',
  'multipart.c',
  'multipartdemux.c',
  'multipartmux.c',
  c_args : gst_plugins_good_args,
  include_directories : [configinc],
  dependencies : [gstbase_dep],
  install : true,
  install_dir : plugins_install_dir,
)
pkgconfig.generate(gstmultipart, install_dir : plugins_pkgconfig_install_dir)
plugins += [gstmultipart]
