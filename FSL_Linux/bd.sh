#!/bin/sh

make ARCH=arm CROSS_COMPILE=arm-xilinx-linux-gnueabi- zynq-zc706.dtb && \
		  cp arch/arm/boot/dts/zynq-zc706.dtb /tftpboot/devicetree.dtb
