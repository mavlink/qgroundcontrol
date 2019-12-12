mpg123_dep = dependency('libmpg123', version : '>= 1.3', required : get_option('mpg123'))

if mpg123_dep.found()
  gstmpg123 = library('gstmpg123',
    'gstmpg123audiodec.c',
    c_args : gst_plugins_good_args,
    include_directories : [configinc],
    dependencies : [gstaudio_dep, mpg123_dep],
    install : true,
    install_dir : plugins_install_dir,
  )
  pkgconfig.generate(gstmpg123, install_dir : plugins_pkgconfig_install_dir)
  plugins += [gstmpg123]
endif
