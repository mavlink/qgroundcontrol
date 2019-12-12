#! /usr/bin/env python

import gi
import sys
gi.require_version('Gst', '1.0')
from gi.repository import GObject, Gst


#gst-launch -v rtpbin name=rtpbin audiotestsrc ! audioconvert ! alawenc ! rtppcmapay ! rtpbin.send_rtp_sink_0 \
#                rtpbin.send_rtp_src_0 ! udpsink port=10000 host=xxx.xxx.xxx.xxx \
#                rtpbin.send_rtcp_src_0 ! udpsink port=10001 host=xxx.xxx.xxx.xxx sync=false async=false \
#                udpsrc port=10002 ! rtpbin.recv_rtcp_sink_0

DEST_HOST = '127.0.0.1'

AUDIO_SRC = 'audiotestsrc'
AUDIO_ENC = 'alawenc'
AUDIO_PAY = 'rtppcmapay'

RTP_SEND_PORT = 5002
RTCP_SEND_PORT = 5003
RTCP_RECV_PORT = 5007

GObject.threads_init()
Gst.init(sys.argv)

# the pipeline to hold everything
pipeline = Gst.Pipeline('rtp_server')

# the pipeline to hold everything
audiosrc = Gst.ElementFactory.make(AUDIO_SRC, 'audiosrc')
audioconv = Gst.ElementFactory.make('audioconvert', 'audioconv')
audiores = Gst.ElementFactory.make('audioresample', 'audiores')

# the pipeline to hold everything
audioenc = Gst.ElementFactory.make(AUDIO_ENC, 'audioenc')
audiopay = Gst.ElementFactory.make(AUDIO_PAY, 'audiopay')

# add capture and payloading to the pipeline and link
pipeline.add(audiosrc, audioconv, audiores, audioenc, audiopay)

audiosrc.link(audioconv)
audioconv.link(audiores)
audiores.link(audioenc)
audioenc.link(audiopay)

# the rtpbin element
rtpbin = Gst.ElementFactory.make('rtpbin', 'rtpbin')

pipeline.add(rtpbin)

# the udp sinks and source we will use for RTP and RTCP
rtpsink = Gst.ElementFactory.make('udpsink', 'rtpsink')
rtpsink.set_property('port', RTP_SEND_PORT)
rtpsink.set_property('host', DEST_HOST)

rtcpsink = Gst.ElementFactory.make('udpsink', 'rtcpsink')
rtcpsink.set_property('port', RTCP_SEND_PORT)
rtcpsink.set_property('host', DEST_HOST)
# no need for synchronisation or preroll on the RTCP sink
rtcpsink.set_property('async', False)
rtcpsink.set_property('sync', False)

rtcpsrc = Gst.ElementFactory.make('udpsrc', 'rtcpsrc')
rtcpsrc.set_property('port', RTCP_RECV_PORT)

pipeline.add(rtpsink, rtcpsink, rtcpsrc)

# now link all to the rtpbin, start by getting an RTP sinkpad for session 0
sinkpad = Gst.Element.get_request_pad(rtpbin, 'send_rtp_sink_0')
srcpad = Gst.Element.get_static_pad(audiopay, 'src')
lres = Gst.Pad.link(srcpad, sinkpad)

# get the RTP srcpad that was created when we requested the sinkpad above and
# link it to the rtpsink sinkpad
srcpad = Gst.Element.get_static_pad(rtpbin, 'send_rtp_src_0')
sinkpad = Gst.Element.get_static_pad(rtpsink, 'sink')
lres = Gst.Pad.link(srcpad, sinkpad)

# get an RTCP srcpad for sending RTCP to the receiver
srcpad = Gst.Element.get_request_pad(rtpbin, 'send_rtcp_src_0')
sinkpad = Gst.Element.get_static_pad(rtcpsink, 'sink')
lres = Gst.Pad.link(srcpad, sinkpad)

# we also want to receive RTCP, request an RTCP sinkpad for session 0 and
# link it to the srcpad of the udpsrc for RTCP
srcpad = Gst.Element.get_static_pad(rtcpsrc, 'src')
sinkpad = Gst.Element.get_request_pad(rtpbin, 'recv_rtcp_sink_0')
lres = Gst.Pad.link(srcpad, sinkpad)

# set the pipeline to playing
Gst.Element.set_state(pipeline, Gst.State.PLAYING)

# we need to run a GLib main loop to get the messages
mainloop = GObject.MainLoop()
mainloop.run()

Gst.Element.set_state(pipeline, Gst.State.NULL)
