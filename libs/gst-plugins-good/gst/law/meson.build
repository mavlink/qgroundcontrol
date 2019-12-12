gstalaw = library('gstalaw',
  'alaw-encode.c', 'alaw-decode.c', 'alaw.c',
  c_args : gst_plugins_good_args,
  include_directories : [configinc],
  dependencies : [gstbase_dep, gstaudio_dep],
  install : true,
  install_dir : plugins_install_dir,
)
pkgconfig.generate(gstalaw, install_dir : plugins_pkgconfig_install_dir)
plugins += [gstalaw]

gstmulaw = library('gstmulaw',
  'mulaw-encode.c', 'mulaw-conversion.c', 'mulaw-decode.c', 'mulaw.c',
  c_args : gst_plugins_good_args,
  include_directories : [configinc],
  dependencies : [gstbase_dep, gstaudio_dep],
  install : true,
  install_dir : plugins_install_dir,
)
pkgconfig.generate(gstmulaw, install_dir : plugins_pkgconfig_install_dir)
plugins += [gstmulaw]
