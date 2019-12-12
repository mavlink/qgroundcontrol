#!/bin/sh
#
# A simple RTP receiver 
#

VIDEO_CAPS="application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H263-1998"

#DEST=192.168.1.126
DEST=localhost

VIDEO_DEC="rtph263pdepay ! avdec_h263"

VIDEO_SINK="videoconvert ! autovideosink"

LATENCY=100

gst-launch-1.0 -v rtpbin name=rtpbin latency=$LATENCY                                    \
           udpsrc caps=$VIDEO_CAPS port=5000 ! rtpbin.recv_rtp_sink_0                   \
	         rtpbin. ! $VIDEO_DEC ! $VIDEO_SINK                                     \
           udpsrc port=5001 ! rtpbin.recv_rtcp_sink_0                                   \
           rtpbin.send_rtcp_src_0 ! udpsink host=$DEST port=5005 sync=false async=false
