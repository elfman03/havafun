@echo off

rem # This script will play live content from the Hava.  You must have mplayer
rem # installed for it to work (see http://mplayerhq.hu)
rem #
rem # Update your Hava IP address below in the HAVA variable

rem # Set this to be your havas address

net stop havasvc
rem set HAVA=192.168.1.253
set HAVA=-

rem # Set this to be the duration that you want to play

set DURATION=0
rem # Set this to be the quality you want >=~0x30 equals max rate ~8Mbps
rem # Use lower numbers (~0x10-0x15) if video is broken up (e.g., over wireless)
rem # Worst case choose 0x00 and let it try to figure it out itself
rem #
set QUALITY=0x00
rem set QUALITY=0x10
rem set QUALITY=0x50

rem # Set your MPLAYER options here
rem #
set MPLAYER="C:\utils\MPlayer-p3-svn-29355\mplayer"
set MOPTS=-quiet -cache 8192
rem # MOPTS="-quiet -cache 8192 -fs -zoom"

echo ""
echo "You must have mplayer installed for this to work (http://mplayerhq.hu)"
echo "Tested with http://sourceforge.net/project/showfiles.php?group_id=205275&package_id=248631&release_id=689228"

rem # record for DURATION seconds onto a pipe.  Have it play in mplayer
rem #
hava_record %HAVA% %QUALITY% %DURATION% - | %MPLAYER% %MOPTS% -
