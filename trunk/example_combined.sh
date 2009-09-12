#!/bin/sh

# This script will change to a channel and then play live content from the 
# Hava.  You must have mplayer installed for it to work 
# (see http://mplayerhq.hu)
#
# Update your Hava IP address below in the HAVA variable

# Set this to be your havas address
#
#HAVA=192.168.1.253
HAVA=-

#
# Which input are you using (see hava_channel showinputs)
#
INPUT=Component

#
# What is your remote control code (from Hava wizard)
#
REMOTE=S0775

# Set this to be the desired channel
#
CHANNEL=122

# Set this to be the duration that you want to play
#
DURATION=0

# Set this to be the quality you want >=~0x30 equals max rate ~8Mbps
# Use lower numbers (~0x10-0x15) if video is broken up (e.g., over wireless)
# Worst case, use 0x00 and settle for trying to figure it out...
#
#QUALITY=0x00
#QUALITY=0x10
QUALITY=0x50

# Set your MPLAYER options here
#
MPLAYER="mplayer"
MOPTS="-quiet -cache 8192"
#MOPTS="-quiet -cache 8192 -fs -zoom"

echo "You must have mplayer installed for this to work (http://mplayerhq.hu)"

#
# Power on and change to desired channel
#
./hava_channel $HAVA $INPUT $REMOTE PowerOn $CHANNEL 
#
# record for DURATION seconds onto a pipe.  Have it play in mplayer
#
./hava_record  $HAVA $QUALITY $DURATION - | $MPLAYER $MOPTS -
