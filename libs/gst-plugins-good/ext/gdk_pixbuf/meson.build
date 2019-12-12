pixbuf_sources = [
  'gstgdkpixbufdec.c',
  'gstgdkpixbufoverlay.c',
  'gstgdkpixbufplugin.c',
  'gstgdkpixbufsink.c',
]

gdkpixbuf_dep = dependency('gdk-pixbuf-2.0', version : '>=2.8.0', required : get_option('gdk-pixbuf'))

if gdkpixbuf_dep.found()
  gstgdkpixbuf = library('gstgdkpixbuf',
    pixbuf_sources,
    c_args : gst_plugins_good_args,
    link_args : noseh_link_args,
    include_directories : [configinc],
    dependencies : [gstbase_dep, gstvideo_dep, gstcontroller_dep, gdkpixbuf_dep],
    install : true,
    install_dir : plugins_install_dir,
  )
  pkgconfig.generate(gstgdkpixbuf, install_dir : plugins_pkgconfig_install_dir)
  plugins += [gstgdkpixbuf]
endif
