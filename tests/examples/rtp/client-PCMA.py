#! /usr/bin/env python

import gi
import sys
gi.require_version('Gst', '1.0')
from gi.repository import GObject, Gst

#
# A simple RTP receiver
#
#  receives alaw encoded RTP audio on port 5002, RTCP is received on  port 5003.
#  the receiver RTCP reports are sent to port 5007
#
#             .-------.      .----------.     .---------.   .-------.   .--------.
#  RTP        |udpsrc |      | rtpbin   |     |pcmadepay|   |alawdec|   |alsasink|
#  port=5002  |      src->recv_rtp recv_rtp->sink     src->sink   src->sink      |
#             '-------'      |          |     '---------'   '-------'   '--------'
#                            |          |
#                            |          |     .-------.
#                            |          |     |udpsink|  RTCP
#                            |    send_rtcp->sink     | port=5007
#             .-------.      |          |     '-------' sync=false
#  RTCP       |udpsrc |      |          |               async=false
#  port=5003  |     src->recv_rtcp      |
#             '-------'      '----------'

AUDIO_CAPS = 'application/x-rtp,media=(string)audio,clock-rate=(int)8000,encoding-name=(string)PCMA'
AUDIO_DEPAY = 'rtppcmadepay'
AUDIO_DEC = 'alawdec'
AUDIO_SINK = 'autoaudiosink'

DEST = '127.0.0.1'

RTP_RECV_PORT = 5002
RTCP_RECV_PORT = 5003
RTCP_SEND_PORT = 5007

GObject.threads_init()
Gst.init(sys.argv)

#gst-launch -v rtpbin name=rtpbin                                                \
#       udpsrc caps=$AUDIO_CAPS port=$RTP_RECV_PORT ! rtpbin.recv_rtp_sink_0              \
#             rtpbin. ! rtppcmadepay ! alawdec ! audioconvert ! audioresample ! autoaudiosink \
#           udpsrc port=$RTCP_RECV_PORT ! rtpbin.recv_rtcp_sink_0                              \
#         rtpbin.send_rtcp_src_0 ! udpsink port=$RTCP_SEND_PORT host=$DEST sync=false async=false

def pad_added_cb(rtpbin, new_pad, depay):
    sinkpad = Gst.Element.get_static_pad(depay, 'sink')
    lres = Gst.Pad.link(new_pad, sinkpad)

# the pipeline to hold eveything
pipeline = Gst.Pipeline('rtp_client')

# the udp src and source we will use for RTP and RTCP
rtpsrc = Gst.ElementFactory.make('udpsrc', 'rtpsrc')
rtpsrc.set_property('port', RTP_RECV_PORT)

# we need to set caps on the udpsrc for the RTP data
caps = Gst.caps_from_string(AUDIO_CAPS)
rtpsrc.set_property('caps', caps)

rtcpsrc = Gst.ElementFactory.make('udpsrc', 'rtcpsrc')
rtcpsrc.set_property('port', RTCP_RECV_PORT)

rtcpsink = Gst.ElementFactory.make('udpsink', 'rtcpsink')
rtcpsink.set_property('port', RTCP_SEND_PORT)
rtcpsink.set_property('host', DEST)

# no need for synchronisation or preroll on the RTCP sink
rtcpsink.set_property('async', False)
rtcpsink.set_property('sync', False)

pipeline.add(rtpsrc, rtcpsrc, rtcpsink)

# the depayloading and decoding
audiodepay = Gst.ElementFactory.make(AUDIO_DEPAY, 'audiodepay')
audiodec = Gst.ElementFactory.make(AUDIO_DEC, 'audiodec')

# the audio playback and format conversion
audioconv = Gst.ElementFactory.make('audioconvert', 'audioconv')
audiores = Gst.ElementFactory.make('audioresample', 'audiores')
audiosink = Gst.ElementFactory.make(AUDIO_SINK, 'audiosink')

# add depayloading and playback to the pipeline and link
pipeline.add(audiodepay, audiodec, audioconv, audiores, audiosink)

audiodepay.link(audiodec)
audiodec.link(audioconv)
audioconv.link(audiores)
audiores.link(audiosink)

# the rtpbin element
rtpbin = Gst.ElementFactory.make('rtpbin', 'rtpbin')

pipeline.add(rtpbin)

# now link all to the rtpbin, start by getting an RTP sinkpad for session 0
srcpad = Gst.Element.get_static_pad(rtpsrc, 'src')
sinkpad = Gst.Element.get_request_pad(rtpbin, 'recv_rtp_sink_0')
lres = Gst.Pad.link(srcpad, sinkpad)

# get an RTCP sinkpad in session 0
srcpad = Gst.Element.get_static_pad(rtcpsrc, 'src')
sinkpad = Gst.Element.get_request_pad(rtpbin, 'recv_rtcp_sink_0')
lres = Gst.Pad.link(srcpad, sinkpad)

# get an RTCP srcpad for sending RTCP back to the sender
srcpad = Gst.Element.get_request_pad(rtpbin, 'send_rtcp_src_0')
sinkpad = Gst.Element.get_static_pad(rtcpsink, 'sink')
lres = Gst.Pad.link(srcpad, sinkpad)

rtpbin.connect('pad-added', pad_added_cb, audiodepay)

Gst.Element.set_state(pipeline, Gst.State.PLAYING)

mainloop = GObject.MainLoop()
mainloop.run()

Gst.Element.set_state(pipeline, Gst.State.NULL)

