#!/bin/sh
#
# A simple RTP receiver
#
#  receives alaw encoded RTP audio on port 5002, RTCP is received on  port 5003.
#  the receiver RTCP reports are sent to port 5007
#
#             .-------.      .----------.     .---------.   .-------.   .-------------.
#  RTP        |udpsrc |      | rtpbin   |     |pcmadepay|   |alawdec|   |autoaudiosink|
#  port=5002  |      src->recv_rtp recv_rtp->sink     src->sink   src->sink           |
#             '-------'      |          |     '---------'   '-------'   '-------------'
#                            |          |
#                            |          |     .-------.
#                            |          |     |udpsink|  RTCP
#                            |    send_rtcp->sink     | port=5007
#             .-------.      |          |     '-------' sync=false
#  RTCP       |udpsrc |      |          |               async=false
#  port=5003  |     src->recv_rtcp      |
#             '-------'      '----------'


# the caps of the sender RTP stream. This is usually negotiated out of band with
# SDP or RTSP.
AUDIO_CAPS="application/x-rtp,media=(string)audio,clock-rate=(int)8000,encoding-name=(string)PCMA"

AUDIO_DEC="rtppcmadepay ! alawdec"

AUDIO_SINK="audioconvert ! audioresample ! autoaudiosink"

# the destination machine to send RTCP to. This is the address of the sender and
# is used to send back the RTCP reports of this receiver. If the data is sent
# from another machine, change this address.
DEST=127.0.0.1

gst-launch-1.0 -v rtpbin name=rtpbin                                                \
	   udpsrc caps=$AUDIO_CAPS port=5002 ! rtpbin.recv_rtp_sink_0              \
	         rtpbin. ! $AUDIO_DEC ! $AUDIO_SINK                                \
           udpsrc port=5003 ! rtpbin.recv_rtcp_sink_0                              \
         rtpbin.send_rtcp_src_0 ! udpsink port=5007 host=$DEST sync=false async=false
