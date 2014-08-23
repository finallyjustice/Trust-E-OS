

TZ_DIR = /home/sky/Desktop/source/trustzone-all/t-osv0.1/trustzone
ifndef TZ_DIR
$(error 你必须设置TZ_DIR)
endif
KDIR = /home/sky/Desktop/source/trustzone-all/utv210-kernel
KERNEL_BUILT_DIR = /home/sky/Desktop/source/trustzone-all/utv210-kernel
obj-m := tz_driver.o
PWD := $(TZ_DIR)/tz_driver

ccflags-y:= -I$(PWD)/../include

all: target_modules

target_modules:
	$(MAKE) ARCH=arm -C $(KDIR) O=$(KERNEL_BUILT_DIR)  M=$(PWD) modules
clean:
	$(MAKE) ARCH=arm -C $(KDIR) M=$(PWD) clean
