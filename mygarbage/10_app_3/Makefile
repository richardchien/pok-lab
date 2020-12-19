export POK_PATH=$(CURDIR)/../../
export ARCH=x86
export BSP=x86-qemu

include $(POK_PATH)/misc/mk/config.mk
include $(POK_PATH)/misc/mk/common-$(ARCH).mk

TARGET = $(shell pwd)/pok.elf
ARCHIVE = $(shell pwd)/partitions.cpio
PARTITIONS = prog_ctrl/prog_ctrl.elf prog_net/prog_net.elf prog_video/prog_video.elf
KERNEL = kernel/kernel.lo

all: clean build-regular

build-regular: build-kernel partitions $(TARGET)

compiletest: all

build-kernel:
	$(CD) kernel && $(MAKE)

partitions:
	$(CD) prog_ctrl && $(MAKE)
	$(CD) prog_net && $(MAKE)
	$(CD) prog_video && $(MAKE)

clean: common-clean
	$(RM) node_impl
	$(CD) kernel && $(MAKE) clean
	$(CD) prog_ctrl && $(MAKE) clean
	$(CD) prog_net && $(MAKE) clean
	$(CD) prog_video && $(MAKE) clean

distclean: clean
	$(CD) kernel && $(MAKE) distclean
	$(CD) prog_ctrl && $(MAKE) distclean
	$(CD) prog_net && $(MAKE) distclean
	$(CD) prog_video && $(MAKE) distclean

include $(POK_PATH)/misc/mk/rules-common.mk
include $(POK_PATH)/misc/mk/rules-main.mk
include $(POK_PATH)/misc/mk/install-rules.mk
