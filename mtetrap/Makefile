MKFILE_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
CURR_DIR := $(shell dirname $(MKFILE_PATH))
CURR_ARCH := $(shell uname -m)
UNAME_R := $(shell uname -r)
OBJNAME=mtetrap
PARALLEL_COUNT=$(shell nproc)

obj-m += $(OBJNAME)_module.o
# CHOSEN_DEBUG_FLAGS=-O0 -g -DDEBUG
# OPT_OR_DEBUG_FLAGS=$(CHOSEN_DEBUG_FLAGS)
CHOSEN_OPT_FLAGS=-O3
OPT_OR_DEBUG_FLAGS=$(CHOSEN_OPT_FLAGS)
ccflags-y= $(OPT_OR_DEBUG_FLAGS)
ldflags-y= $(OPT_OR_DEBUG_FLAGS)

QEMU_DEBUG_FLAGS+=
CFLAGS+=-march=armv8.5-a+memtag

ifeq ($(CURR_ARCH), aarch64)
CC=gcc
KERNEL_FLAGS=
else
CC=aarch64-linux-gnu-gcc
KERNEL_FLAGS=ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-
endif

ANDROID_KERNEL_FLAGS=CC=clang-17 CXX=clang++-17

all: mte-handler-bypass build-kernel-module

bootstrap:
	sudo apt install -y build-essential linux-headers-$(UNAME_R) dwarves pahole gcc-aarch64-linux-gnu gdb-multiarch libncurses-dev libelf-dev
	sudo cp /sys/kernel/btf/vmlinux /usr/lib/modules/$(UNAME_R)/build/
	sudo apt install -y qemu-system qemu-system-arm qemu-user-static qemu-user qemu-system-modules-spice qemu-system-gui virt-viewer qemu-kvm libvirt-daemon-system virtinst virtiofsd
	sudo adduser $(USER) libvirt

mte-handler-bypass: CFLAGS+=$(OPT_OR_DEBUG_FLAGS)

clean:
	make $(KERNEL_FLAGS) $(ANDROID_KERNEL_FLAGS) -j${PARALLEL_COUNT} -C ./android-kernel M=$(PWD) clean && \
	rm -f mte-handler-bypass

run-qemu-mte-example: mte-example
	qemu-aarch64 -L /usr/aarch64-linux-gnu/ ./mte-example

../mteqemu/initrd.gz:
	cd ../mteqemu && curl -O http://ftp.us.debian.org/debian/dists/stable/main/installer-arm64/current/images/cdrom/initrd.gz

../mteqemu/vmlinuz:
	cd ../mteqemu && curl -O http://ftp.us.debian.org/debian/dists/stable/main/installer-arm64/current/images/cdrom/vmlinuz

../mteqemu/debian-12.7.0-arm64-DVD-1.iso:
	cd ../mteqemu && curl -O -L https://cdimage.debian.org/debian-cd/current/arm64/iso-dvd/debian-12.7.0-arm64-DVD-1.iso

create-qemu: ../mteqemu/initrd.gz ../mteqemu/vmlinuz ../mteqemu/debian-12.7.0-arm64-DVD-1.iso
	mkdir -p ../mteqemu
	echo "Follow steps from https://www.willhaley.com/blog/debian-arm-qemu/ to setup qemu"
	cd ../mteqemu && \
		qemu-img create -f qcow2 debian-arm.sda.qcow2 128G && \
		qemu-system-aarch64 \
			-machine type=virt,mte=on \
			-cpu max \
			-m 8g \
			-smp 4 \
			-initrd "./initrd.gz" \
			-kernel "./vmlinuz" \
			-append "console=ttyAMA0" \
			-drive file=./debian-12.7.0-arm64-DVD-1.iso,id=cdrom,if=none,media=cdrom \
				-device virtio-scsi-device \
				-device scsi-cd,drive=cdrom \
			-drive file="./debian-arm.sda.qcow2",id=hd,if=none,media=disk \
				-device virtio-scsi-device \
				-device scsi-hd,drive=hd \
			-netdev user,id=net0,hostfwd=tcp::5555-:22 \
				-device virtio-net-device,netdev=net0 \
			-nographic
	echo "Once complete run the following in the qemu"
	echo "Check deb sources from https://wiki.debian.org/SourcesList"
	echo "Fix sudo https://wiki.debian.org/sudo/"
	echo "sudo apt install git build-essential cmake make"

create-snapshot:
	cd ../mteqemu && \
		qemu-img create -f qcow2 -b debian-arm.sda.snapshot-3.qcow2 -F qcow2 debian-arm.sda.snapshot-4.qcow2
	@echo ""
	@echo "!!!!!!!Update run-qemu to use the new snapshot"

run-qemu:
	cd ../mteqemu && qemu-system-aarch64 \
		$(QEMU_DEBUG_FLAGS) \
		-machine type=virt,mte=on \
		-cpu max \
		-m 16g \
		-smp 16 \
		-initrd "./initrd.img-from-guest-new5" \
		-kernel "./vmlinuz-from-guest-new5" \
		-append "console=ttyAMA0 root=/dev/sda2 nokaslr" \
		-drive file="./debian-arm.sda.snapshot-4.qcow2",id=hd,if=none,media=disk \
			-device virtio-scsi-device \
			-device scsi-hd,drive=hd \
		-netdev user,id=net0,hostfwd=tcp::5555-:22 \
			-device virtio-net-device,netdev=net0 \
		# -display gtk
		# -nographic
		-device virtio-serial -device virtserialport,chardev=spicechannel0,name=com.redhat.spice.0 -chardev spicevmc,id=spicechannel0,name=vdagent -display spice-app
		# -display default,gl=on,show-cursor=on \
		# -device virtio-gpu-pci \
		# -device virtio-keyboard-pci \
		# -device virtio-mouse-pci
		# -display none,show-cursor=on -vnc 0.0.0.0:52 \
		# -display default,show-cursor=on \
		# -device nec-usb-xhci,id=usb,addr=0x4 \
		# -device usb-mouse -device usb-kbd
		# -device usb-mouse
		# -device usb-kbd,bus=1,device=5
		# -display none -vnc 0.0.0.0:52

run-qemu-debug:
	QEMU_DEBUG_FLAGS="-s" $(MAKE) run-qemu

create-qemu-ubuntu: ../mteqemu/initrd.gz ../mteqemu/vmlinuz ../mteqemu/ubuntu-24.10-desktop-arm64.iso
	mkdir -p ../mteqemu
	echo "Follow steps from https://ubuntu.com/server/docs/boot-arm64-virtual-machines-on-qemu to setup qemu"
	cd ../mteqemu && \
		qemu-img create -f qcow2 ubuntu-arm.sda.qcow2 128G && \
		truncate -s 64m varstore.img && \
		truncate -s 64m efi.img && \
		dd if=/usr/share/qemu-efi-aarch64/QEMU_EFI.fd of=efi.img conv=notrunc && \
		sudo qemu-system-aarch64 \
			-machine type=virt,mte=on \
			-cpu max \
			-m 8g \
			-smp 4 \
			-display gtk \
			-drive if=pflash,format=raw,file=efi.img,readonly=on \
			-drive if=pflash,format=raw,file=varstore.img \
			-drive file=./ubuntu-24.10-desktop-arm64.iso,id=cdrom,if=none,media=cdrom \
				-device virtio-scsi-device \
				-device scsi-cd,drive=cdrom \
			-device virtio-net-device,netdev=net0 \
			-netdev user,id=net0,hostfwd=tcp::5555-:22
	echo "Once complete run the following in the qemu"
	echo "Check deb sources from https://wiki.debian.org/SourcesList"
	echo "Fix sudo https://wiki.debian.org/sudo/"
	echo "sudo apt install git build-essential cmake make"

linux-6.11.6.tar.xz:
	wget "https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-6.11.6.tar.xz"

linux-6.11.6: linux-6.11.6.tar.xz
	mkdir -p $@
	tar -xf $< -C $@ --strip-components 1
	cp .config $@/
	# Run in debian
	# make localmodconfig && \
	# scripts/config --disable MODULE_SIG && \
	# scripts/config --enable DEBUG_INFO && \
	# scripts/config --enable DEBUG_INFO_DWARF_TOOLCHAIN_DEFAULT && \

# Maybe needed for btfids
# cd linux-6.1.114/tools/bpf/resolve_btfids && make
# mkdir -p /usr/src/linux-headers-$(UNAME_R)/tools/bpf/resolve_btfids
# if [ ! -e /usr/src/linux-headers-$(UNAME_R)/tools/bpf/resolve_btfids ]; then \
# 	ln -s $(shell realpath linux-6.1.114/tools/bpf/resolve_btfids/resolve_btfids) /usr/src/linux-headers-$(UNAME_R)/tools/bpf/resolve_btfids; \
# fi

android-kernel:
	mkdir -p $@
	cd $@ && \
	git init && \
	git remote add origin https://android.googlesource.com/kernel/common && \
	git fetch --depth 1 origin bf4f9bc41c3b5203e1e7284e1de78e82f0630473 && \
	git checkout FETCH_HEAD
	cp pixel8_config $@/.config
	cp ./Module.symvers $@/Module.symvers

build-android-kernel: android-kernel
	cd android-kernel && \
	make $(KERNEL_FLAGS) $(ANDROID_KERNEL_FLAGS) -j${PARALLEL_COUNT} oldconfig && \
	make $(KERNEL_FLAGS) $(ANDROID_KERNEL_FLAGS) -j${PARALLEL_COUNT} modules_prepare

build-local-android-kernel:
	cd android-kernel && make $(KERNEL_FLAGS) $(ANDROID_KERNEL_FLAGS) -j${PARALLEL_COUNT}


build-kernel: linux-6.11.6
	rm -f ./linux-headers-6.11.6_6.11.6-1_arm64.deb ./linux-image-6.11.6_6.11.6-1_arm64.deb ./linux-image-6.11.6-dbg_6.11.6-1_arm64.deb ./linux-libc-dev_6.11.6-1_arm64.deb ./linux-upstream_6.11.6-1_arm64.buildinfo ./linux-upstream_6.11.6-1_arm64.changes
	cd linux-6.11.6 && \
		make $(KERNEL_FLAGS) -j${PARALLEL_COUNT} clean && \
		make $(KERNEL_FLAGS) -j${PARALLEL_COUNT} bindeb-pkg && \
		make $(KERNEL_FLAGS) -j${PARALLEL_COUNT} scripts_gdb
	echo "Run the create-snapshot target"

install-kernel:
ifeq ($(CURR_ARCH), aarch64)
	echo "Can only install on host"
else
	ssh localhost -p 5555 "rm -f ~/linux-*"
	scp -P 5555 ./linux-headers-6.11.6_6.11.6-1_arm64.deb ./linux-image-6.11.6_6.11.6-1_arm64.deb ./linux-image-6.11.6-dbg_6.11.6-1_arm64.deb ./linux-libc-dev_6.11.6-1_arm64.deb localhost:~
	ssh root@localhost -p 5555 "dpkg -i /home/$(USER)/linux-*; rm /boot/vmlinuz; rm /boot/initrd.img; ln -s /boot/vmlinuz-6.11.6 /boot/vmlinuz; ln -s /boot/initrd.img-6.11.6 /boot/initrd.img"
	scp -P 5555 localhost:/boot/vmlinuz  ../mteqemu/vmlinuz-from-guest-new5
	scp -P 5555 localhost:/boot/initrd.img  ../mteqemu/initrd.img-from-guest-new5
endif

build-kernel-module:
	make $(KERNEL_FLAGS) -j${PARALLEL_COUNT} -C ./linux-6.11.6 M=$(PWD) modules

build-android-kernel-module:
	make $(KERNEL_FLAGS) $(ANDROID_KERNEL_FLAGS) -j${PARALLEL_COUNT} -C ./android-kernel M=$(PWD) modules

clean-android-kernel-module:
	make $(KERNEL_FLAGS) -j${PARALLEL_COUNT} -C ./android-kernel M=$(PWD) clean

install-kernel-module: remove-kernel-module
ifeq ($(CURR_ARCH), aarch64)
	sudo insmod $(OBJNAME)_module.ko
else
	scp -P 5555 $(OBJNAME)_module.ko localhost:~
	ssh root@localhost -p 5555 "rmmod $(OBJNAME)_module 2>/dev/null; insmod /home/$(USER)/$(OBJNAME)_module.ko"
endif

install-android-kernel-module:
	scp mtetrap_module.ko XXXXXXXX.csres.XXXXXXXX.edu:~/mtetrap_module_android.ko
	ssh XXXXXXXX.csres.XXXXXXXX.edu "adb -s 41290DLJG000LP push ~/mtetrap_module_android.ko /data/androdeb/debian/mtetrap/mtetrap_module.ko"
	# Turn off selinux
	ssh XXXXXXXX.csres.XXXXXXXX.edu "adb shell setenforce 0"

magisk_patched-27000_HaAza.img:
	adb -s 41290DLJG000LP pull /sdcard/Download/magisk_patched-27000_HaAza.img ./

android-reroot: magisk_patched-27000_HaAza.img
	adb reboot bootloader
	fastboot flash init_boot ./magisk_patched-27000_HaAza.img
	fastboot reboot
	sleep 10
	adb -s 41290DLJG000LP root

remove-kernel-module:
ifeq ($(CURR_ARCH), aarch64)
	sudo rmmod $(OBJNAME)_module 2>/dev/null
else
	ssh root@localhost -p 5555 "rmmod $(OBJNAME)_module 2>/dev/null; echo done"
endif

run-gdb:
ifeq ($(CURR_ARCH), aarch64)
	echo "Can only run on host"
else
	ENTRY_POINT=$(shell readelf -h ./mte-handler-bypass | grep Entry | perl -lne 'print $$& if /Entry point address:\s+\K[^\s]+/'); \
	MODULE_LOC=$(shell ssh root@localhost -p 5555 "cat /sys/module/mtetrap_module/sections/.text"); \
	gdb-multiarch -ex "target remote localhost:1234" \
		-ex "b arch/arm64/kernel/probes/kprobes.c:329" \
		-ex "set confirm off" \
		-ex "add-symbol-file ./mte-handler-bypass $$ENTRY_POINT" \
		-ex "set confirm on" \
		linux-6.11.6/vmlinux
endif

# -ex "b *(&do_tag_check_fault + 4)" \
# -ex "add-symbol-file ./mtetrap_module.ko $$MODULE_LOC" \

install-mte-handler-bypass:
ifeq ($(CURR_ARCH), aarch64)
	echo "Can only run on host"
else
	ssh localhost -p 5555 "rm -f ~/mte-handler-bypass"
	scp -P 5555 ./mte-handler-bypass localhost:~/mte-handler-bypass
endif

run-mte-handler-bypass:
ifeq ($(CURR_ARCH), aarch64)
	./mte-handler-bypass
else
	ssh localhost -p 5555 "~/mte-handler-bypass"
endif

build_android:
	cmake \
		-DCMAKE_SYSTEM_NAME=Android \
		-DCMAKE_SYSTEM_VERSION=23 \
		-DCMAKE_ANDROID_NDK=$(shell realpath ~/Android/Sdk/ndk/28.0.12433566/) \
		-DCMAKE_ANDROID_ARCH_ABI=arm64v8-a+memtag \
		-S ./ -B $@

build_android/mte-handler-bypass-android: build_android
	cd build_android && make -j${PARALLEL_COUNT}

build-android-mte-handler-bypass: build_android/mte-handler-bypass-android

install-android-mte-handler-bypass:
	scp mte-handler-bypass XXXXXXXX.csres.XXXXXXXX.edu:~/Code/mtetrap/mte-handler-bypass
	ssh XXXXXXXX.csres.XXXXXXXX.edu "adb -s 41290DLJG000LP push ~/Code/mtetrap/mte-handler-bypass /data/androdeb/debian/mtetrap/mte-handler-bypass"
	# scp build_android/mte-handler-bypass-android XXXXXXXX.csres.XXXXXXXX.edu:~/Code/mtetrap/mte-handler-bypass-android
	# ssh XXXXXXXX.csres.XXXXXXXX.edu "adb -s 41290DLJG000LP push ~/Code/mtetrap/mte-handler-bypass-android /data/androdeb/debian/mtetrap/mte-handler-bypass-android"

boot_extracted.img:
	echo "Follow https://gist.github.com/gitclone-url/a1f693b64d8f8701ec24477a2ccaab87 this to extract boot_extracted.img"
	ssh XXXXXXXX.csres.XXXXXXXX.edu "adb -s 41290DLJG000LP pull /sdcard/Download/boot_extracted.img ./"
	scp XXXXXXXX.csres.XXXXXXXX.edu:~/boot_extracted.img ./boot_extracted.img

mkbootimg:
	git clone https://github.com/osm0sis/mkbootimg.git

mkbootimg/unpackbootimg: mkbootimg
	cd mkbootimg && CFLAGS="-Wno-stringop-overflow" make -j${PARALLEL_COUNT}

extract-symvers-ng:
	git clone https://github.com/bol-van/extract-symvers-ng

custom-Module.symvers: mkbootimg/unpackbootimg boot_extracted.img
	echo "Following https://xdaforums.com/t/dev-building-a-custom-kernel-and-kernel-modules-for-stock-kernel.3195492/"
	mkdir -p ./unpacked_image
	mkbootimg/unpackbootimg  -o ./unpacked_image -i boot_extracted.img
	lz4 -d ./unpacked_image/boot_extracted.img-kernel boot_uncompressed

# with fhsenv shell
p8-all: p8-mte-handler-bypass p8-build-android-kernel

p8-get-android-kernel:
	mkdir -p p8-android-kernel
	cd p8-android-kernel && \
	repo init -u https://android.googlesource.com/kernel/manifest -b android-gs-shusky-5.15-android14-qpr3 && \
	repo forall -c git checkout android-14.0.0_r0.98 && \
	repo sync -c -j$(PARALLEL_COUNT)

p8-build-android-kernel:
	cd p8-android-kernel && \
	./build_shusky.sh

M ?= $(shell pwd)
ANDROID_SRC ?= $(M)/p8-android-kernel
KERNEL_SRC ?= $(ANDROID_SRC)/out/cache/last_build/aosp/
ANDROID_NDK_ROOT ?= $(ANDROID_SRC)/prebuilts/ndk-r23
ANDROID_SYSROOT ?= $(ANDROID_SYSROOT)/toolchains/llvm/prebuilt/linux-x86_64/sysroot/

# include $(KERNEL_SRC)/../private/google-modules/soc/gs/Makefile.include
PBINPATH := $(ANDROID_SRC)/prebuilts/clang/host/linux-x86/clang-r487747c/bin/
# find . -name "zuma_shusky_env.sh"
FLAGS_FOR_KERNEL := ARCH=arm64 \
										CC=$(PBINPATH)clang \
										CXX=$(PBINPATH)clang++ \
										AR=$(PBINPATH)llvm-ar \
										LD=$(PBINPATH)ld.lld \
										NM=$(PBINPATH)llvm-nm \
										OBJCOPY=$(PBINPATH)llvm-objcopy \
										OBJDUMP=$(PBINPATH)llvm-objdump \
										STRIP=$(PBINPATH)llvm-strip \
										READELF=$(PBINPATH)llvm-readelf \
										HOSTCC=$(PBINPATH)clang \
										HOSTCXX=$(PBINPATH)clang++

p8-modules p8-clean:
	$(MAKE) $(FLAGS_FOR_KERNEL) -C $(KERNEL_SRC) M=$(M) KBUILD_EXTRA_SYMBOLS="$(EXTRA_SYMBOLS)" $(patsubst p8-%,%,$(@))

HOST_KERNEL_HEADERS := /lib/modules/$(shell uname -r)/build
a1-fedora-modules a1-fedora-clean:
	$(MAKE) -C $(HOST_KERNEL_HEADERS) M=$(M) $(patsubst a1-fedora-%,%,$(@))

p8-modules-install: p8-modules
	adb push mtetrap_module.ko /data/local/tmp/
	adb shell insmod /data/local/tmp/mtetrap_module.ko
	adb shell dmesg | grep mtetrap
	#adb shell lsmod | grep mtetrap

p8-modules-remove:
	adb shell rmmod mtetrap

# CC := $(PBINPATH)clang
# mte-handler-bypass: CFLAGS+=--target=aarch64-linux-android --sysroot=$(ANDROID_SYSROOT) $(OPT_OR_DEBUG_FLAGS) -no-pie -static

# use CC with specific default triple(aarch64-linux-android), as it doesn't need any sysroot or isystem dancing
p8-mte-handler-bypass: CC := $(ANDROID_SRC)/prebuilts/ndk-r23/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android31-clang
p8-mte-handler-bypass: CFLAGS+= -static
p8-mte-handler-bypass: mte-handler-bypass

p8-openssl-get:
	git clone -b openssl-3.0.13 git@github.com:openssl/openssl.git --depth 1

p8-openssl-make-patch:
	cd openssl && \
		git diff > $(PWD)/openssl-mtetrap.patch

p8-openssl-apply-patch:
	# go and run `git apply` by yourself
	# `git apply --reverse --check $$file` may help

# Define the list of build variants
VARIANTS := default mtesig mtekmod dr mprotsig

# Generate the full target names for both cross-compile and native builds
CROSS_TARGETS := $(addprefix p8-openssl-configure-and-compile-, $(addsuffix -cross, $(VARIANTS)))
NATIVE_TARGETS := $(addprefix p8-openssl-configure-and-compile-, $(addsuffix -native, $(VARIANTS)))

.PHONY: p8-openssl-configure-and-compile-cross p8-openssl-configure-and-compile-native clean

# Build all cross-compiled variants
p8-openssl-configure-and-compile-cross: $(CROSS_TARGETS)

# Build all native variants
p8-openssl-configure-and-compile-native: $(NATIVE_TARGETS)


# --- Target-Specific CFLAGS ---
# Define the specific CFLAGS for each build variant using a pattern.
# The '%' wildcard will match both '-cross' and '-native' suffixes.
p8-openssl-configure-and-compile-default-%: CFLAGS_EXTRA :=
p8-openssl-configure-and-compile-mtesig-%: CFLAGS_EXTRA := -DENABLE_MTE_SIG_WP
p8-openssl-configure-and-compile-mtekmod-%: CFLAGS_EXTRA := -DENABLE_MTE_KMOD_WP
p8-openssl-configure-and-compile-dr-%: CFLAGS_EXTRA := -DENABLE_DR_WP
p8-openssl-configure-and-compile-mprotsig-%: CFLAGS_EXTRA := -DENABLE_MPROT_WP


# --- Build Rules ---

# Rule for CROSS-COMPILING (cross targets)
$(CROSS_TARGETS):
	@echo "--- Cross-Compiling OpenSSL variant: $(patsubst p8-openssl-configure-and-compile-%-cross,%,$@) ---"
	# Using a subshell (...) ensures that 'cd' and 'export' don't affect other make processes.
	( \
		cd openssl && \
		export CC=aarch64-unknown-linux-gnu-gcc && \
		export LD=aarch64-unknown-linux-gnu-ld && \
		export RANLIB=aarch64-unknown-linux-gnu-ranlib && \
		export AR=aarch64-unknown-linux-gnu-ar && \
		./Configure -static linux-aarch64 $(CFLAGS) $(CFLAGS_EXTRA) && \
		make clean && \
		make -j$$(nproc) update && \
		make -j$$(nproc) apps/openssl \
	)
	# The final binary is copied and renamed based on the target name.
	cp openssl/apps/openssl openssl-$(patsubst p8-openssl-configure-and-compile-%-cross,%,$@)

# Rule for NATIVE COMPILATION (native targets)
$(NATIVE_TARGETS):
	@echo "--- Natively Compiling OpenSSL variant: $(patsubst p8-openssl-configure-and-compile-%-native,%,$@) ---"
	( \
		cd openssl && \
		./Configure -static $(CFLAGS) $(CFLAGS_EXTRA) && \
		make clean && \
		make -j$$(nproc) update && \
		make -j$$(nproc) apps/openssl \
	)
	# The final binary is copied and renamed based on the target name.
	cp openssl/apps/openssl openssl-$(patsubst p8-openssl-configure-and-compile-%-native,%,$@)

memtrace-benchmark-prep: p8-openssl-configure-and-compile-debian
	adb shell mkdir -p /data/androdeb/debian/root/memtrace
	adb push openssl-*-debian /data/androdeb/debian/root/memtrace
	adb push openssl-results/run.sh /data/androdeb/debian/root/memtrace
	# adb shell chown +x /data/androdeb/debian/root/memtrace/run.sh

memtrace-benchmark: memtrace-benchamrk-prep
	adb shell -t /data/androdeb/run-command /root/memtrace/run.sh

run-term:
	echo "Press Ctrl+B, D to detach"
	tmux attach-session -t qemumteterm || (tmux new -s qemumteterm)
