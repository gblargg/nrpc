all:
	ca65 shell.s -DSERIAL_PAL=0
	ld65 -C shell.cfg -o shell-ntsc.bin shell.o
	
	ca65 shell.s -DSERIAL_PAL=0 -DSERIAL_FAST=1
	ld65 -C shell.cfg -o shell-ntsc-fast.bin shell.o
	
	ca65 shell.s -DSERIAL_PAL=1
	ld65 -C shell.cfg -o shell-pal.bin shell.o
	
	ca65 shell.s -DSERIAL_PAL=1 -DSERIAL_FAST=1
	ld65 -C shell.cfg -o shell-pal-fast.bin shell.o
	
	ca65 shell.s -DSERIAL_SYNC=1
	ld65 -C shell.cfg -o shell-sync.bin shell.o
	
	ca65 shell.s -DSERIAL_SYNC=1 -DSERIAL_FAST=1
	ld65 -C shell.cfg -o shell-sync-fast.bin shell.o
	
	rm *.o
	
	cat shell-ntsc.bin shell-ntsc-fast.bin shell-pal.bin shell-pal-fast.bin shell-sync.bin shell-sync-fast.bin >shell-all.bin
	
	echo "static const unsigned char nrpcc_shell [] = {" >nrpcc_shell.h
	hexdump -v -e '1/1 "0x%02x,\n"' shell-all.bin >>nrpcc_shell.h
	echo "};" >>nrpcc_shell.h
	mv nrpcc_shell.h ..

clean:
	rm *.bin
