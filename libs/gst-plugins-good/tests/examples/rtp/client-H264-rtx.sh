#!/bin/sh
#
# A simple RTP receiver with retransmission
#
#  receives H264 encoded RTP video on port 5000, RTCP is received on  port 5001.
#  the receiver RTCP reports are sent to port 5005
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
#             '-------'      '----------'              


# the caps of the sender RTP stream. This is usually negotiated out of band with
# SDP or RTSP. normally these caps will also include SPS and PPS but we don't
# have a mechanism to get this from the sender with a -launch line.
VIDEO_CAPS="application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264"

VIDEO_DEC="rtph264depay ! avdec_h264"

VIDEO_SINK="videoconvert ! autovideosink"

# the destination machine to send RTCP to. This is the address of the sender and
# is used to send back the RTCP reports of this receiver. If the data is sent
# from another machine, change this address.
DEST=127.0.0.1

LATENCY=200

gst-launch-1.0 -v rtpbin name=rtpbin rtp-profile=avpf latency=$LATENCY do-retransmission=1 \
    udpsrc caps=$VIDEO_CAPS port=5000 ! rtpbin.recv_rtp_sink_0                      \
      rtpbin. ! $VIDEO_DEC ! $VIDEO_SINK                                            \
    udpsrc port=5001 ! rtpbin.recv_rtcp_sink_0                                      \
      rtpbin.send_rtcp_src_0 ! udpsink port=5005 host=$DEST sync=false async=false
