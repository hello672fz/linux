#!/bin/bash

## step0: clean
rm -rf initramfs

## step1: create folder structure
initramfs_dirs=(bin sbin etc proc sys newroot)
for dir in ${initramfs_dirs[@]}; do mkdir -p initramfs/$dir; done

## step2: touch mdev_conf
## this is needed for new BusyBox?
touch initramfs/etc/mdev.conf

## step3: download BusyBox
wget https://busybox.net/downloads/binaries/1.31.0-defconfig-multiarch-musl/busybox-x86_64 -O initramfs/bin/busybox

chmod +x initramfs/bin/busybox

## step4: link busybox to sh
ln -s busybox initramfs/bin/sh

## step5: create init script
cat << EOF > initramfs/init

#Create all the symlinks to /bin/busybox
/bin/busybox --install -s /bin/
/bin/busybox --install -s /sbin/
mkdir /usr
ln -s /bin /usr/bin
ln -s /sbin /usr/sbin

#Mount things needed by this script
mount -t proc proc /proc
mount -t sysfs sysfs /sys

#Disable kernel messages from popping onto the screen
echo 0 > /proc/sys/kernel/printk

#Clear the screen
clear

#Create device nodes
mknod /dev/null c 1 3
mknod /dev/tty c 5 0
mdev -s

#Function for parsing command line options with "=" in them
get_opt() {
	echo "\$@" | cut -d "=" -f 2
}

#Defaults
init="/sbin/init"
root="/dev/hda1"

#Process command line options
#reads the contents of /proc/cmdline,
#which contains the kernel command line parameters
for i in \$(cat /proc/cmdline); do
	case \$i in
		root\=*)
			root=\$(get_opt \$i)
			;;
		init\=*)
			init=\$(get_opt \$i)
			;;
	esac
done

#Mount the root device
mount "\${root}" /newroot

#Check if \$init exists and is executable
if [[ -x "/newroot/\${init}" ]] ; then
	#Unmount all other mounts so that the ram used by
	#the initramfs can be cleared after switch_root
	umount /sys /proc

	#Switch to the new root and execute init
	exec switch_root /newroot "\${init}"
fi

#This will only be run if the exec above failed
echo "Failed to switch_root, dropping to a shell"
exec sh
EOF

chmod +x initramfs/init

## step6: create cpio and igz
find initramfs/ | cpio -H newc -o > initramfs.cpio
cat initramfs.cpio | gzip > initramfs.igz