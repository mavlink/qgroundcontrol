#!/bin/sh
#
# A simple RTP server
#  sends the output of autoaudiosrc as alaw encoded RTP on port 5002, RTCP is sent on
#  port 5003. The destination is 127.0.0.1.
#  the receiver RTCP reports are received on port 5007
#
# .--------.    .-------.    .-------.      .----------.     .-------.
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

# change this to send the RTP data and RTCP to another host
DEST=127.0.0.1

#AELEM=autoaudiosrc
AELEM=audiotestsrc

# PCMA encode from an the source
ASOURCE="$AELEM ! audioconvert"
AENC="alawenc ! rtppcmapay"

gst-launch-1.0 -v rtpbin name=rtpbin \
     $ASOURCE ! $AENC ! rtpbin.send_rtp_sink_0  \
            rtpbin.send_rtp_src_0 ! udpsink port=5002 host=$DEST                      \
            rtpbin.send_rtcp_src_0 ! udpsink port=5003 host=$DEST sync=false async=false \
         udpsrc port=5007 ! rtpbin.recv_rtcp_sink_0
