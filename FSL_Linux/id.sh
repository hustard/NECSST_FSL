#!/bin/sh

sudo mount /dev/sdb1 tmp/
sudo cp /tftpboot/devicetree.dtb ./tmp/
sudo umount tmp
