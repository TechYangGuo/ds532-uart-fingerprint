# Fingerprint UART Driver Makefile
# Platform: IMX6ULL / Linux 4.9.88

obj-m += fingerprint_uart.o

KDIR := /home/book/Workspace/wds/sdk/imx6ull/100ask_imx6ull-sdk/Linux-4.9.88

ARCH_ARGS := CROSS_COMPILE=/home/book/Workspace/wds/sdk/imx6ull/100ask_imx6ull-sdk/Buildroot_2020.02.x/output/host/bin/arm-buildroot-linux-gnueabihf- ARCH=arm
CC := /home/book/Workspace/wds/sdk/imx6ull/100ask_imx6ull-sdk/ToolChain/arm-buildroot-linux-gnueabihf_sdk-buildroot/bin/arm-buildroot-linux-gnueabihf-gcc

PWD := $(shell pwd)

EXTRA_CFLAGS += -DDEBUG

all: 
	make $(ARCH_ARGS) -C $(KDIR) M=`pwd` modules 
	$(CC) fingerprint_test.c -o fingerprint_test

install:
	adb push *.ko fingerprint_test /root/smart_lock/driver/

clean: 
	make $(ARCH_ARGS) -C $(KDIR) M=$(PWD) modules clean 
	rm -f fingerprint_test

.PHONY: all install clean
