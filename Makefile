all: hava_channel

windows: hava_channel.exe

hava_channel: hava_channel.c
	gcc -g -o hava_channel hava_channel.c

hava_channel.exe: hava_channel.c
	cl hava_channel.c /link ws2_32.lib

clean:
	rm -f hava_channel
	rm -f hava_channel.obj hava_channel.exe
