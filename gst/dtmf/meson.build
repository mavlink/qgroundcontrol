dtmf_sources = [
  'gstdtmfsrc.c',
  'gstrtpdtmfsrc.c',
  'gstrtpdtmfdepay.c',
  'gstdtmf.c'
]

gstdtmf = library('gstdtmf',
  dtmf_sources,
  c_args : gst_plugins_good_args,
  include_directories : [configinc],
  dependencies : [gstbase_dep, gstrtp_dep, libm],
  install : true,
  install_dir : plugins_install_dir,
)
pkgconfig.generate(gstdtmf, install_dir : plugins_pkgconfig_install_dir)
plugins += [gstdtmf]

