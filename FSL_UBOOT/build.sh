#!/bin/sh


make CROSS_COMPILE=arm-xilinx-linux-gnueabi- && \
				   cp u-boot u-boot.elf && \
				   bootgen -image bootimage.bif -w on -o i BOOT.bin && \
				   cp BOOT.bin /tftpboot/
