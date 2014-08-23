export TZ_DIR = $(CURDIR)

# # T-OS 目标文件名
T-OS = $(TZ_DIR)/bin/t-os.bin
# # 内核镜像目标文件名
K_IAMGE = $(TZ_DIR)/bin/linux_zimage

# trustzone linux 驱动
TZ_DRIVER = $(TZ_DIR)/bin/tz_driver.ko

# client api 目标共享链接库
TZ_API = $(TZ_DIR)/bin/tz_api.so

# 测试字符串服务DEMO
STRING_DEMO = $(TZ_DIR)/bin/test_string_task.elf

# 测试通信服务DEMO
COMMUNICATION = $(TZ_DIR)/bin/test_communication_task.elf

.PHONY: all 
all: $(T-OS) $(K_IAMGE) $(TZ_DRIVER) $(TZ_API) $(STRING_DEMO) $(COMMUNICATION)

	@echo "===================================="
	@echo "目标文件存放至$(TZ_DIR)/bin目录下"
	@ls $(TZ_DIR)/bin
	@echo "===================================="
$(T-OS):
	@echo "===================================="
	@echo "            编译T-OS"
	@echo "===================================="
	$(MAKE) -C $(TZ_DIR)/t-os
	@mkdir -p `dirname $@`
	cp $(TZ_DIR)/t-os/build/t-os.bin $@

$(K_IAMGE): 
	@echo "===================================="
	@echo "          编译linux镜像"
	@echo "===================================="
#	$(MAKE) -C $(TZ_DIR)/kernel
	@mkdir -p `dirname $@`
	cp $(TZ_DIR)/kernel/s5pv210_zImage $@
$(TZ_DRIVER):
	@echo "===================================="
	@echo "          编译trustzone linux驱动"
	@echo "===================================="
#	$(MAKE) -C $(TZ_DIR)/tz_driver
	@mkdir -p `dirname $@`
	cp $(TZ_DIR)/tz_driver/tz_driver.ko $@


$(TZ_API):
	@echo "===================================="
	@echo "          编译tz_api.o"
	@echo "===================================="
	$(MAKE) -C $(TZ_DIR)/tz_api/jni
	@mkdir -p `dirname $@`
	cp $(TZ_DIR)/tz_api/libs/armeabi/tz_api.so $@

$(STRING_DEMO):
	@echo "===================================="
	@echo "          编译测试字符串服务DEMO"
	@echo "===================================="
	$(MAKE) -C $(TZ_DIR)/test_api/test_string_task/jni
	@mkdir -p `dirname $@`
	cp $(TZ_DIR)/test_api/test_string_task/libs/armeabi/test_string_task.elf $@

$(COMMUNICATION):
	@echo "===================================="
	@echo "          编译测试测试通信服务DEMO"
	@echo "===================================="
	$(MAKE) -C $(TZ_DIR)/test_api/test_communication_task/jni
	@mkdir -p `dirname $@`
	cp $(TZ_DIR)/test_api/test_communication_task/libs/armeabi/test_communication_task.elf $@

clean:
	$(MAKE) -C $(TZ_DIR)/t-os clean
#	$(MAKE) -C $(TZ_DIR)/kernel clean
#	$(MAKE) -C $(TZ_DIR)/tz_driver clean
	$(MAKE) -C $(TZ_DIR)/tz_api/jni clean
	$(MAKE) -C $(TZ_DIR)/test_api/test_string_task/jni clean
	$(MAKE) -C $(TZ_DIR)/test_api/test_communication_task/jni clean
	rm -rf $(TZ_DIR)/bin

