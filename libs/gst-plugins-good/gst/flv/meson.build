gstflv = library('gstflv',
  'gstflvdemux.c', 'gstflvmux.c',
  c_args : gst_plugins_good_args,
  include_directories : [configinc, libsinc],
  dependencies : [gstpbutils_dep, gstvideo_dep, gsttag_dep, gstaudio_dep],
  install : true,
  install_dir : plugins_install_dir,
)
pkgconfig.generate(gstflv, install_dir : plugins_pkgconfig_install_dir)
plugins += [gstflv]
