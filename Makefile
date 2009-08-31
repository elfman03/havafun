all: hava_channel hava_record hava_info

RELEASE=alpha2

release: 
	zip dist/havafun-$(RELEASE)-win.zip *.exe *.cmd RELEASE_NOTES.html
	tar -czvf dist/havafun-$(RELEASE)-lin.tar.gz hava_channel hava_record hava_info *.sh RELEASE_NOTES.html

windows: hava_channel.exe hava_channel_nowin.exe hava_record.exe hava_info.exe

hava_util.o: hava_util.c hava_util.h hava_util_internals.h
	gcc -g -c hava_util.c

hava_info: hava_info.c hava_util.o
	gcc -g -o hava_info hava_info.c hava_util.o

hava_record: hava_record.c hava_util.o
	gcc -g -o hava_record hava_record.c hava_util.o

hava_channel: hava_channel.c hava_util.o
	gcc -g -o hava_channel hava_channel.c hava_util.o

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
	rm -f hava_util.o  hava_util.obj
	rm -f hava_channel hava_channel.exe hava_channel_nowin.exe
	rm -f hava_channel.obj hava_channel.pdb hava_channel.ilk vc80.pdb
	rm -f hava_channel_nowin.pdb hava_channel_nowin.ilk
	rm -f hava_record  hava_record.exe
	rm -f hava_record.obj  hava_record.pdb  hava_record.ilk 
	rm -f hava_info  hava_info.exe
	rm -f hava_info.obj  hava_info.pdb  hava_info.ilk 
