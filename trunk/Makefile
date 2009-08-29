all: hava_channel hava_record

windows: hava_channel.exe 

hava_util.o: hava_util.c hava_util.h hava_util_internals.h
	gcc -g -c hava_util.c

hava_record: hava_record.c hava_util.o
	gcc -g -o hava_record hava_record.c hava_util.o

hava_channel: hava_channel.c hava_util.o
	gcc -g -o hava_channel hava_channel.c hava_util.o

hava_util.obj: hava_util.c hava_util.h hava_util_internals.h
	cl /c /Zi hava_util.c

hava_channel.exe: hava_channel.c hava_util.obj
	cl /Zi hava_channel.c /link hava_util.obj ws2_32.lib 

clean:
	rm -f hava_channel hava_channel.exe
	rm -f hava_record  hava_record.exe
	rm -f hava_util.o  hava_util.obj
	rm -f hava_channel.obj hava_channel.pdb hava_channel.ilk vc80.pdb
