# HARDWARE

There are two boards on the Palette devices: main board and screen.

https://www.wellpcb.com/special/identifying-circuit-board-parts.html

## Mainboard

The main board has jumper cables to each of the magnetic ports and
sports an Atmel AT90USB1286-MU.

https://ww1.microchip.com/downloads/en/DeviceDoc/doc7593.pdf

https://github.com/abcminiuser/lufa

Port pins are numbered left to right assuming orientation of top port.

First pin of each port connector are connected.

The known pin-out configuration:

```
PE6: None
PE7: None

UVcc: C8 capacitor
D-: USB D-
D+: USB D+
UGnd: C2 capacitor
UCap: C2 capacitor
VBus:

PE3: None

PB0: None
PB1: [SCLK] Top right header #3
PB2: [MOSI] Top right header #4
PB3: [MISO] Top right header #1 (unsure, hard to see)
PB4: None
PB5: None
PB6: None
PB7: None

PE4: None
PE5: X3 / bottom port (square header); top-right header #5

RESET: connected to header near C2 capacitor

VCC: connected to GND (via resistor?)
GND: connected to VCC

XTAL2: Y1 crystal/oscillator (16.000 / S:07 / saw-wave)
XTAL1: Y1 crystal/oscillator (16.000 / S:07 / saw-wave)

PD0: [SCL] X1 / top port (pin 2); X2 / right port (pin 2); X3 / bottom port (pin 2); SCL header
PD1: [SDA] X1 / top port (pin 1); X2 / right port (pin 1); X3 / bottom port (pin 1); SDA header

PD2: Video ribbon
PD3: Near video ribbon
PD4: Near video ribbon
PD5: None
PD6: None
PD7: None

PE0: X2 / right side port (square header)
PE1: None

PC0: None
PC1: None
PC2: None
PC3: None
PC4: LED (R)
PC5: LED (B)
PC6: LED (G)
PC7: None

PE2: [HWB] HWB header

PA7: X1 / top port (square header)
PA6: None
PA5: None
PA4: None
PA3: None
PA2: None
PA1: None
PA0: None

VCC: to GND (via resistor?)
GND: to VCC (via resistor?)

PF7: None
PF6: None
PF5: None
PF4: None
PF3: None
PF2: None
PF1: None
PF0: None

AREF: None
GND: to copper pour (via resistor?)
AVCC: None
```

## Component

When viewing the component with male connector pointing upward:

Leftmost pin is GND, second from left is +5V.

Rightmost pin is SDA, second from right is SCL.

A third pin is attached to the connector which is used to detect
the presence of a device on that port. This is essentially an
out-of-band hack to support dynamic addressing on the I2C bus.

```
      _____   _____   _____       _____   _____   _____
 ___ [ GND ] [ +5V ] [     ] ___ [     ] [ SCL ] [ SDA ] ___
|                                                           |
|                                                           |
|               ___________________________                 |
|              /                           \                |
|             |           CONTROL           |               |
```


### ISP Programming

Connect a 6 pin ISP cable and ISP programming Atmel's bootloader

https://blog.rapid7.com/2019/04/16/extracting-firmware-from-microcontrollers-onboard-flash-memory-part-1-atmel-microcontrollers/

https://nvisium.com/blog/2019/08/07/extracting-firmware-from-iot-devices.html

https://libreboot.org/docs/install/rpi_setup.html

https://www.raspberrypi.org/documentation/usage/gpio/

https://therandombit.blogspot.com/2011/01/in-circuit-programmer-isp-how-to-atmel.html

## Screen

The screen board is very similar setup to the [Pixelduino](https://www.element14.com/community/community/arduino/blog/2014/12/22/the-pixelduino--a-tiny-arduino-with-15-color-oled-display-5v-boost-regulator-and-microsd).

It even has the same JST PH connector.

It is powered by an Atmel ATmega328P.

http://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf

The display is a Solomon Systech SSD1351.

The SSD1351 seems to be a 4-wire or 5-wire SPI interface.

The SSD1306 has I2C address 0x3D.

https://github.com/sumotoy/SSD_13XX

There are a few other chips which I'm not clear on:

 * adesto1422 (possibly flash for storing screen images?)

## Dial

Dial component has a potentiometer attached to an ATMega168A.

## Tools

### PCB Design Software

http://www.gpleda.org/

LibrePCB

### Development boards

Teensy++ 2.0

http://www.ssalewski.de/AT90USB_board.html.en

### Components

https://www.onlinecomponents.com/

### Oscilloscope

[Reverse Engineering I2C Signals](https://rheingoldheavy.com/i2c-signal-reverse-engineering/)

https://www.amazon.com/gp/product/B08832JX4H/ref=ox_sc_saved_title_3?smid=A2AX0Z128F82WQ&psc=1

https://www.amazon.com/Siglent-Technologies-SDS1202X-Oscilloscope-Channels/dp/B06XZML6RD/ref=sr_1_3?dchild=1&keywords=oscilloscope&qid=1592110922&s=industrial&sr=1-3

### RPi as meter

https://www.raspberrypi.org/blog/build-oscilloscope-raspberry-pi-arduino/

https://www.raspberrypi.org/forums/viewtopic.php?t=57480

https://www.raspberrypi.org/forums/viewtopic.php?t=123694

https://learn.adafruit.com/reading-a-analog-in-and-controlling-audio-volume-with-the-raspberry-pi?view=all

Teensy as an ADC: https://www.pjrc.com/teensy/adc.html

Two common ADCs: MCP3002, PCF8591

#### Teensy++ 2.0

Use Teensy + RPi as oscilloscope

https://github.com/Grumpy-Mike/Mikes-Pi-Bakery/tree/master/Arduino_Scope

https://create.arduino.cc/projecthub/javier-munoz-saez/2-oled-i2c-analog-pin-dynamic-plotting-46429b?ref=similar&ref_id=171605&offset=3

https://www.instructables.com/id/Girino-Fast-Arduino-Oscilloscope/

#### Arduino as logic analyzer

https://web.archive.org/web/20160329225834/https://johngineer.com/blog/?p=455

https://github.com/aster94/logic-analyzer

https://www.instructables.com/id/Arduinolyzerjs-Turn-your-Arduino-into-a-Logic-Anal/

https://www.element14.com/community/people/jancumps/blog/2015/03/13/make-a-logic-analyzer-from-your-dev-kit-part-1-arduino-uno

#### MCP3008

https://learn.adafruit.com/reading-a-analog-in-and-controlling-audio-volume-with-the-raspberry-pi?view=all

#### PCF8591

##### Information

http://wiringpi.com/extensions/i2c-pcf8591/

https://www.raspberrypi.org/forums/viewtopic.php?p=148900#p148900

https://www.raspberrypi.org/forums/viewtopic.php?f=44&t=50485&p=391450&hilit=+PCF8591#p391450

##### Devices

https://www.adafruit.com/product/1085

https://www.creatroninc.com/product/pcf8591-8-bit-i2c-adc-dac/?search_query=PRODA-008591&results=1

https://www.amazon.com/gp/product/B014KRGE12/ref=ox_sc_act_title_1?smid=ADHH624DX2Q66&psc=1

https://www.amazon.com/gp/product/B07DQGQYJW/ref=ox_sc_saved_title_1?smid=ATCVC199CHVK0&psc=1

https://www.amazon.com/gp/product/B013SMDGNY/ref=ox_sc_saved_title_2?smid=ADHH624DX2Q66&psc=1

