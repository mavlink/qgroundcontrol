foreach plugin : ['alpha', 'apetag', 'audiofx', 'audioparsers', 'auparse',
                  'autodetect', 'avi', 'cutter', 'debugutils', 'deinterlace',
                  'dtmf', 'effectv', 'equalizer', 'flv', 'flx', 'goom',
                  'goom2k1', 'icydemux', 'id3demux', 'imagefreeze',
                  'interleave', 'isomp4', 'law', 'level', 'matroska',
                  'monoscope', 'multifile', 'multipart', 'replaygain', 'rtp',
                  'rtpmanager', 'rtsp', 'shapewipe', 'smpte', 'spectrum',
                  'udp', 'videobox', 'videocrop', 'videofilter', 'videomixer',
                  'wavenc', 'wavparse', 'y4m']
  if not get_option(plugin).disabled()
    subdir(plugin)
  endif
endforeach
