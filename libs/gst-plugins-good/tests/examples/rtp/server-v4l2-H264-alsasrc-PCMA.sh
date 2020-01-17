#!/bin/sh
#
# A simple RTP server
#  sends the output of v4l2src as h264 encoded RTP on port 5000, RTCP is sent on
#  port 5001. The destination is 127.0.0.1.
#  the video receiver RTCP reports are received on port 5005
#  sends the output of autoaudiosrc as alaw encoded RTP on port 5002, RTCP is sent on
#  port 5003. The destination is 127.0.0.1.
#  the receiver RTCP reports are received on port 5007
#
#  .-------.    .-------.    .-------.      .----------.     .-------.
#  |v4lssrc|    |h264enc|    |h264pay|      | rtpbin   |     |udpsink|  RTP
#  |      src->sink    src->sink    src->send_rtp send_rtp->sink     | port=5000
#  '-------'    '-------'    '-------'      |          |     '-------'
#                                           |          |
#                                           |          |     .-------.
#                                           |          |     |udpsink|  RTCP
#                                           |    send_rtcp->sink     | port=5001
#                            .-------.      |          |     '-------' sync=false
#                 RTCP       |udpsrc |      |          |               async=false
#               port=5005    |     src->recv_rtcp      |
#                            '-------'      |          |
#                                           |          |
# .--------.    .-------.    .-------.      |          |     .-------.
# |audiosrc|    |alawenc|    |pcmapay|      | rtpbin   |     |udpsink|  RTP
# |       src->sink    src->sink    src->send_rtp send_rtp->sink     | port=5002
# '--------'    '-------'    '-------'      |          |     '-------'
#                                           |          |
#                                           |          |     .-------.
#                                           |          |     |udpsink|  RTCP
#                                           |    send_rtcp->sink     | port=5003
#                            .-------.      |          |     '-------' sync=false
#                 RTCP       |udpsrc |      |          |               async=false
#               port=5007    |     src->recv_rtcp      |
#                            '-------'      '----------'
#
# ideally we should transport the properties on the RTP udpsink pads to the
# receiver in order to transmit the SPS and PPS earlier.

# change this to send the RTP data and RTCP to another host
DEST=127.0.0.1

# tuning parameters to make the sender send the streams out of sync. Can be used
# ot test the client RTCP synchronisation.
#VOFFSET=900000000
VOFFSET=0
AOFFSET=0

# H264 encode from the source
VELEM="v4l2src"
#VELEM="videotestsrc is-live=1"
VCAPS="video/x-raw,width=352,height=288,framerate=15/1"
VSOURCE="$VELEM ! queue ! videorate ! videoconvert ! $VCAPS"
VENC="x264enc tune=zerolatency byte-stream=true bitrate=300 ! rtph264pay"

VRTPSINK="udpsink port=5000 host=$DEST ts-offset=$VOFFSET name=vrtpsink"
VRTCPSINK="udpsink port=5001 host=$DEST sync=false async=false name=vrtcpsink"
VRTCPSRC="udpsrc port=5005 name=vrtpsrc"

# PCMA encode from the source
AELEM="autoaudiosrc"
#AELEM="audiotestsrc is-live=1"
ASOURCE="$AELEM ! queue ! audioresample ! audioconvert"
AENC="alawenc ! rtppcmapay"

ARTPSINK="udpsink port=5002 host=$DEST ts-offset=$AOFFSET name=artpsink"
ARTCPSINK="udpsink port=5003 host=$DEST sync=false async=false name=artcpsink"
ARTCPSRC="udpsrc port=5007 name=artpsrc"

gst-launch-1.0 -v rtpbin name=rtpbin \
    $VSOURCE ! $VENC ! rtpbin.send_rtp_sink_0                                             \
        rtpbin.send_rtp_src_0 ! $VRTPSINK                                                 \
        rtpbin.send_rtcp_src_0 ! $VRTCPSINK                                               \
      $VRTCPSRC ! rtpbin.recv_rtcp_sink_0                                                 \
    $ASOURCE ! $AENC ! rtpbin.send_rtp_sink_1                                             \
        rtpbin.send_rtp_src_1 ! $ARTPSINK                                                 \
        rtpbin.send_rtcp_src_1 ! $ARTCPSINK                                               \
      $ARTCPSRC ! rtpbin.recv_rtcp_sink_1
