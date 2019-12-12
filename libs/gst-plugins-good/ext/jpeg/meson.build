jpeg_sources = [
  'gstjpeg.c',
  'gstjpegenc.c',
  'gstjpegdec.c',
]

jpeglib = dependency('libjpeg', required : get_option('jpeg'))

if jpeglib.found()
  gstjpeg = library('gstjpeg',
    jpeg_sources,
    c_args : gst_plugins_good_args,
    link_args : noseh_link_args,
    include_directories : [configinc, libsinc],
    dependencies : [gst_dep, gstbase_dep, gstvideo_dep, jpeglib, libm],
    install : true,
    install_dir : plugins_install_dir,
  )
  pkgconfig.generate(gstjpeg, install_dir : plugins_pkgconfig_install_dir)
  plugins += [gstjpeg]
endif
