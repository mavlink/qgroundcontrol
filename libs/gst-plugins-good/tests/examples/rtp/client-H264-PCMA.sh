#!/bin/sh
#
# A simple RTP receiver
#
#  receives H264 encoded RTP video on port 5000, RTCP is received on  port 5001.
#  the receiver RTCP reports are sent to port 5005
#  receives alaw encoded RTP audio on port 5002, RTCP is received on  port 5003.
#  the receiver RTCP reports are sent to port 5007
#
#             .-------.      .----------.     .---------.   .-------.   .-----------.
#  RTP        |udpsrc |      | rtpbin   |     |h264depay|   |h264dec|   |xvimagesink|
#  port=5000  |      src->recv_rtp recv_rtp->sink     src->sink   src->sink         |
#             '-------'      |          |     '---------'   '-------'   '-----------'
#                            |          |
#                            |          |     .-------.
#                            |          |     |udpsink|  RTCP
#                            |    send_rtcp->sink     | port=5005
#             .-------.      |          |     '-------' sync=false
#  RTCP       |udpsrc |      |          |               async=false
#  port=5001  |     src->recv_rtcp      |
#             '-------'      |          |
#                            |          |
#             .-------.      |          |     .---------.   .-------.   .-------------.
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

# the destination machine to send RTCP to. This is the address of the sender and
# is used to send back the RTCP reports of this receiver. If the data is sent
# from another machine, change this address.
DEST=127.0.0.1

# this adjusts the latency in the receiver
LATENCY=200

# the caps of the sender RTP stream. This is usually negotiated out of band with
# SDP or RTSP. normally these caps will also include SPS and PPS but we don't
# have a mechanism to get this from the sender with a -launch line.
VIDEO_CAPS="application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264"
AUDIO_CAPS="application/x-rtp,media=(string)audio,clock-rate=(int)8000,encoding-name=(string)PCMA"

VIDEO_DEC="rtph264depay ! avdec_h264"
AUDIO_DEC="rtppcmadepay ! alawdec"

VIDEO_SINK="videoconvert ! autovideosink"
AUDIO_SINK="audioconvert ! audioresample ! autoaudiosink"

gst-launch-1.0 -v rtpbin name=rtpbin latency=$LATENCY                                  \
     udpsrc caps=$VIDEO_CAPS port=5000 ! rtpbin.recv_rtp_sink_0                       \
       rtpbin. ! $VIDEO_DEC ! $VIDEO_SINK                                             \
     udpsrc port=5001 ! rtpbin.recv_rtcp_sink_0                                       \
         rtpbin.send_rtcp_src_0 ! udpsink port=5005 host=$DEST sync=false async=false \
     udpsrc caps=$AUDIO_CAPS port=5002 ! rtpbin.recv_rtp_sink_1                       \
       rtpbin. ! $AUDIO_DEC ! $AUDIO_SINK                                             \
     udpsrc port=5003 ! rtpbin.recv_rtcp_sink_1                                       \
         rtpbin.send_rtcp_src_1 ! udpsink port=5007 host=$DEST sync=false async=false
