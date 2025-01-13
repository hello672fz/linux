#!/bin/bash

## start qemu
sudo /opt/qemu-2024-06-21/bin/qemu-system-x86_64 \
-m 4G \
-nographic \
-kernel arch/x86/boot/bzImage \
-append "console=ttyS0 root=/dev/sda rw init=/lib/systemd/systemd nokaslr" \
-initrd $1/initramfs.igz \
-M q35,cxl=on \
-drive format=raw,file=$1/rootfs.img \
-object memory-backend-ram,id=vmem0,share=on,size=256M \
-device pxb-cxl,bus_nr=12,bus=pcie.0,id=cxl.1 \
-device cxl-rp,port=0,bus=cxl.1,id=rp0,chassis=0,slot=2 \
-device cxl-type3,bus=rp0,volatile-memdev=vmem0,id=cxl-vmem0 \
-M cxl-fmw.0.targets.0=cxl.1,cxl-fmw.0.size=4G \