rtsp_sources = [
  'gstrtsp.c',
  'gstrtspsrc.c',
  'gstrtpdec.c',
  'gstrtspext.c',
]

gstrtsp = library('gstrtsp',
  rtsp_sources,
  c_args : gst_plugins_good_args,
  include_directories : [configinc, libsinc],
  dependencies : [gstbase_dep, gio_dep, gstrtp_dep, gstrtsp_dep, gstsdp_dep, gstnet_dep],
  install : true,
  install_dir : plugins_install_dir,
)
pkgconfig.generate(gstrtsp, install_dir : plugins_pkgconfig_install_dir)
plugins += [gstrtsp]
