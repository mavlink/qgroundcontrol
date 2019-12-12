valgrind_dep = dependency('valgrind', required: false).partial_dependency(compile_args : true)
if valgrind_dep.found()
  cdata.set('HAVE_VALGRIND', 1)
endif

# internal helper lib for unit testing audio parsers
libparser = static_library('libparser', 'elements/parser.c',
  c_args : gst_plugins_good_args + ['-DGST_USE_UNSTABLE_API'],
  include_directories : [configinc],
  dependencies : [gstcheck_dep],
  install : false)

libparser_dep = declare_dependency(link_with : libparser,
  dependencies : gstcheck_dep)

# name, condition when to skip the test and extra dependencies
good_tests = [
  [ 'elements/audioamplify', false, [gstfft_dep] ],
  [ 'elements/audiochebband', false, [gstfft_dep] ],
  [ 'elements/audiocheblimit', false, [gstfft_dep] ],
  [ 'elements/audiodynamic', false, [gstfft_dep] ],
  [ 'elements/audioecho', false, [gstfft_dep] ],
  [ 'elements/audiofirfilter', false, [gstfft_dep] ],
  [ 'elements/audioiirfilter', false, [gstfft_dep] ],
  [ 'elements/audioinvert', false, [gstfft_dep] ],
  [ 'elements/audiopanorama', false, [gstfft_dep] ],
  [ 'elements/audiowsincband', false, [gstfft_dep] ],
  [ 'elements/audiowsinclimit', false, [gstfft_dep] ],
  [ 'elements/alphacolor' ],
  [ 'elements/alpha' ],
  [ 'elements/avimux', false, [gstriff_dep] ],
  [ 'elements/avisubtitle', false, [gstriff_dep] ],
  [ 'elements/capssetter' ],
  [ 'elements/aacparse', false, [libparser_dep] ],
  [ 'elements/ac3parse', false, [libparser_dep] ],
  [ 'elements/amrparse', false, [libparser_dep] ],
  [ 'elements/flacparse', false, [libparser_dep] ],
  [ 'elements/mpegaudioparse', false, [libparser_dep] ],
  [ 'elements/autodetect' ],
  [ 'elements/deinterlace' ],
  [ 'elements/dtmf' ],
  [ 'elements/flvdemux' ],
  [ 'elements/flvmux' ],
  [ 'elements/mulawdec' ],
  [ 'elements/mulawenc' ],
  [ 'elements/icydemux' ],
  [ 'elements/id3demux' ],
  [ 'elements/imagefreeze' ],
  [ 'elements/deinterleave' ],
  [ 'elements/interleave' ],
  [ 'elements/level' ],
  [ 'elements/matroskademux', false, [gstriff_dep] ],
  [ 'elements/matroskamux', false, [gstriff_dep] ],
  [ 'elements/matroskaparse', false, [gstriff_dep] ],
  [ 'elements/multifile' ],
  [ 'elements/splitmux', ],
  [ 'elements/qtmux', false, [gstriff_dep, zlib_dep] ],
  [ 'elements/qtdemux', false, [gstriff_dep, zlib_dep] ],
  [ 'elements/rganalysis' ],
  [ 'elements/rglimiter' ],
  [ 'elements/rgvolume' ],
  [ 'elements/rtph261' ],
  [ 'elements/rtph263' ],
  [ 'elements/rtph264' ],
  [ 'elements/rtph265' ],
  [ 'elements/rtpvp9' ],
  [ 'elements/rtpbin' ],
  [ 'elements/rtpbin_buffer_list' ],
  [ 'elements/rtpcollision' ],
  [ 'elements/rtpfunnel' ],
  [ 'elements/rtpjitterbuffer', false, [gstrtp_dep],
     ['../../gst/rtpmanager/gstrtpjitterbuffer.c',
      '../../gst/rtpmanager/rtpjitterbuffer.c',
      '../../gst/rtpmanager/rtpstats.c',
      '../../gst/rtpmanager/rtptimerqueue.c']],
  [ 'elements/rtpmux' ],
  [ 'elements/rtprtx' ],
  [ 'elements/rtpsession' ],
  [ 'elements/rtpstorage', false, [],  ['../../gst/rtp/gstrtpstorage.c',
					'../../gst/rtp/rtpstorage.c',
					'../../gst/rtp/rtpstoragestream.c']],
  [ 'elements/rtpred' ],
  [ 'elements/rtpulpfec' ],
  [ 'elements/rtpssrcdemux' ],
  [ 'elements/rtp-payloading' ],
  [ 'elements/spectrum', false, [gstfft_dep] ],
  [ 'elements/shapewipe' ],
  [ 'elements/udpsink' ],
  [ 'elements/udpsrc' ],
  [ 'elements/videobox' ],
  [ 'elements/videocrop' ],
  [ 'elements/videofilter' ],
  [ 'elements/videomixer' ],
  [ 'elements/aspectratiocrop' ],
  [ 'pipelines/wavenc' ],
  [ 'elements/wavparse', false, [gstriff_dep] ],
  [ 'elements/wavpackparse', ],
  [ 'elements/y4menc' ],
  [ 'pipelines/effectv' ],
  [ 'elements/equalizer' ],
  [ 'pipelines/simple-launch-lines' ],
  [ 'pipelines/tagschecking' ],
  [ 'generic/states' ],
]

# FIXME: valgrind elements/rtp-payloading - needs fixing
# elements/videocrop should be disabled since it takes way too long in valgrind

# FIXME: unistd dependency or not tested yet on windows
if host_machine.system() != 'windows'
  good_tests += [
    [ 'pipelines/flacdec', not flac_dep.found() ],
    [ 'elements/gdkpixbufsink', not gdkpixbuf_dep.found(), [gdkpixbuf_dep] ],
    [ 'elements/gdkpixbufoverlay', not gdkpixbuf_dep.found() ],
    [ 'elements/jpegdec', not jpeglib.found() ],
    [ 'elements/jpegenc', not jpeglib.found() ],
    [ 'elements/mpg123audiodec', not mpg123_dep.found(),  [gstfft_dep]],
    [ 'elements/souphttpsrc', not libsoup_dep.found(), [libsoup_dep] ],
    [ 'elements/id3v2mux', not taglib_dep.found() ],
    [ 'elements/apev2mux', not taglib_dep.found() ],
    [ 'elements/vp8enc', not vpx_dep.found() or not have_vp8_encoder ],
    [ 'elements/vp8dec', not vpx_dep.found() or not have_vp8_decoder ],
    [ 'elements/vp9enc', not vpx_dep.found() or not have_vp9_encoder ],
    [ 'pipelines/lame', not lame_dep.found() ],
    [ 'elements/wavpackdec', not wavpack_dep.found() ],
    [ 'elements/wavpackenc', not wavpack_dep.found() ],
    [ 'pipelines/wavpack', not wavpack_dep.found() ],
  ]
endif

foo='''
if HAVE_ORC
check_orc = orc/deinterlace orc/videomixer orc/videobox
else
check_orc =
endif
'''

test_defines = [
  '-UG_DISABLE_ASSERT',
  '-UG_DISABLE_CAST_CHECKS',
  '-DGST_CHECK_TEST_ENVIRONMENT_BEACON="GST_PLUGIN_LOADING_WHITELIST"',
  '-DGST_TEST_FILES_PATH="' + meson.current_source_dir() + '/../files"',
  '-DGST_USE_UNSTABLE_API',
]

pluginsdirs = []
if gst_dep.type_name() == 'pkgconfig'
  pbase = dependency('gstreamer-plugins-base-' + api_version, required : false)
  pluginsdirs = [gst_dep.get_pkgconfig_variable('pluginsdir'),
                 pbase.get_pkgconfig_variable('pluginsdir')]
endif

# fake device drivers: we could run hardware element tests against dummy drivers
# v4l2: vivo (part of normal kernel)
#   modprobe vivo;
#   gst-launch v4l2src device="/dev/video1" ! xvimagesink;
#   rmmod vivo
#
# alsa: snd-dummy (part of normal alsa, not removable)
#   modprobe snd-dummy;
#   gst-launch alsasrc device="hw:2" ! fakesink
#   gst-launch fakesrc ! alsasink device="hw:2"
#
# need a way to figure out value for the device property
state_ignore_elements = '''aasink autoaudiosrc autoaudiosink autovideosrc
 autovideosink cacasink cairotextoverlay gtkglsink gtksink jackaudiosrc
 jackaudiosink osssrc osssink osxaudiosink osxaudiosrc osxvideosrc osxvideosink
 pulsesink pulsesrc pulsemixer v4l2src'''

# FIXME: check, also + PTHREAD_CFLAGS
test_deps = [gst_dep, gstbase_dep, gstnet_dep, gstcheck_dep, gstaudio_dep,
  gstvideo_dep, gstpbutils_dep, gstrtp_dep, gstrtsp_dep, gsttag_dep,
  gstapp_dep, gio_dep, valgrind_dep] + glib_deps

# FIXME: add valgrind suppression common/gst.supp gst-plugins-good.supp
foreach t : good_tests
  fname = '@0@.c'.format(t.get(0))
  test_name = t.get(0).underscorify()
  extra_sources = t.get(3, [ ])
  extra_deps = t.get(2, [ ])
  skip_test = t.get(1, false)
  if not skip_test
    env = environment()
    env.set('GST_PLUGIN_SYSTEM_PATH_1_0', '')
    env.set('GST_STATE_IGNORE_ELEMENTS', state_ignore_elements)
    env.set('CK_DEFAULT_TIMEOUT', '20')
    env.set('GST_PLUGIN_LOADING_WHITELIST', 'gstreamer', 'gst-plugins-base',
      'gst-plugins-good@' + meson.build_root())
    env.set('GST_PLUGIN_PATH_1_0', [meson.build_root()] + pluginsdirs)
    env.set('GSETTINGS_BACKEND', 'memory')

    env.set('GST_REGISTRY', join_paths(meson.current_build_dir(), '@0@.registry'.format(test_name)))
    exe = executable(test_name, fname, extra_sources,
      include_directories : [configinc, libsinc],
      c_args : ['-DHAVE_CONFIG_H=1' ] + test_defines,
      dependencies : [libm] + test_deps + extra_deps,
    )
    test(test_name, exe, env: env, timeout: 3 * 60)
  endif
endforeach

# orc tests
orc_tests = [
  ['orc_deinterlace', files('../../gst/deinterlace/tvtime.orc')],
  ['orc_videomixer', files('../../gst/videomixer/videomixerorc.orc')],
  ['orc_videobox', files('../../gst/videobox/gstvideoboxorc.orc')],
]

orc_test_dep = dependency('', required : false)
if have_orcc
  # FIXME: there should really just be a separate orc-test-0.4.pc file for this
  if orc_dep.type_name() == 'pkgconfig'
    orc_test_dep = cc.find_library('orc-test-0.4', required : false)
  else
    orc_test_dep = dependency('', fallback: ['orc', 'orc_test_dep'], required: false)
  endif
endif

if have_orcc and orc_test_dep.found()
  foreach t : orc_tests
    tname = t[0]
    torcfile = t[1]
    tcfilename = '@0@.c'.format(tname)
    test_c = custom_target(tcfilename,
      output: tcfilename,
      input: torcfile,
      command: [orcc, '--include', 'stdint.h', '--test', '-o', '@OUTPUT@', '@INPUT@'])
    test_exe = executable(tname, test_c, dependencies: [orc_dep, orc_test_dep])
    test(tname, test_exe)
  endforeach
endif
