# ============================================================================
# GStreamer Configuration
# Exclusion lists and install functions for reducing package size
# ============================================================================

# Plugins excluded from installation (QGC only needs video streaming)
# These are plugin names without prefix (libgst) or extension (.so/.dll)
set(GSTREAMER_EXCLUDED_PLUGINS
    # Audio plugins (QGC is video-only)
    a52dec
    adpcmdec
    adpcmenc
    alaw
    alsa
    amrnb
    amrwbdec
    audiobuffersplit
    audioconvert
    audiofx
    audiofxbad
    audiolatency
    audiomixer
    audiomixmatrix
    audioparsers
    audiorate
    audioresample
    audiotestsrc
    audiovisualizers
    bs2b
    dsd
    dtmf
    dtsdec
    equalizer
    espeak
    faad
    fdkaac
    flac
    fluidsynthmidi
    gme
    gsm
    jack
    ladspa
    lame
    lc3
    ldac
    level
    lv2
    midi
    modplug
    mpg123
    mulaw
    openaptx
    openmpt
    opus
    opusparse
    pipewire
    pocketsphinx
    pulseaudio
    removesilence
    replaygain
    sbc
    sid
    siren
    soundtouch
    speex
    spandsp
    spectrum
    twolame
    voaacenc
    voamrwbenc
    volume
    vorbis
    wavenc
    wavpack
    wavparse
    wildmidi

    # DVD/Bluray/CD (not needed for drone video)
    cdio
    cdparanoia
    dvdlpcmdec
    dvdread
    dvdspu
    dvdsub
    resindvd

    # Image formats (QGC doesn't process images via GStreamer)
    gdkpixbuf
    jpeg
    jpegformat
    openexr
    openjpeg
    png
    pnm
    webp

    # Subtitle/text rendering
    assrender
    closedcaption
    dvbsubenc
    dvbsuboverlay
    kate
    pango
    subenc
    subparse
    teletext
    ttmlsubs

    # Legacy/specialized hardware
    1394
    dc1394
    decklink
    dvb
    fbdevsink
    openal
    openni2
    oss4
    ossaudio
    shm
    uvch264
    v4l2codecs
    video4linux2
    winks
    ximagesink
    ximagesrc
    xvimagesink

    # Effects/visualization (not needed)
    aasink
    accurip
    adder
    alpha
    alphacolor
    autoconvert
    bayer
    cacasink
    cairo
    camerabin
    coloreffects
    colormanagement
    compositor
    cutter
    effectv
    faceoverlay
    fieldanalysis
    frei0r
    freeverb
    gaudieffects
    geometrictransform
    goom
    goom2k1
    imagefreeze
    inter
    interlace
    interleave
    ivtc
    legacyrawparse
    libvisual
    monoscope
    navigationtest
    overlaycomposition
    qroverlay
    rfbsrc
    rsvg
    segmentclip
    shapewipe
    smpte
    speed
    videobox
    videocrop
    videofilter
    videofiltersbad
    videoframe_audiolevel
    videomixer
    videorate
    videoscale
    videosignal
    videotestsrc
    vmnc
    zbar
    zxing

    # Debug/development tools
    analyticsoverlay
    basedebug
    codec2json
    codecalpha
    codectimestamper
    coretracers
    debug
    debugutilsbad
    festival
    flite
    netsim

    # Unused container formats
    aiff
    asf
    asfmux
    auparse
    avi
    flv
    flxdec
    icydemux
    mxf
    ogg
    realmedia

    # Streaming protocols (using RTSP/UDP/TCP only)
    adaptivedemux2
    curl
    dash
    hls
    mpegtsmux
    mse
    neonhttpsrc
    rtmp
    rtmp2
    rtpmanagerbad
    rtponvif
    smooth
    smoothstreaming
    soup
    srt
    srtp

    # Bluetooth/fingerprinting
    bluez
    chromaprint

    # Encryption/DRM
    aes
    dtls

    # Misc unused
    apetag
    autodetect
    avtp
    bz2
    directfb
    dv
    encoding
    gdp
    gtk
    gtkwayland
    id3demux
    id3tag
    insertbin
    ipcpipeline
    ivfparse
    jp2kdecimator
    kms
    libcamera
    libcluttergst3
    mpeg2dec
    mpeg2enc
    mpegpsdemux
    mpegpsmux
    mplex
    multifile
    multipart
    musepack
    nice
    opencv
    pbtypes
    pcapparse
    peadapter
    peautogain
    peconvolver
    pecrystalizer
    proxy
    python
    rawparse
    rist
    rtspclientsink
    sctp
    shout2
    sndfile
    svtav1
    switchbin
    taglib
    theora
    timecode
    transcode
    unixfd
    uvcgadget
    vulkan
    waylandsink
    webrtc
    webrtcdsp
    xingmux
    y4mdec
    y4menc
)

# ============================================================================
# GStreamer Base Libraries Exclusion List
# Libraries not needed for video streaming (primarily affects Windows)
# ============================================================================
set(GSTREAMER_EXCLUDED_LIBS
    # Audio libraries (QGC is video-only)
    gstaudio-1.0
    gstbadaudio-1.0
    gstfft-1.0

    # Unused features
    gstadaptivedemux-1.0
    gstanalytics-1.0
    gstbasecamerabinsrc-1.0
    gstcheck-1.0
    gstcontroller-1.0
    gstinsertbin-1.0
    gstisoff-1.0
    gstmse-1.0
    gstopencv-1.0
    gstphotography-1.0
    gstplay-1.0
    gstplayer-1.0
    gstrtspserver-1.0
    gstsctp-1.0
    gsttranscoder-1.0
    gsturidownloader-1.0
    gstvulkan-1.0
    gstwayland-1.0
    gstwebrtc-1.0
    gstwebrtcnice-1.0
)

# ============================================================================
# Function: gstreamer_install_plugins
# Install GStreamer plugins with exclusions applied
#
# Arguments:
#   SOURCE_DIR  - Directory containing plugins
#   DEST_DIR    - Installation destination
#   EXTENSION   - File extension (so or dll)
#   PREFIX      - File prefix (libgst for Linux, gst for Windows)
# ============================================================================
function(gstreamer_install_plugins)
    cmake_parse_arguments(ARG "" "SOURCE_DIR;DEST_DIR;EXTENSION;PREFIX" "" ${ARGN})

    # Glob all plugins
    file(GLOB all_plugins "${ARG_SOURCE_DIR}/${ARG_PREFIX}*.${ARG_EXTENSION}")

    # Filter out excluded plugins
    set(plugins_to_install "")
    foreach(plugin_path IN LISTS all_plugins)
        get_filename_component(plugin_name "${plugin_path}" NAME)

        # Check if this plugin should be excluded
        set(exclude FALSE)
        foreach(excluded IN LISTS GSTREAMER_EXCLUDED_PLUGINS)
            if(plugin_name MATCHES "^${ARG_PREFIX}${excluded}[^a-z]")
                set(exclude TRUE)
                break()
            endif()
        endforeach()

        if(NOT exclude)
            list(APPEND plugins_to_install "${plugin_path}")
        endif()
    endforeach()

    # Install filtered plugins
    if(plugins_to_install)
        install(FILES ${plugins_to_install} DESTINATION "${ARG_DEST_DIR}")
    endif()
endfunction()

# ============================================================================
# Function: gstreamer_install_libs
# Install GStreamer libraries with exclusions applied (for Windows)
#
# Arguments:
#   SOURCE_DIR  - Directory containing libraries (e.g., bin/ on Windows)
#   DEST_DIR    - Installation destination
#   EXTENSION   - File extension (dll)
# ============================================================================
function(gstreamer_install_libs)
    cmake_parse_arguments(ARG "" "SOURCE_DIR;DEST_DIR;EXTENSION" "" ${ARGN})

    # Glob all DLLs
    file(GLOB all_libs "${ARG_SOURCE_DIR}/*.${ARG_EXTENSION}")

    # Filter out excluded libraries
    set(libs_to_install "")
    foreach(lib_path IN LISTS all_libs)
        get_filename_component(lib_name "${lib_path}" NAME)

        # Check if this is a GStreamer library that should be excluded
        set(exclude FALSE)
        foreach(excluded IN LISTS GSTREAMER_EXCLUDED_LIBS)
            # Match libgstfoo-1.0.dll or gstfoo-1.0.dll patterns
            if(lib_name MATCHES "(lib)?${excluded}[^a-z]")
                set(exclude TRUE)
                break()
            endif()
        endforeach()

        if(NOT exclude)
            list(APPEND libs_to_install "${lib_path}")
        endif()
    endforeach()

    # Install filtered libraries
    if(libs_to_install)
        install(FILES ${libs_to_install} DESTINATION "${ARG_DEST_DIR}")
    endif()
endfunction()
