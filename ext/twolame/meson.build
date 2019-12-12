twolame_dep = dependency('twolame', version : '>= 0.3.10', required : get_option('twolame'))

if twolame_dep.found()
  twolame = library('gsttwolame',
    ['gsttwolamemp2enc.c'],
    c_args : gst_plugins_good_args,
    include_directories : [configinc, libsinc],
    dependencies : [gstaudio_dep, twolame_dep],
    install : true,
    install_dir : plugins_install_dir,
  )
  pkgconfig.generate(twolame, install_dir : plugins_pkgconfig_install_dir)
  plugins += [twolame]
endif
