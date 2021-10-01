#include <avr/boot.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>

#include <util/delay.h>

#include "Descriptors.h"
#include "mjson.h"

#include <LUFA/Drivers/USB/USB.h>
#include <LUFA/Platform/Platform.h>

#define CPU_PRESCALE(n) (CLKPR = 0x80, CLKPR = (n))
#define CPU_16MHz       0x00

#ifndef DISABLE_FAILSAFE_BOOTLOADER
#define FAILSAFE_BOOTLOADER
#endif

#ifdef LED_REVERSE_POLARITY
#define LED_ON(n) PORTC &= ~_BV(n)
#define LED_OFF(n) PORTC |= _BV(n)
#else
#define LED_ON(n) PORTC |= _BV(n)
#define LED_OFF(n) PORTC &= ~_BV(n)
#endif

#define LED_R_ON() LED_ON(PC4)
#define LED_R_OFF() LED_OFF(PC4)
#define LED_G_ON() LED_ON(PC6)
#define LED_G_OFF() LED_OFF(PC6)
#define LED_B_ON() LED_ON(PC5)
#define LED_B_OFF() LED_OFF(PC5)

// At each timer overflow, we increment the overflow counter.
// The overflow counter rolls over at 255. So this will happen
// roughly 108 times per second. At this rate, the LED can be off
// for up to 9.2 ms, which should not result in any flickering.
#define LED_TIMER_SCALE 64

// The result is that an RGB value of 255 will always be higher
// than the overflow counter, so it will always be on. An RGB
// value of 128 will be higher 50% of the time and thus be off
// for 50% of the time (corresponds to it's duty cycle).
#define LED_VALUE_RANGE 255

// Minimum LED refresh rate in hertz.
// For persistance of vision effects, an LED must not be off for
// more than 1/100 of a second (10ms) at a time. Thus, the lower
// bound for dimness is to have the LED off for 10ms and then on
// for 10ms or longer.
#define LED_REFRESH_RATE 100

// Number of LED timer ticks before overflow interrupt.
// Assuming a clock rate of 16MHz, the processor runs 16,000,000
// cycles per second. With timer running at 1/64 clock interval,
// the timer counter will be decremented 250,000 times per second.
// With a timer counter of 9, the timer will overflow roughly
// 27,777 times per second.
#define LED_TIMER_COUNTER (uint16_t) (F_CPU / LED_TIMER_SCALE / LED_VALUE_RANGE / LED_REFRESH_RATE)

#define BOOTLOADER_START (FLASHEND - bootloader_size() + 1)

// Bootloader start address in bytes
#define EEPROM_BOOT_SLOT (uint8_t*) 0
#define EEPROM_BOOT_START_FIRMWARE 128
#define EEPROM_BOOT_START_BOOTLOADER 0
uint8_t EEMEM BootloaderMode = EEPROM_BOOT_START_FIRMWARE;

volatile uint16_t LedRedValue, LedGreenValue, LedBlueValue;

volatile unsigned int TimerLedOverflows;

volatile bool AwaitingLedRefresh = false;

char Response[1024];

USB_ClassInfo_CDC_Device_t VirtualSerial_CDC_Interface =
	{
		.Config =
			{
				.ControlInterfaceNumber   = INTERFACE_ID_CDC_CCI,
				.DataINEndpoint           =
					{
						.Address          = CDC_TX_EPADDR,
						.Size             = CDC_TXRX_EPSIZE,
						.Banks            = 1,
					},
				.DataOUTEndpoint =
					{
						.Address          = CDC_RX_EPADDR,
						.Size             = CDC_TXRX_EPSIZE,
						.Banks            = 1,
					},
				.NotificationEndpoint =
					{
						.Address          = CDC_NOTIFICATION_EPADDR,
						.Size             = CDC_NOTIFICATION_EPSIZE,
						.Banks            = 1,
					},
			},
	};

void reset_watchdog(void) {
	MCUSR &= ~(1 << WDRF);
	wdt_disable();
}

void reset_ports(void) {
	DDRC = 0;
	PORTC = 0;
}

void reset_timers(void) {
	cli();

	TIMSK1 = 0;
	TCCR1B = 0;
	TCNT1 = 0;
}

void reset_usb(void) {
	UDCON = 1;
	USBCON = (1 << FRZCLK);
	UCSR1B = 0;
	_delay_ms(50);
}

void reset_bootloader(void) {
	reset_watchdog();
	reset_timers();
	reset_usb();
	reset_ports();
}

void initialize_timer1_counter(void) {
	unsigned char sreg;
	sreg = SREG;
	cli();
	TCNT1 = LED_TIMER_COUNTER;
	SREG = sreg;
}

void reset_adc(void) {
	ADCSRA = 0;
}

void configure_cpu(void) {
	CPU_PRESCALE(CPU_16MHz);
}

void configure_ports(void) {
	DDRC |= _BV(DDC4);
	DDRC |= _BV(DDC5);
	DDRC |= _BV(DDC6);
}

void configure_timers(void) {
	// See docs/FIRMWARE.md#timerconfiguration
	// Atmel datasheet: Timer/Counter Control Register 14.8.2

	reset_timers();

	// PWM, phase and frequency correct.
	TCCR1B |= (1 << WGM13);

	// Invoke interrupt on overflow.
	TIMSK1 |= (1 << TOIE1);

	initialize_timer1_counter();

	// Decrement counter at 1/64 clock rate
	// or 250,000 decrements per second.
	TCCR1B |= ((1 << CS10) | (1 << CS11));

	sei();
}

void configure_watchdog_interrupt(void) {
	_WD_CONTROL_REG = _BV(WDIE);
}

void configure_usb(void) {
	USB_Init();
}

uint16_t led_brightness_correct(uint16_t value) {
	if (value == 0)
		return 0;
	return (uint16_t) pow(pow(255, (float) value / 255.0), 1.4);
}

void set_led_values(uint16_t led_r, uint16_t led_g, uint16_t led_b) {
	// Brightness is corrected using an exponential curve, as the human
	// eye does not distinguish as much at higher brightness levels.
	// In time, this may need to be switched to use a lookup table for
	// performance reasons.
	LedRedValue = led_brightness_correct(led_r);
	LedGreenValue = led_brightness_correct(led_g);
	LedBlueValue = led_brightness_correct(led_b);
}

void led_r_flash(uint8_t count) {
	for (uint8_t i = 0; i < count; i++) {
		LED_R_ON();
		_delay_ms(100);
		LED_R_OFF();
		_delay_ms(100);
	}
}

void led_g_flash(uint8_t count) {
	for (uint8_t i = 0; i < count; i++) {
		LED_G_ON();
		_delay_ms(100);
		LED_G_OFF();
		_delay_ms(100);
	}
}

void led_b_flash(uint8_t count) {
	for (uint8_t i = 0; i < count; i++) {
		LED_B_ON();
		_delay_ms(100);
		LED_B_OFF();
		_delay_ms(100);
	}
}

void led_sweep(uint8_t count) {
	for (uint8_t i = 0; i < count; i++) {
		led_r_flash(1);
		led_g_flash(1);
		led_b_flash(1);
	}
}

void indicate_post(void) {
	LED_R_OFF();
	LED_G_OFF();
	LED_B_OFF();
	_delay_ms(250);
	led_sweep(1);
}

void indicate_bootloader(void) {
	led_g_flash(2);
}

void indicate_initialized(void) {
	LED_R_ON();
	LED_G_ON();
	LED_B_ON();
	_delay_ms(250);
	LED_R_OFF();
	LED_G_OFF();
	LED_B_OFF();
}

void indicate_start(void) {
	LED_R_ON();
	LED_G_ON();
	_delay_ms(250);
	LED_R_OFF();
	LED_G_OFF();
	LED_B_ON();
	_delay_ms(250);
	LED_B_OFF();
}

// Boot section size in bytes
static inline uint16_t bootloader_size(void) {
	uint8_t fuse_bits = boot_lock_fuse_bits_get(GET_HIGH_FUSE_BITS);
	switch ((fuse_bits >> 1) & 3) {
		case 0: return 8192;
		case 1: return 4096;
		case 2: return 2048;
		case 3: return 1024;
	}
	return 8192;
}

#ifdef FAILSAFE_BOOTLOADER
void bootloader_activate(void) {
	eeprom_busy_wait();
	eeprom_update_byte(EEPROM_BOOT_SLOT, EEPROM_BOOT_START_BOOTLOADER);
	eeprom_busy_wait();
}

void bootloader_deactivate(void) {
	eeprom_busy_wait();
	eeprom_update_byte(EEPROM_BOOT_SLOT, EEPROM_BOOT_START_FIRMWARE);
	eeprom_busy_wait();
}

void bootloader_jump_conditional(void) {
	eeprom_busy_wait();
	uint8_t eeprom_data = eeprom_read_byte(EEPROM_BOOT_SLOT);
	if (eeprom_data == EEPROM_BOOT_START_BOOTLOADER)
		bootloader_jump();
}
#endif /* FAILSAFE_BOOTLOADER */

// Jump to the start of the bootloader section.
void bootloader_jump(void) {
	indicate_bootloader();

	reset_bootloader();

	((void (*)(void))( (uint16_t)(BOOTLOADER_START / 2) ))();
}

void led_pwm_demo(void) {
	for (uint16_t i = 0; i <= 255; i++) {
		wdt_reset();
		set_led_values(255 - i, i, 255);
		_delay_ms(15);
	}

	set_led_values(0, 0, 0);

	_delay_ms(500);

	set_led_values(255, 255, 255);

	_delay_ms(500);

	set_led_values(0, 0, 0);
}

void refresh_leds(void) {
	if ((TimerLedOverflows <= LedRedValue) && (LedRedValue != 0))
		LED_R_ON();
	else
		LED_R_OFF();

	if ((TimerLedOverflows <= LedGreenValue) && (LedGreenValue != 0))
		LED_G_ON();
	else
		LED_G_OFF();

	if ((TimerLedOverflows <= LedBlueValue) && (LedBlueValue != 0))
		LED_B_ON();
	else
		LED_B_OFF();

	AwaitingLedRefresh = false;
}

void await_led_refresh(void) {
	AwaitingLedRefresh = true;
	for (;;) {
		if (! AwaitingLedRefresh) {
			break;
		}
	}
}

void process_usb_endpoints(void) {
	// TODO: maybe move this to after the DEVICE_STATE_Configured check?
	//CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);

	CDC_Device_USBTask(&VirtualSerial_CDC_Interface);

	// TODO: only fire this when device is connected.
	USB_USBTask();

	if (USB_DeviceState != DEVICE_STATE_Configured)
		return;

	// TODO: possibly replace this with LUFA RingBuffer_t?
	uint8_t  DataStream[CDC_TXRX_EPSIZE * 8];
	uint8_t  ErrorCode;
	uint16_t BytesProcessed = 0;

	memset(DataStream, 0, sizeof(DataStream));

	Endpoint_SelectEndpoint(5);
	if (Endpoint_IsConfigured() && Endpoint_IsOUTReceived() && Endpoint_IsReadWriteAllowed()) {
		while ((ErrorCode = Endpoint_Read_Stream_LE(DataStream, sizeof(DataStream),
							    &BytesProcessed)) == ENDPOINT_RWSTREAM_IncompleteTransfer);
		//led_g_flash(1);
		// FIXME: seeing timeouts on every single Read_Stream
		//if (ErrorCode == ENDPOINT_RWSTREAM_Timeout)
		//	led_b_flash(1);
		Endpoint_ClearOUT();
	}

	if (BytesProcessed) {
		//sprintf(Response + strlen(Response), "{\"debug-length\":%d}", BytesProcessed);
		//sprintf(Response + strlen(Response), "{\"debug-echo\":[%s]}", buf);

		int status = 0;

		const char* end = (const char*) DataStream + strlen((const char*) DataStream);
		const char* cur = (const char*) DataStream;

		//sprintf(Response + strlen(Response), "{\"debug-parse\":\"cur: %x, end: %x\"}", cur, end);
		while (cur < end) {
			bool command_check = false;
			bool command_start = false;
			bool command_boot = false;
			char command_screen_string[12] = "";
			unsigned int command_screen_orientation = 0;
			unsigned int command_screen_display = 0;

			unsigned int command_led_r[18];
			unsigned int command_led_g[18];
			unsigned int command_led_b[18];
			unsigned int command_led_i[18];
			int command_led_count = 0;

			const struct json_attr_t json_command_led[] = {
				{"i",      t_uinteger,  .addr.uinteger = command_led_i},
				{"r",      t_uinteger,  .addr.uinteger = command_led_r},
				{"g",      t_uinteger,  .addr.uinteger = command_led_g},
				{"b",      t_uinteger,  .addr.uinteger = command_led_b},
				{"m",      t_uinteger},
				{NULL},
			};

			int command_flip_count = 0;

			const struct json_attr_t json_command_flip[] = {
				{"i",      t_uinteger},
				{"m",      t_uinteger},
				{NULL},
			};

			const struct json_attr_t json_commands[] = {
				{"start",	       t_boolean,  .addr.boolean = &command_start},
				{"check",	       t_boolean,  .addr.boolean = &command_check},
				{"boot",	       t_boolean,  .addr.boolean = &command_boot},
				{"hidmap",             t_array},
				{"joymap",             t_array},
				{"midimap",            t_array},
				{"led",                t_array,    .addr.array.element_type = t_object,
								   .addr.array.arr.objects.subtype = json_command_led,
								   .addr.array.maxlen = 18,
								   .addr.array.count = &command_led_count},
				{"flip",               t_array,
								   .addr.array.element_type = t_object,
								   .addr.array.arr.objects.subtype = json_command_flip,
								   .addr.array.maxlen = 18,
								   .addr.array.count = &command_flip_count},
				{"screen_string",      t_string,   .addr.string  = command_screen_string,
					                           .len = sizeof(command_screen_string)},
				{"screen_orientation", t_uinteger, .addr.uinteger = &command_screen_orientation},
				{"screen_display",     t_uinteger, .addr.uinteger = &command_screen_display},
				{NULL},
			};

			status = json_read_object(cur, json_commands, &cur);
			//sprintf(Response + strlen(Response), "{\"debug-parse\":\"cur: %x, end: %x\"}", cur, end);

			// TODO: scale down CPU frequency after some period of not receiving these?
			//if (command_check)
			//	strcat(Response, "{\"debug-check\":1}");

			if (strlen(command_screen_string))
				sprintf(Response + strlen(Response), "{\"debug-screen-string\":\"%s\"}", command_screen_string);

			for (int i = 0; i < command_led_count; i++) {
				//sprintf(Response + strlen(Response), "{\"debug-led\":\"%d = (%d,%d,%d)\"}",
				//		command_led_i[i], command_led_r[i], command_led_g[i], command_led_b[i]);
				if (command_led_i[i] == 1) {
					set_led_values(command_led_r[i], command_led_g[i], command_led_b[i]);
					//await_led_refresh();
					refresh_leds();
				}
			}

			if (command_boot) {
#ifdef FAILSAFE_BOOTLOADER
				bootloader_activate();
#endif /* FAILSAFE_BOOTLOADER */
				bootloader_jump();
			}

			if (command_start)
				strcat(Response, "{\"l\":{\"u\":\"00000\",\"i\":1,\"t\":0,\"c\":[null,null,null]}}");

			if (status != 0) {
				strcat(Response, "{\"debug-error\":\"");
				strcat(Response, json_error_string(status));
				strcat(Response, "\"}");
				//sprintf(Response + strlen(Response), "{\"debug-echo\":[%s]}", (const char*) DataStream);
				break;
			}
		}
	}

	if (! strlen(Response))
		return;

	Endpoint_SelectEndpoint(4);
	if (Endpoint_IsConfigured() && Endpoint_IsINReady() && Endpoint_IsReadWriteAllowed()) {
		//led_r_flash(1);
		BytesProcessed = 0;
		while ((ErrorCode = Endpoint_Write_Stream_LE(Response, strlen(Response),
						&BytesProcessed)) == ENDPOINT_RWSTREAM_IncompleteTransfer);
		Endpoint_ClearIN();
		if (ErrorCode == ENDPOINT_RWSTREAM_NoError)
			memset(Response, 0, sizeof(Response));
	}
}

int main(void) {
	Response[0] = '\0';

	reset_watchdog();

	reset_timers();

	reset_adc();

	reset_usb();

#ifdef FAILSAFE_BOOTLOADER
	bootloader_jump_conditional();

	bootloader_activate();
#endif /* FAILSAFE_BOOTLOADER */

	configure_ports();

	configure_cpu();

	indicate_post();

	_delay_ms(500);

	configure_usb();

	indicate_initialized();

	configure_timers();

	wdt_enable(WDTO_4S);

	//configure_watchdog_interrupt();

	/*
	_delay_ms(1000);

	led_pwm_demo();

	_delay_ms(1000);

	reset_timers();
	*/

	wdt_reset();

	indicate_start();

	wdt_reset();

	sei();

	for (;;) {
		wdt_reset();
		process_usb_endpoints();
	}
}

void EVENT_USB_Device_Connect(void) {
	led_g_flash(1);
}

void EVENT_USB_Device_Disconnect(void) {
	led_r_flash(1);
}

void EVENT_USB_Device_ConfigurationChanged(void) {
	CDC_Device_ConfigureEndpoints(&VirtualSerial_CDC_Interface);
}

void EVENT_USB_Device_ControlRequest(void) {
	CDC_Device_ProcessControlRequest(&VirtualSerial_CDC_Interface);
}

/*
ISR(WDT_vect, ISR_NAKED){
	_WD_CONTROL_REG = _BV(WDIE);
	reti();
}
*/

ISR(TIMER1_OVF_vect) {
	// This interrupt fires roughly 27,777 times per second when
	// CPU is prescaled to 16Mhz.

	// LED brightness as it appears to the human eye does not change
	// linearly with duty cycle. LedValue* values are adjusted
	// by the set_led_values() function.

	// We compare the current RGB value of each LED to the overflow
	// counter. If the RGB value is higher, we keep the LED on. When
	// RGB is lower than the overflow counter, we turn the LED off.

	refresh_leds();

	if (TimerLedOverflows == 255)
		TimerLedOverflows = 0;
	else
		TimerLedOverflows++;

	initialize_timer1_counter();
}

