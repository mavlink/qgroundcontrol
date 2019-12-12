png_sources = [
  'gstpng.c',
  'gstpngenc.c',
  'gstpngdec.c',
]

libpng_dep = dependency('libpng', version : '>=1.2', required : get_option('png'))

if libpng_dep.found()
  gstpng = library('gstpng',
    png_sources,
    c_args : gst_plugins_good_args,
    link_args : noseh_link_args,
    include_directories : [configinc, libsinc],
    dependencies : [gstbase_dep, gstvideo_dep, libpng_dep],
    install : true,
    install_dir : plugins_install_dir,
  )
  pkgconfig.generate(gstpng, install_dir : plugins_pkgconfig_install_dir)
  plugins += [gstpng]
endif
