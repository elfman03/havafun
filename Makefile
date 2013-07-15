RELEASE=PREalpha4

all: 

release: dist release_win release_lin

dist:
	mkdir dist

release_win: LICENSE.txt hava/RELEASE_NOTES.html hava/example_live.cmd hava/hava_channel.exe hava/hava_channel_nowin.exe hava/hava_info.exe hava/hava_record.exe
	zip dist/havafun-$(RELEASE)-win.zip LICENSE.txt \
	    hava/RELEASE_NOTES.html \
	    hava/example_live.cmd \
	    hava/hava_channel.exe \
	    hava/hava_channel_nowin.exe \
	    hava/hava_info.exe \
	    hava/hava_record.exe

release_lin: LICENSE.txt hava/RELEASE_NOTES.html hava/hava_channel hava/hava_record hava/hava_info hava/example_combined.sh hava/example_live.sh hava/example_record.sh
	tar -czvf dist/havafun-$(RELEASE)-lin.tar.gz LICENSE.txt \
	    hava/RELEASE_NOTES.html \
	    hava/hava_channel \
	    hava/hava_record \
	    hava/hava_info \
	    hava/example_combined.sh \
	    hava/example_live.sh \
	    hava/example_record.sh

hava/hava_channel.exe hava/hava_channel_nowin.exe hava/hava_info.exe hava/hava_record.exe hava/hava_channel hava/hava_info hava/hava_record: hava/havafun-alpha3-win.zip hava/havafun-alpha3-lin.tar.gz
	unzip -o -d hava hava/havafun-alpha3-win.zip hava_channel.exe hava_channel_nowin.exe hava_info.exe hava_record.exe
	tar -C hava --overwrite -zxvf hava/havafun-alpha3-lin.tar.gz hava_channel hava_record hava_info

clean:
	rm -f hava/hava_channel hava/hava_record hava/hava_info
	rm -f hava/hava_channel.exe hava/hava_channel_nowin.exe hava/hava_record.exe hava/hava_info.exe
