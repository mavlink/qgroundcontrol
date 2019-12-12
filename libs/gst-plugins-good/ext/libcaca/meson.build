libcaca_dep = dependency('caca', required : get_option('libcaca'))

if libcaca_dep.found()
  caca = library('gstcacasink', 'gstcacasink.c',
    c_args : gst_plugins_good_args,
    link_args : noseh_link_args,
    include_directories : [configinc],
    dependencies : [gstvideo_dep, gstbase_dep, libcaca_dep],
    install : true,
    install_dir : plugins_install_dir
  )
  plugins += [caca]
endif
