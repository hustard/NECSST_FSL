#!/bin/sh

sudo mount /dev/sdb1 tmp/
sudo cp /tftpboot/uImage ./tmp/
sudo umount tmp

sudo eject /dev/sdb
