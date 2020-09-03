obj-m += exchardev.o

all:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build/ M=$(shell pwd)
	$(CC) -o exapp exapp.c

clean:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build/ M=$(shell pwd) clean
	$(RM) exapp exapp.o

test_run:
	sudo insmod ./exchardev.ko
	sudo ./exapp

test_clean:
	sudo rmmod ./exchardev.ko
