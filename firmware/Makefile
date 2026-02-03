include config.mk

# Source files from all modules
SRC=$(foreach m, $(MODULES), $(wildcard $(m)/src/*.c))

# Include directories
INCLUDES=$(foreach m, $(MODULES), -I$(m)/inc)

# Preprocessor defines
_DEFINES=$(foreach m, $(DEFINES), -D$(m))

# Build directory
BUILD_DIR=build

# Object files in build directory (preserve directory structure)
OBJECTS=$(patsubst %.c,$(BUILD_DIR)/%.o,$(SRC))
DEPS=$(patsubst %.c,$(BUILD_DIR)/%.d,$(SRC))

LDSCRIPT=ciaa_lpc4337.ld

ARCH_FLAGS=-mcpu=cortex-m4 -mthumb

TARGET=$(BUILD_DIR)/$(APP).elf
TARGET_BIN=$(BUILD_DIR)/$(APP).bin
TARGET_LST=$(BUILD_DIR)/$(APP).lst
TARGET_MAP=$(BUILD_DIR)/$(APP).map

ifeq ($(USE_FPU),y)
ARCH_FLAGS+=-mfloat-abi=hard -mfpu=fpv4-sp-d16
endif

CFLAGS=$(ARCH_FLAGS) $(INCLUDES) $(_DEFINES) -ggdb3 -O$(OPT) -ffunction-sections -fdata-sections
LDFLAGS=$(ARCH_FLAGS) -T$(LDSCRIPT) -nostartfiles -Wl,-gc-sections -Wl,-Map=$(TARGET_MAP) -Wl,--cref

ifeq ($(USE_NANO),y)
LDFLAGS+=--specs=nano.specs
endif

ifeq ($(SEMIHOST),y)
LDFLAGS+=--specs=rdimon.specs
endif

CROSS=arm-none-eabi-
CC=$(CROSS)gcc
LD=$(CROSS)gcc
SIZE=$(CROSS)size
LIST=$(CROSS)objdump -xdS
OBJCOPY=$(CROSS)objcopy
GDB=$(CROSS)gdb
OOCD=openocd

OOCD_SCRIPT?=ciaa-nxp.cfg

ifeq ($(VERBOSE),y)
Q=
else
Q=@
endif

all: $(TARGET) $(TARGET_BIN) $(TARGET_LST) size

-include $(DEPS)

# Rule to compile .c files to .o in build directory
$(BUILD_DIR)/%.o: %.c
	@echo CC $<
	@mkdir -p $(dir $@)
	$(Q)$(CC) -MMD $(CFLAGS) -c -o $@ $<

$(TARGET): $(OBJECTS)
	@echo LD $@
	@mkdir -p $(dir $@)
	$(Q)$(LD) $(LDFLAGS) -o $@ $(OBJECTS)

$(TARGET_BIN): $(TARGET)
	@echo BIN
	$(Q)$(OBJCOPY) -v -O binary $< $@

$(TARGET_LST): $(TARGET)
	@echo LIST
	$(Q)$(LIST) $< > $@

size: $(TARGET)
	$(Q)$(SIZE) $<

program: $(TARGET_BIN)
	@echo PROG
	$(Q)$(OOCD) -f $(OOCD_SCRIPT) \
	    -c "init" \
	    -c "halt 0" \
	    -c "flash erase_address 0x1A000000 0x80000" \
	    -c "flash write_image $< 0x1A000000 bin" \
	    -c "reset run" \
	    -c "shutdown" 2>&1

debug: $(TARGET_BIN)
	@echo DEBUG
	$(Q)$(OOCD) -f $(OOCD_SCRIPT) 2>&1

clean:
	@echo CLEAN
	$(Q)rm -rf $(BUILD_DIR)

.PHONY: all size clean program debug