gsticydemux = library('gsticydemux',
  'gsticydemux.c',
  c_args : gst_plugins_good_args,
  include_directories : [configinc, libsinc],
  dependencies : [gst_dep, gstbase_dep, gsttag_dep, zlib_dep],
  install : true,
  install_dir : plugins_install_dir,
)
pkgconfig.generate(gsticydemux, install_dir : plugins_pkgconfig_install_dir)
plugins += [gsticydemux]
