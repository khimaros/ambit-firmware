# ambit-firmware

This alternative firmware for Palette devices is a work in progress.

It is not recommended to flash this firmware to a Palette device,
as you may not be able to return to the bootloader.

See [ROADMAP.md](ROADMAP.md) for current feature status.

## Build

Clone the repository and submodules:

```
$ git clone --recurse-submodules ...
```

Build the firmware image:

```
$ make
```

## Flash

The recommended development board is a Teensy++ 2.0.

For easiest development, the version with preinstalled jumper
pins is recommended for easy insertion into a breadboard.

For the best experience, attach LEDs to pins C4, C5, and C6
(red, blue, and green) and back to GND.

See `docs/HARDWARE.md` for more detailed pin-out information.
A schematic is not yet available.

To flash the Teensy development board:

```
$ make teensy-ee
<press HW RST button>
$ make teensy
```

### Production

If you do want to run this on a Palette device, you should be
familiar with the ISP flashing process and be comfortable opening
the Palette base and attaching to the MISO/MOSI headers in case
the failsafe bootloader does not work.

For correct behavior on a Palette device, the LED voltages also
need to be reversed. It is **strongly recommended** to enable
the failsafe bootloader.

Ensure `Makefile` matches the following:

```
#CDEFS        += -DDISABLE_FAILSAFE_BOOTLOADER
CDEFS        += -DLED_REVERSE_POLARITY
```

To flash the production Palette:

```
$ make
$ dfu-programmer at90usb1286 erase --force
$ dfu-programmer at90usb1286 flash --eeprom Ambit.eep
$ dfu-programmer at90usb1286 flash Ambit.hex
$ dfu-programmer at90usb1286 launch
```

## Test

A few ambit demos are known to work with the custom firmware:

```
$ cd ../
$ make start-debug
$ make demoscene
$ make console
$ make reboot_bootloader
```

## Failsafe bootloader

**NOTE**: the failsafe bootloader will always jump to the bootloader if
the EEPROM is not flashed. This is the case with `make teensy` as the
teensy CLI loader does not support .eep files. To test the failsafe
bootloader with a Teensy device, you must use the graphical loader app.

By default the EEPROM is programmed to `EEPROM_BOOT_START_FIRMWARE`, causing
it to execute the ambit firmware. Early the boot process, the EEPROM value is
checked. If it is set to anything other than `EEPROM_BOOT_START_FIRMWARE`, an
immediate jump to the bootloader is made.

If the conditional jump is not executed, the EEPROM value is then set to
`EEPROM_BOOT_START_BOOTLOADER` and execution continues as usual. If the device
freezes or is reset at this point, it will jump to the bootloader on any
subsequent boot.

This failsafe bootloader EEPROM slot has two possible values: 0, 128

If the EEPROM slot contains anything other than 128, jump to the bootloader
will be executed. If the EEPROM slot contains 128, normal firmware
execution continues.

These numbers were chosen so that if the EEPROM flash fails, the default
will be to boot back into the bootloader as the default EEPROM values
after DFU erase are all 0x0.

Prior to jump, LED ports are configured and a POST LED sequence is executed.

## Performance

The two most important LUFA related constants for tuning USB performance
are the `USB_STREAM_TIMEOUT_MS` value in `Config/LUFAConfig.h` and the
value of `CDC_TXRX_EPSIZE` in `Descriptors.h`.
