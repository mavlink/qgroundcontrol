jack_sources = [
  'gstjackaudioclient.c',
  'gstjackaudiosink.c',
  'gstjackaudiosrc.c',
  'gstjack.c',
  'gstjackutil.c',
]

libjack_dep = dependency('jack', version : '>= 1.9.7', required : get_option('jack'))

if libjack_dep.found()
  gstjack = library('gstjack',
    jack_sources,
    c_args : gst_plugins_good_args + ['-DHAVE_JACK_1_9_7'],
    include_directories : [configinc, libsinc],
    dependencies : [gst_dep, gstbase_dep, gstaudio_dep, libjack_dep],
    install : true,
    install_dir : plugins_install_dir,
  )
  pkgconfig.generate(gstjack, install_dir : plugins_pkgconfig_install_dir)
  plugins += [gstjack]
endif
