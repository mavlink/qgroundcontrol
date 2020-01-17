dv_sources = [
  'gstdv.c',
  'gstdvdec.c',
  'gstdvdemux.c',
  'gstsmptetimecode.c',
  'smpte_test.c',
]

dv_dep = dependency('libdv', version : '>= 0.100', required : get_option('dv'))

if dv_dep.found()
  gstdv = library('gstdv',
    dv_sources,
    c_args : gst_plugins_good_args,
    include_directories : [configinc, libsinc],
    dependencies : [gstbase_dep, gsttag_dep, gstaudio_dep, gstvideo_dep, dv_dep],
    install : true,
    install_dir : plugins_install_dir,
  )
  pkgconfig.generate(gstdv, install_dir : plugins_pkgconfig_install_dir)
  plugins += [gstdv]
  # FIXME
  #executable('smpte_test',
  #  'smpte_test.c', 'gstsmptetimecode.c',
  #  dependencies : [gstbase_dep, gsttag_dep, gstaudio_dep, gstvideo_dep, dv_dep],
  #  install : false)
endif
