RELEASE=2022.1

all: windows
#all: vulk_info vulk_channel
#all: vulk_channel vulk_record vulk_info

#
# Linux
#

vulk_util.o: vulk_util.c vulk_util.h vulk_util_internals.h
	gcc -g -c vulk_util.c

vulk_info: vulk_info.c vulk_util.o
	gcc -g -o vulk_info vulk_info.c vulk_util.o

vulk_channel: vulk_channel.c vulk_util.o
	gcc -g -o vulk_channel vulk_channel.c vulk_util.o

vulk_record: vulk_record.c vulk_util.o
	gcc -g -o vulk_record vulk_record.c vulk_util.o

#
# Windows
#

windows: vulk_record.exe vulk_info.exe
#windows: vulk_channel.exe vulk_channel_nowin.exe vulk_record.exe vulk_info.exe

vulk_util.obj: vulk_util.c vulk_util.h vulk_util_internals.h
	cl /c /Zi vulk_util.c

vulk_info.exe: vulk_info.c vulk_util.obj
	cl /Zi vulk_info.c /link vulk_util.obj ws2_32.lib 

vulk_channel.exe: vulk_channel.c vulk_util.obj
	cl /Zi vulk_channel.c /link vulk_util.obj ws2_32.lib 

vulk_channel_nowin.exe: vulk_channel.c vulk_util.obj
	cl /DHAVA_NOWIN=1 /Zi /Fevulk_channel_nowin.exe vulk_channel.c /link vulk_util.obj ws2_32.lib 

vulk_record.exe: vulk_record.c vulk_util.obj
	cl /Zi vulk_record.c /link vulk_util.obj ws2_32.lib 

clean:
	rm -f vulk_util.o  vulk_util.obj
	rm -f vulk_info    vulk_info.exe
	rm -f vulk_channel            vulk_channel.exe vulk_channel_nowin.exe
	rm -f vulk_channel.obj        vulk_channel.pdb  vulk_channel.ilk 
	rm -f vulk_channel_nowin.pdb  vulk_channel_nowin.ilk
	rm -f vulk_record      vulk_record.exe
	rm -f vulk_record.obj  vulk_record.pdb  vulk_record.ilk 
	rm -f vulk_info.obj  vulk_info.pdb  vulk_info.ilk 
	rm -f vc80.pdb  vc90.pdb vc140.pdb
	chmod -x *.c *.h *.html Makefile
