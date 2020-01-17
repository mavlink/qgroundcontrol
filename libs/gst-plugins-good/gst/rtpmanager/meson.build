rtpmanager_sources = [
  'gstrtpmanager.c',
  'gstrtpbin.c',
  'gstrtpdtmfmux.c',
  'gstrtpjitterbuffer.c',
  'gstrtpmux.c',
  'gstrtpptdemux.c',
  'gstrtprtxqueue.c',
  'gstrtprtxreceive.c',
  'gstrtprtxsend.c',
  'gstrtpssrcdemux.c',
  'rtpjitterbuffer.c',
  'rtpsession.c',
  'rtpsource.c',
  'rtpstats.c',
  'rtptimerqueue.c',
  'gstrtpsession.c',
  'gstrtpfunnel.c',
]

gstrtpmanager = library('gstrtpmanager',
  rtpmanager_sources,
  c_args : gst_plugins_good_args,
  include_directories : [configinc, libsinc],
  dependencies : [gstbase_dep, gstnet_dep, gstrtp_dep, gio_dep],
  install : true,
  install_dir : plugins_install_dir,
)
pkgconfig.generate(gstrtpmanager, install_dir : plugins_pkgconfig_install_dir)
plugins += [gstrtpmanager]
