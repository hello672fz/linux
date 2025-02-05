#!/bin/bash

rm -rf mnt rootfs.img

mkdir mnt

## step1: make a rootfs file
fallocate -l 200GiB rootfs.img
mkfs.ext4 rootfs.img

## step2: mount rootfs
mount -o loop rootfs.img ./mnt

## step3: start a docker container
## Install necessary tools, such as vim, cmake, gcc ...
## Port ndctl
# docker pull guoweiu/ubuntu-rootfs:24.04
docker run -it -d guoweiu/ubuntu-rootfs:24.04

## step4: export the contents of the container
cid=`sudo docker ps | grep ubuntu-rootfs | awk '{print $1}'`
docker export ${cid} -o rootfs.tar

## step5: Extract .tar files
tar -xf rootfs.tar -C ./mnt

## step6: modify rootfs externally

## step7: umount rootfs
umount ./mnt

docker stop ${cid}
