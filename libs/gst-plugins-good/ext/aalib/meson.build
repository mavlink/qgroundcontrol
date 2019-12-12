# Very much not going to implement all kinds of logic around aalib-config

have_aalib = false

# TODO: https://github.com/mesonbuild/meson/issues/3940
aalib_option = get_option('aalib')
if not aalib_option.disabled()
  have_aalib = cc.has_header('aalib.h')
  if not have_aalib and aalib_option.enabled()
    error('aalib plugin enabled, but aalib.h not found')
  endif
endif

if have_aalib
  libaa_dep = cc.find_library('aa', required : aalib_option)
  if libaa_dep.found()
    gstaasink = library('gstaasink', 'gstaasink.c',
      c_args : gst_plugins_good_args,
      link_args : noseh_link_args,
      include_directories : [configinc],
      dependencies : [gstvideo_dep, gstbase_dep, libaa_dep],
      install : true,
      install_dir : plugins_install_dir
    )
    pkgconfig.generate(gstaasink, install_dir : plugins_pkgconfig_install_dir)
    plugins += [gstaasink]
  endif
endif
