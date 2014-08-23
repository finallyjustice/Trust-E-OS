#
#
# 相关目录
TOP_DIR = $(CURDIR)
BUILD_DIR = $(TOP_DIR)/build
DEPEND_DIR = $(TOP_DIR)/depend
SRC_DIR = $(TOP_DIR)/src

# 指定ARM平台
# 可选值： S5PV210,FVP_EB_Cortex_A8
PLATFORM = FVP_EB_Cortex_A8

# Debug 
# if want to debug printf please set DEBUG=TRUE
DEBUG = TRUE

# Client Emulator or not?
CLIENT_EMULATOR = TRUE

# 交叉编译工具链
CROSS_COMPILE =  arm-linux-

# 源码目录
ARCH_DIR = $(SRC_DIR)/arch
BOARD_DIR = $(SRC_DIR)/board
LIB_DIR = $(SRC_DIR)/lib

INCLUDE_DIR = -I$(ARCH_DIR)/include
INCLUDE_DIR += -I$(BOARD_DIR)/include
INCLUDE_DIR += -I$(LIB_DIR)/include

# objs
ARCH_OBJS_MK = $(ARCH_DIR)/objects.mk
BOARD_OBJS_MK = $(BOARD_DIR)/objects.mk
LIB_OBJS_MK = $(LIB_DIR)/objects.mk


include $(ARCH_OBJS_MK)
include $(BOARD_OBJS_MK)
include $(LIB_OBJS_MK)

OBJS_Y += $(foreach obj,$(arch-objs-y),$(BUILD_DIR)/arch/$(obj))
OBJS_Y += $(foreach obj,$(board-objs-y),$(BUILD_DIR)/board/$(obj))
OBJS_Y += $(foreach obj,$(lib-objs-y),$(BUILD_DIR)/lib/$(obj))


MID_DEPS_Y = $(patsubst %.o,%.d,$(OBJS_Y))
DEPS_Y = $(subst $(BUILD_DIR),$(DEPEND_DIR),$(MID_DEPS_Y))

# 链接脚本
LD_SCRIPT = $(SRC_DIR)/linker.ld

# 编译工具及选项
LD = $(CROSS_COMPILE)gcc
CC = $(CROSS_COMPILE)gcc
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump
LDFLAGS = -g -Wall -nostdlib -Wl,-T$(LD_SCRIPT)

CFLAGS = -c -g -mcpu=cortex-a8 -mlittle-endian -Wall -nostdlib
CFLAGS += $(INCLUDE_DIR)
CFLAGS += -D$(PLATFORM)
ifeq ($(DEBUG),TRUE)
	CFLAGS += -DDEBUG
endif
# CFLAGS += -DFVB_EB_Cortex_A8
# 目标产生文件
ELF_FILE = $(BUILD_DIR)/client.elf
BIN_FILE = $(BUILD_DIR)/client.bin
MAP_FILE = $(BUILD_DIR)/client.map

.PHONY : all clean

all : $(BIN_FILE) $(MAP_FILE) 
	@echo "------平台 $(PLATFORM) Client 编译完成------"
# 依赖文件
$(DEPEND_DIR)/%.d : $(SRC_DIR)/%.c
	@mkdir -p `dirname $@`
	@echo 更新依赖文件 $(notdir $@)
	@$(CC) $(CFLAGS) -MM $< | sed -e 1's,^,$(BUILD_DIR)/,' > $@
sinclude $(DEPS_Y)

$(BIN_FILE) : $(ELF_FILE)
	@echo 转换 $(notdir $<) 为 $(notdir $@) 
	@$(OBJCOPY) -O binary $< $@

$(MAP_FILE) : $(ELF_FILE)
	@echo 反编译 $(notdir $<) 为 $(notdir $@)
	@$(OBJDUMP) -htdalD $(ELF_FILE) > $(MAP_FILE)
#	@$(OBJDUMP) -alD $(ELF_FILE) > $(MAP_FILE)

$(ELF_FILE) : $(OBJS_Y) $(LD_SCRIPT)
	@mkdir -p `dirname $(ELF_FILE)`
	@echo 链接 objects 为 $(notdir $@)
	@$(LD) $(OBJS_Y) $(LDFLAGS) -o $@

$(BUILD_DIR)/%.o : $(SRC_DIR)/%.S
	@echo 编译 $(notdir $<) 为 $(notdir $@)
	@mkdir -p `dirname $@`
	@$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o : $(SRC_DIR)/%.c
	@echo 编译 $(notdir $<) 为  $(notdir $@)
	@mkdir -p `dirname $@`
	@$(CC) $(CFLAGS) $< -o $@


 

.PHONY : clean
clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(DEPEND_DIR)
show :
	@echo "hello world"
	@echo $(OBJS_Y)