all: hava_channel hava_record

windows: hava_channel.exe 

hava_record: hava_record.c hava_util.h
	gcc -g -o hava_record hava_record.c

hava_channel: hava_channel.c
	gcc -g -o hava_channel hava_channel.c

hava_channel.exe: hava_channel.c
	cl /Zi hava_channel.c /link ws2_32.lib

clean:
	rm -f hava_channel hava_record
	rm -f hava_channel.obj hava_channel.exe hava_channel.pdb hava_channel.ilk vc80.pdb
