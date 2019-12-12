#!/bin/sh
#
# A simple RTP server 
#
# Transcodes and streams audio and video to H263p and AMR respectively from any
# file that is decodable by decodebin.
#
# use ./client-H263p-AMR.sh to receive the RTP stream.
#

# change these to change the server sync. This causes the server to send the
# packets largly out-of-sync, the client should use the RTCP SR packets to
# restore proper lip-sync between the streams.
AOFFSET=0
VOFFSET=0

# encoder seems to be picky about PAR, so force one that should work
VCAPS="video/x-raw,width=352,height=288,framerate=15/1,pixel-aspect-ratio=1/1"

# video and audio encoding and payloading
VENCPAY="avenc_h263p ! rtph263ppay"
AENCPAY="amrnbenc ! rtpamrpay"

# video conversion 
VCONV="videoconvert ! videoscale ! videorate ! $VCAPS ! videoconvert"

ACONV="audioconvert ! audioresample"

#HOST=192.168.1.126
HOST=127.0.0.1

if test -z "$1"; then
  echo "No URI argument.";
  echo "Usage: $0 file:///path/to/video.file";
  exit 1;
fi

gst-launch-1.0 -v rtpbin name=rtpbin \
           uridecodebin uri="$1" name=decode \
           decode. ! $VCONV ! $VENCPAY ! rtpbin.send_rtp_sink_0      \
                     rtpbin.send_rtp_src_0 ! queue ! udpsink host=$HOST port=5000 ts-offset=$AOFFSET      \
                     rtpbin.send_rtcp_src_0 ! udpsink host=$HOST port=5001 sync=false async=false         \
                     udpsrc port=5005 ! rtpbin.recv_rtcp_sink_0                                           \
           decode. ! $ACONV ! $AENCPAY ! rtpbin.send_rtp_sink_1                         \
	             rtpbin.send_rtp_src_1 ! queue ! udpsink host=$HOST port=5002 ts-offset=$VOFFSET      \
	             rtpbin.send_rtcp_src_1 ! udpsink host=$HOST port=5003 sync=false async=false         \
                     udpsrc port=5007 ! rtpbin.recv_rtcp_sink_1
