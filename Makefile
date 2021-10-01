CC=avr-gcc

# Run "make help" for target help.

MCU          = at90usb1286
ARCH         = AVR8
BOARD        = USER
F_CPU        = 16000000
F_USB        = $(F_CPU)
OPTIMIZATION = s
TARGET       = Ambit
CDEFS        =
SRC          = $(TARGET).c Descriptors.c microjson/mjson.c $(LUFA_SRC_USB) $(LUFA_SRC_USBCLASS) $(LUFA_SRC_PLATFORM)

CDEFS        += -DDISABLE_FAILSAFE_BOOTLOADER
#CDEFS        += -DLED_REVERSE_POLARITY

LUFA_PATH    = ./lufa/LUFA
LUFA_OPTS    = -DUSE_LUFA_CONFIG_HEADER -IConfig

CC_FLAGS     = $(CDEFS) $(LUFA_OPTS) -Imicrojson
LD_FLAGS     =

# Default target
all:

flash-production: Ambit.eep Ambit.hex
	dfu-programmer at90usb1286 erase --force
	dfu-programmer at90usb1286 flash --eeprom Ambit.eep
	dfu-programmer at90usb1286 flash Ambit.hex
	#dfu-programmer at90usb1286 launch

# Include LUFA-specific DMBS extension modules
DMBS_LUFA_PATH ?= $(LUFA_PATH)/Build/LUFA
include $(DMBS_LUFA_PATH)/lufa-sources.mk
include $(DMBS_LUFA_PATH)/lufa-gcc.mk

# Include common DMBS build system modules
DMBS_PATH      ?= $(LUFA_PATH)/Build/DMBS/DMBS
include $(DMBS_PATH)/core.mk
include $(DMBS_PATH)/cppcheck.mk
include $(DMBS_PATH)/doxygen.mk
include $(DMBS_PATH)/dfu.mk
include $(DMBS_PATH)/gcc.mk
include $(DMBS_PATH)/hid.mk
include $(DMBS_PATH)/avrdude.mk
include $(DMBS_PATH)/atprogram.mk

