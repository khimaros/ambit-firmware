# HACKING

The chip in the Palette Gear devices is an
[Atmel AT90USB1286](http://ww1.microchip.com/downloads/en/DeviceDoc/doc7593.pdf).

When in DFU bootloader mode, this is what lsusb shows:

```
Bus 001 Device 050: ID 03eb:2ffb Atmel Corp. at90usb AVR DFU bootloader
```

The AT90USB devices have three flash areas: **USER**, **EEPROM**, and **BOOT**.

USER and EEPROM are flashable from the DFU bootloader.

The BOOT section can only be written from ISP.

EEPROM can be written ~100,000 times (each block?)

The size of the bootloader is determined using fuses.

Fuse calculator: https://eleccelerator.com/fusecalc/fusecalc.php?chip=at90usb1286

See also: [HARDWARE.md](HARDWARE.md)

## Differences between Teensy++ 2.0 and Palette

LED high/low voltages have opposite behavior on Palette. The LEDs are powered
by default and a high voltage on the LED pins turns them off.

Teensy has a button to enter HWB so there is no need for the failsafe
bootloader jump. This saves EEPROM writes.

Teensy has a different bootloader HalfKay which is smaller thus BOOT
section starts at a higher memory address.

## Development environments

### Atmel Studio

TODO

### Arduino IDE

Clone https://github.com/DynamicPerception/ArduinoAT90USB

```
$ cd ArduinoAT90USB/
$ sudo cp hardware/DynamicPerception/avr/ /usr/share/arduino/hardware/
```

Custom bootloader for Teensy: http://blog.lincomatic.com/?p=548

https://www.avrfreaks.net/forum/at90usb1286-usb-dfu-bootloader

Restart or launch the Arduino IDE.

Random info:

https://savannah.nongnu.org/bugs/?44140

## Flashing

Palette devices are programmable using [dfu-programmer](https://github.com/dfu-programmer/dfu-programmer).

On Debian Buster, it is available via `apt install dfu-programmer`.

Alternatively, download and build:

```
$ git clone https://github.com/dfu-programmer/dfu-programmer
$ cd dfu-programmer
$ ./bootstrap.sh
$ ./configure
$ make
$ sudo make install
```

Connect the device and ensure you have perms on the USB device.
The included udev rules should work for most.

Now verify the dfu-programmer connection:

```
$ dfu-programmer at90usb1286 get bootloader-version
Bootloader Version: 0x01 (1)

$ dfu-programmer at90usb1286 get manufacturer
Manufacturer Code: 0x58 (88)
```

To boot back into the Palette firmware:

```
$ dfu-programmer at90usb1286 launch
```

## Bootloader Jump

Failsafe during early development and used for `{"boot":1}` command.

Resources:

https://www.pjrc.com/teensy/jump_to_bootloader.html

https://www.fourwalledcubicle.com/files/LUFA/Doc/120219/html/\_page\_\_software\_bootloader\_start.html

https://www.avrfreaks.net/forum/stupid-question-how-jump-bootloader-at90usb1287

https://github.com/WestfW/fusebytes/blob/master/fusebytes.ino

https://github.com/tmk/tmk_keyboard/blob/45e45691e0c03f1b5ea4341c6d90f9a09016a37d/tmk_core/common/avr/bootloader.c#L58

Bootloader location is determined by BOOTSZ fuses (word addressed).

Default BOOTSZ values for AT90USB128: BOOTSZ1 = 0, BOOTSZ0 = 0

Boot size configuration:

```
BOOTSZ1 | BOOTSZ0 | Boot reset word address | Boot reset byte address
--------+---------+-------------------------+------------------------
      1 |       1 | 0xFE00                  | 0x1FC0
      1 |       0 | 0xFC00                  | 0x1F80
      0 |       1 | 0xF800                  | 0x1F00
      0 |       0 | 0xF000                  | 0x1E00
```

Calculating byte address from word address:

```
>>> hex(int(0xf000 / 8))
'0x1e00'
```

## Timer configuration

https://www.pjrc.com/teensy/interrupts.html

### Glossary

- TCCR: Timer/Counter Control Register
- TCNT: Timer Counter
- TIFR: Timer Interrupt Flag Register
- TIMSK: Timer Interrupt Mask Register
- OCR: Output Compare Register
- OCF: Output Compare Flag
- COM: Compare Output Mode
- CTC: Clear Timer on Compare
- CS: Clock Select
- WGM: Wave Generation Mode
- ICNC: Input Capture Noise Canceler
- ICES: Input Capture Edge Select
- ICIE: Input Capture Interrupt Enable
- OCIE: Output Compare Match Interrupt Enable
- TOIE: Timer/Counter Overflow Interrupt Enable

```
BOTTOM | The counter reaches the BOTTOM when it becomes 0x00.
MAX    | The counter reaches its MAXimum when it becomes 0xFF (decimal 255).
TOP    | The counter reaches the TOP when it becomes equal to the highest value in the
       | count sequence. The TOP value can be assigned to be the fixed value 0xFF
       | (MAX) or the value stored in the OCR0A Register. The assignment is dependent
       | on the mode of operation.
```

TCCRnB values for controlling timer speed:

```
CSn2 | CSn1 | CSn0 | Description
-----+------+------+-------------------------------------------------------
 0   | 0    | 0    | No clock source. (Timer/Counter stopped)
 0   | 0    | 1    | clk / 1 (no prescaling)
 0   | 1    | 0    | clk / 8 (from prescaler)
 0   | 1    | 1    | clk / 64 (from prescaler)
 1   | 0    | 0    | clk / 256 (from prescaler)
 1   | 0    | 1    | clk / 1024 (from prescaler)
 1   | 1    | 0    | External clock source on Tn pin. Clock on falling edge
 1   | 1    | 1    | External clock source on Tn pin. Clock on rising edge
```

TCCRnB values for controlling Timer/Counter mode:

```
 WGMn3 | WGMn2  | WGMn1   | WGMn0   | Timer/Counter mode               | TOP    | Update of | TOVn flag
       | (CTCn) | (PWMn1) | (PWMn0) | of operation                     |        | OCRnx at  | set on
-------+--------+---------+---------+----------------------------------+--------+-----------+----------
 0     | 0      | 0       | 0       | Normal                           | 0xFFFF | Immediate | MAX
 0     | 0      | 0       | 1       | PWM, phase correct, 8-bit        | 0x00FF | TOP       | BOTTOM
 0     | 0      | 1       | 0       | PWM, phase correct, 9-bit        | 0x01FF | TOP       | BOTTOM
 0     | 0      | 1       | 1       | PWM, phase correct, 10-bit       | 0x03FF | TOP       | BOTTOM
 0     | 1      | 0       | 0       | CTC                              | OCRnA  | Immediate | MAX
 0     | 1      | 0       | 1       | Fast PWM, 8-bit                  | 0x00FF | TOP       | TOP
 0     | 1      | 1       | 0       | Fast PWM, 9-bit                  | 0x01FF | TOP       | TOP
 0     | 1      | 1       | 1       | Fast PWM, 10-bit                 | 0x03FF | TOP       | TOP
 1     | 0      | 0       | 0       | PWM, phase and frequency Correct | ICRn   | BOTTOM    | BOTTOM
 1     | 0      | 0       | 1       | PWM, phase and frequency Correct | OCRnA  | BOTTOM    | BOTTOM
 1     | 0      | 1       | 0       | PWM, phase correct               | ICRn   | TOP       | BOTTOM
 1     | 0      | 1       | 1       | PWM, phase correct               | OCRnA  | TOP       | BOTTOM
 1     | 1      | 0       | 0       | CTC                              | ICRn   | Immediate | MAX
 1     | 1      | 0       | 1       | (Reserved)                       | –      | –         | –
 1     | 1      | 1       | 0       | Fast PWM                         | ICRn   | TOP       | TOP
 1     | 1      | 1       | 1       | Fast PWM                         | OCRnA  | TOP       | TOP
```

It's important to disable interrupts and save SREG when reading or writing TCNT, OCRnA/B/C or ICRn.

```
unsigned char sreg;
unsigned int i;

sreg = SREG;

cli();

TCNTn = i;
SREG = sreg;

sei();
```

## USB Communication

http://www.avrbeginners.net/new/wp-content/uploads/2011/10/avrbeginners_40_USB_Control_Transfers_with_LUFA_1.1.pdf

CTC communication?

Relevant examples in LUFA:

 - Demos/Device/LowLevel/VirtualSerial/VirtualSerial.c
 - Demos/Device/ClassDriver/VirtualSerial/VirtualSerial.c

Including LUFA modules in existing project:

https://www.fourwalledcubicle.com/files/LUFA/Doc/120730/html/_page__configuring_apps.html#Sec_AppConfigParams

USART?

https://www.fourwalledcubicle.com/files/LUFA/Doc/120219/html/_page__alternative_stacks.html

http://www.ssalewski.de/AT90USB_firmware.html.en

http://www.obdev.at/products/vusb/index.html

https://dicks.home.xs4all.nl/avr/usbtiny/

## JSON Decoding

https://ww1.microchip.com/downloads/en/Appnotes/90003239A.pdf

https://github.com/MicrochipTech/json_decoder

https://github.com/zserge/jsmn

http://www.catb.org/esr/microjson/

## Screen Driver

The display is a Solomon Systech SSD1351 powered by an ATmega328P

Probably SPI between the ATmega328P and SSD1351.

Unclear what the communication between microcontrollers is.

http://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf

Arduino driver: https://github.com/sumotoy/SSD_13XX

C driver: https://github.com/Gecko05/SSD1351-Driver-Library

May also be a TWI interface: https://ww1.microchip.com/downloads/en/Appnotes/doc1981.pdf

## Component Driver

Certainly TWI.

Might be using dynamic address resolution:

https://hackaday.io/project/2738-intelligent-shields/log/8519-dynamic-address-resolution

Could be a TWI/I2C connection given connection to SDA and SCL

Devices are addressed using a 7-bit address (coordinated by Philips) transfered as the first byte after the so-called start condition. The LSB of that byte is R/~W, i. e. it determines whether the request to the slave is to read or write data during the next cycles. (There is also an option to have devices using 10-bit addresses but that is not covered by this example.)

https://www.nongnu.org/avr-libc/user-manual/group__twi__demo.html

https://www.exploreembedded.com/wiki/Basics_of_I2C_with_AVR

https://en.wikipedia.org/wiki/I%C2%B2C

TWI address scanning is possible:

https://hackaday.com/2015/06/25/embed-with-elliot-i2c-bus-scanning/

https://github.com/RobTillaart/Arduino/tree/master/sketches/MultiSpeedI2CScanner

https://www.instructables.com/id/ESP8266-I2C-PORT-and-Address-Scanner/

https://github.com/jainrk/i2c_port_address_scanner/blob/master/i2c_port_address_scanner/i2c_port_address_scanner.ino

As is sniffing:

http://alex.kathack.com/codes/avr/i2c_sniffer/index.html

## LED Driver

Main processor has two 8-bit and two 16-bit timers.

LEDs modulation is done using a single 8-bit timer (Timer/Counter 1)

Frequency for persistence of vision must be at least 100Hz.

In other words, LED must not be off for more than 10ms.

The duty cycle of the LED determines the brightness.

Resources:

https://sites.google.com/site/qeewiki/books/avr-guide/pwm-on-the-atmega328

https://electronics.stackexchange.com/questions/52110/fading-rgb-led-with-attiny2313-timer

https://www.avrfreaks.net/forum/3-pwm-rgb-led

https://electronics.stackexchange.com/questions/424594/regulate-an-rgb-led-via-pwm-both-in-brightness-and-in-hue

https://github.com/avrxml/asf/blob/master/thirdparty/qtouch/devspecific/sam0/samd/examples/example_selfcap/main.c

### LED brightness correction

Brightness is non-linear `pow(pow(255, n / 255), 1.4)` is better.

https://ledshield.wordpress.com/2012/11/13/led-brightness-to-your-eye-gamma-correction-no/

https://electronics.stackexchange.com/questions/1983/correcting-for-non-linear-brightness-in-leds-when-using-pwm

Sigmoid curves might be the right way to go:

https://blog.arkieva.com/basics-on-s-curves/#:~:text=S%20Curve%20Logistics%20Equation&text=S(x)%20%3D%20(1,the%20formula%20I%20use%2C%20where
