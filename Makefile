obj-m += vgfbdev.o
vgfbdev-objs := vgfb.o vgfbmx.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
