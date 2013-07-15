RELEASE=PREalpha4

all: vulk_info
#all: hava_channel hava_record hava_info

#
# Linux
#

vulk_util.o: vulk_util.c vulk_util.h vulk_util_internals.h
	gcc -g -c vulk_util.c

vulk_info: vulk_info.c vulk_util.o
	gcc -g -o vulk_info vulk_info.c vulk_util.o

hava_record: hava_record.c hava_util.o
	gcc -g -o hava_record hava_record.c hava_util.o

hava_channel: hava_channel.c hava_util.o
	gcc -g -o hava_channel hava_channel.c hava_util.o

#
# Windows
#

windows: hava_channel.exe hava_channel_nowin.exe hava_record.exe hava_info.exe

hava_util.obj: hava_util.c hava_util.h hava_util_internals.h
	cl /c /Zi hava_util.c

hava_info.exe: hava_info.c hava_util.obj
	cl /Zi hava_info.c /link hava_util.obj ws2_32.lib 

hava_channel.exe: hava_channel.c hava_util.obj
	cl /Zi hava_channel.c /link hava_util.obj ws2_32.lib 

hava_channel_nowin.exe: hava_channel.c hava_util.obj
	cl /DHAVA_NOWIN=1 /Zi /Fehava_channel_nowin.exe hava_channel.c /link hava_util.obj ws2_32.lib 

hava_record.exe: hava_record.c hava_util.obj
	cl /Zi hava_record.c /link hava_util.obj ws2_32.lib 

clean:
	rm -f vulk_util.o  vulk_util.obj
	rm -f vulk_info    vulk_info.exe
	rm -f hava_channel            hava_channel.exe hava_channel_nowin.exe
	rm -f hava_channel.obj        hava_channel.pdb  hava_channel.ilk 
	rm -f hava_channel_nowin.pdb  hava_channel_nowin.ilk
	rm -f hava_record      hava_record.exe
	rm -f hava_record.obj  hava_record.pdb  hava_record.ilk 
	rm -f hava_info.obj  hava_info.pdb  hava_info.ilk 
	rm -f vc80.pdb  vc90.pdb
	chmod -x *.c *.h *.html Makefile
