obj-m += exramfs.o

all:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build/ M=$(shell pwd)

clean:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build/ M=$(shell pwd) clean

test_run:
	sudo insmod ./exramfs.ko
	sudo mkdir -p /mnt/exramfs
	sudo mount -t exramfs -o size=512k exramfs /mnt/exramfs

test_clean:
	sudo umount /mnt/exramfs
	sudo rmmod ./exramfs.ko
