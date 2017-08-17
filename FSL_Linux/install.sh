#!/bin/sh

mount /dev/sdc1 /mnt/tuna0
cp ./uImage /mnt/tuna0
sync
umount /mnt/tuna0

eject /dev/sdc
