#!/bin/sh

sudo mount /dev/sdb1 tmp/
sudo cp ./BOOT.bin ./tmp/
sudo umount tmp

sudo eject /dev/sdb
