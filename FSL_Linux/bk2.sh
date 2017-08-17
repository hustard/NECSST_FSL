#!/bin/sh

make ARCH=arm CROSS_COMPILE=arm-xilinx-linux-gnueabi- -j 4 UIMAGE_LOADADDR=0x00008000 uImage && \
		  cp arch/arm/boot/uImage /tftpboot/
