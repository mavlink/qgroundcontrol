gstapetag = library('gstapetag', 'gstapedemux.c',
  c_args : gst_plugins_good_args,
  include_directories : [configinc, libsinc],
  dependencies : [gstpbutils_dep, gsttag_dep, gst_dep],
  install : true,
  install_dir : plugins_install_dir,
)
pkgconfig.generate(gstapetag, install_dir : plugins_pkgconfig_install_dir)
plugins += [gstapetag]
