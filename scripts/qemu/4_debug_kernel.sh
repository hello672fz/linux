#!/bin/bash

## start qemu
sudo /opt/qemu-2024-06-21/bin/qemu-system-x86_64 \
-m 32G \
-device e1000,netdev=net0 \
-netdev user,id=net0,hostfwd=tcp::8088-:22 \
-nographic \
-kernel arch/x86/boot/bzImage \
-append "console=ttyS0 root=/dev/sda rw init=/lib/systemd/systemd nokaslr" \
-initrd $1/initramfs.igz \
-drive format=raw,file=$1/rootfs.img \
-S \
-gdb tcp::1240
