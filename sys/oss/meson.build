oss_header_locations = [
  # Linux and newer BSD versions
  ['sys/soundcard.h', 'HAVE_OSS_INCLUDE_IN_SYS', 'OSS includes are in sys/'],
  # Some old BSD versions and also newer OpenBSD versions
  ['soundcard.h', 'HAVE_OSS_INCLUDE_IN_ROOT', 'OSS includes are in root'],
  # Some old BSD versions
  ['machine/soundcard.h', 'HAVE_OSS_INCLUDE_IN_MACHINE', 'OSS includes are in machine/'],
]

have_oss = false
oss_option = get_option('oss')
if not oss_option.disabled()
  foreach hdr : oss_header_locations
    if not have_oss
      if cc.has_header(hdr[0])
        cdata.set(hdr[1], 1, description: hdr[2])
        have_oss = true
      endif
    endif
  endforeach

  if not have_oss and oss_option.enabled()
    error('OSS plugin was enabled but soundcard.h was not found')
  endif
endif

if have_oss
  plugins += [library('gstossaudio',
    'gstossaudio.c', 'gstosshelper.c', 'gstosssink.c', 'gstosssrc.c',
    c_args : gst_plugins_good_args,
    include_directories : [configinc, libsinc],
    dependencies : [gstaudio_dep, gstbase_dep],
    install : true,
    install_dir : plugins_install_dir)]
endif
