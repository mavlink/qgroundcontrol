#!/bin/sh
#
# A simple RTP receiver 
#

VIDEO_CAPS="application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)VP8"
AUDIO_CAPS="application/x-rtp,media=(string)audio,clock-rate=(int)48000,encoding-name=(string)OPUS"

SRC=localhost
DEST=localhost

VIDEO_DEC="rtpvp8depay ! vp8dec"
AUDIO_DEC="rtpopusdepay ! opusdec"

VIDEO_SINK="videoconvert ! autovideosink"
AUDIO_SINK="audioconvert ! audioresample ! autoaudiosink"

LATENCY=100

gst-launch-1.0 -v rtpbin name=rtpbin latency=$LATENCY                                    \
           udpsrc caps=$VIDEO_CAPS address=$SRC port=5000 ! rtpbin.recv_rtp_sink_0                   \
	         rtpbin. ! $VIDEO_DEC ! $VIDEO_SINK                                     \
           udpsrc address=$SRC port=5001 ! rtpbin.recv_rtcp_sink_0                                   \
           rtpbin.send_rtcp_src_0 ! udpsink host=$DEST port=5005 sync=false async=false \
	   udpsrc caps=$AUDIO_CAPS address=$SRC port=5002 ! rtpbin.recv_rtp_sink_1                   \
	         rtpbin. ! $AUDIO_DEC ! $AUDIO_SINK                                     \
           udpsrc address=$SRC port=5003 ! rtpbin.recv_rtcp_sink_1                                   \
           rtpbin.send_rtcp_src_1 ! udpsink host=$DEST port=5007 sync=false async=false
