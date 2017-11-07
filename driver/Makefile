obj-m += vgfbdev.o
ccflags-y := -Wall -Werror -Og -g
vgfbdev-objs := vgfb.o vgfbmx.o mode.o mode_normal.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
