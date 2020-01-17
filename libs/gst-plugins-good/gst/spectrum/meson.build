gstspectrum = library('gstspectrum',
  'gstspectrum.c',
  c_args : gst_plugins_good_args,
  include_directories : [configinc],
  dependencies : [gstbase_dep, gstfft_dep, gstaudio_dep, libm],
  install : true,
  install_dir : plugins_install_dir,
)
pkgconfig.generate(gstspectrum, install_dir : plugins_pkgconfig_install_dir)
plugins += [gstspectrum]
