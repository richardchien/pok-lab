export POK_PATH=$(CURDIR)/../../
export ARCH=x86
export BSP=x86-qemu

include $(POK_PATH)/misc/mk/config.mk
include $(POK_PATH)/misc/mk/common-$(ARCH).mk

TARGET = $(shell pwd)/pok.elf
ARCHIVE = $(shell pwd)/partitions.cpio
PARTITIONS = prog1/prog1.elf prog2/prog2.elf
KERNEL = kernel/kernel.lo

all: clean build-regular

build-regular: build-kernel partitions $(TARGET)

compiletest: all

build-kernel:
	$(CD) kernel && $(MAKE)

partitions:
	$(CD) prog1 && $(MAKE) 
	$(CD) prog2 && $(MAKE) 

clean: common-clean
	$(RM) node_impl
	$(CD) kernel && $(MAKE) clean
	$(CD) prog1 && $(MAKE) clean
	$(CD) prog2 && $(MAKE) clean

distclean: clean
	$(CD) kernel && $(MAKE) distclean
	$(CD) prog1 && $(MAKE) distclean
	$(CD) prog2 && $(MAKE) distclean

include $(POK_PATH)/misc/mk/rules-common.mk
include $(POK_PATH)/misc/mk/rules-main.mk
include $(POK_PATH)/misc/mk/install-rules.mk
