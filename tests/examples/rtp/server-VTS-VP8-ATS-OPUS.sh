#!/bin/sh
#
# A simple RTP server 
#

SRC=localhost
DEST=localhost
VCAPS="video/x-raw,width=352,height=288,framerate=15/1"

gst-launch-1.0 -v rtpbin name=rtpbin \
           videotestsrc ! $VCAPS ! vp8enc ! rtpvp8pay ! rtpbin.send_rtp_sink_0          \
                     rtpbin.send_rtp_src_0 ! udpsink host=$DEST port=5000                                 \
                     rtpbin.send_rtcp_src_0 ! udpsink host=$DEST port=5001 sync=false async=false         \
                     udpsrc address=$SRC  port=5005 ! rtpbin.recv_rtcp_sink_0                                \
           audiotestsrc ! opusenc ! rtpopuspay ! rtpbin.send_rtp_sink_1  \
	             rtpbin.send_rtp_src_1 ! udpsink host=$DEST port=5002                                 \
	             rtpbin.send_rtcp_src_1 ! udpsink host=$DEST port=5003 sync=false async=false         \
                     udpsrc address=$SRC port=5007 ! rtpbin.recv_rtcp_sink_1
