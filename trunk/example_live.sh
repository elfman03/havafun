#!/bin/sh

# This script will play live content from the Hava.  You must have mplayer
# installed for it to work (see http://mplayerhq.hu)
#
# Update your Hava IP address below in the HAVA variable

# Set this to be your havas address
#
HAVA=192.168.1.253

# Set this to be the duration that you want to play
#
DURATION=0

# Set your MPLAYER options here
#
MPLAYER="mplayer"
MOPTS="-quiet -cache 8192"
#MOPTS="-quiet -cache 8192 -fs -zoom"

echo "You must have mplayer installed for this to work (http://mplayerhq.hu)"

# record for DURATION seconds onto a pipe.  Have it play in mplayer
#
./hava_record $HAVA $DURATION - | $MPLAYER $MOPTS -
