matroska_sources = [
  'ebml-read.c',
  'ebml-write.c',
  'matroska.c',
  'matroska-demux.c',
  'matroska-parse.c',
  'matroska-ids.c',
  'matroska-mux.c',
  'matroska-read-common.c',
  'webm-mux.c',
  'lzo.c',
]

bz2_dep = cc.find_library('bz2', required : get_option('bz2'))
cdata.set('HAVE_BZ2', bz2_dep.found() and cc.has_header('bzlib.h'))

gstmatroska = library('gstmatroska',
  matroska_sources,
  c_args : gst_plugins_good_args,
  link_args : noseh_link_args,
  include_directories : [configinc],
  dependencies : [gstpbutils_dep, gstaudio_dep, gstriff_dep,
                  gstvideo_dep, gsttag_dep, gstbase_dep,
                  gst_dep, zlib_dep, bz2_dep, libm],
  install : true,
  install_dir : plugins_install_dir,
)
pkgconfig.generate(gstmatroska, install_dir : plugins_pkgconfig_install_dir)
plugins += [gstmatroska]
