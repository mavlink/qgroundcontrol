gstid3demux = library('gstid3demux',
  'gstid3demux.c',
  c_args : gst_plugins_good_args,
  include_directories : [configinc, libsinc],
  dependencies : [gst_dep, gstbase_dep, gsttag_dep, gstpbutils_dep],
  install : true,
  install_dir : plugins_install_dir,
)
pkgconfig.generate(gstid3demux, install_dir : plugins_pkgconfig_install_dir)
plugins += [gstid3demux]
