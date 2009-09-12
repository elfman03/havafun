#!/bin/sh

# This script will record for a while and transcode video to H.264 format.  
# You must have mencoder installed for it to work (see http://mplayerhq.hu)
#
# Update your Hava IP address below in the HAVA variable

# Set this to be your havas address
#
HAVA=192.168.1.253

# Set this to be the quality you want >=~0x30 equals max rate ~8Mbps
# Use lower numbers (~0x10-0x15) if video is broken up (e.g., over wireless)
# Worst case use 0x00 and try to figure it out ourselves
#
#QUALITY=0x00
#QUALITY=0x10
QUALITY=0x50

# Set this to be the duration that you want to play
#
DURATION=300

# Set this to be the filename that you want to create
#
TARGET=target.mpg

# Set your MPLAYER options here
#
MENCODER="mencoder"
MOPTS="-quiet -idx -ovc lavc -lavcopts vcodec=mpeg2video:vbitrate=4339 -oac copy"


echo "You must have mencoder installed for this to work (http://mplayerhq.hu)"

# record for DURATION seconds onto a pipe.  Have it play in mplayer
#
./hava_record $HAVA $QUALITY $DURATION - | $MENCODER $MOPTS - -o $TARGET
#
# alternately this can be done in two steps
#
# ./hava_record $HAVA $QUALTY $DURATION tmp.mpg
# $MENCODER $MOPTS tmp.mpg -o $TARGET
