if libjack_dep.found()
  executable('jack_client', 'jack_client.c',
    dependencies: [gst_dep, gtk_dep, libjack_dep],
    c_args: gst_plugins_good_args,
    include_directories: [configinc],
    install: false)
endif
