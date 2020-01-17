rtp_progs = [
  'server-alsasrc-PCMA',
  'client-PCMA',
  'client-rtpaux',
  'server-rtpaux',
]

foreach prog : rtp_progs
  executable(prog, prog + '.c',
    dependencies: [gstrtp_dep, gst_dep, libm],
    c_args : gst_plugins_good_args,
    include_directories : [configinc],
    install: false)
endforeach
