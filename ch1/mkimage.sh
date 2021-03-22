#!/bin/sh
qemu-img create -f raw disk.img 200M
mkfs.fat -n 'MIKAN OS'  -s 2 -f 2 -R 32 -F 32 disk.img # volume-name=MIKAN OS, sector-of-cluster=2, number-of-fats=2, fat-size=32, number-of-reserved-sector=32
mkdir -p mnt
sudo mount -o loop disk.img mnt
sudo mkdir -p mnt/EFI/BOOT
sudo cp ./BOOTX64.EFI mnt/EFI/BOOTX64.EFI
sudo umount mnt
