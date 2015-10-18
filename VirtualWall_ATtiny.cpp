/*
* VirtualWall_ATtiny.cpp
*
* Created: 2015-09-12 23:04:31
*  Author: herrfrost
*/

#ifndef F_CPU
#define F_CPU 1000000L
#endif

#define TURN_BACK 0b10100010 // 162
#define NOTIFY_LED_PIN PB4
#define IR_LED_PIN PB1

#include <avr/io.h>
#include <avr/interrupt.h>
#include <limits.h>
#include <util/delay.h>
#include <avr/sleep.h>

volatile bool wdi = true;
volatile uint8_t ledCounter = 0;

void PWMPinOutOn()
{
	DDRB |= (1 << IR_LED_PIN);
}

void PWMPinOutOff()
{
	DDRB &= ~(1 << IR_LED_PIN);
}

void SendOne()
{
	PWMPinOutOn();
	_delay_ms(3);
	PWMPinOutOff();
	_delay_ms(1);
}

void SendZero()
{
	PWMPinOutOn();
	_delay_ms(1);
	PWMPinOutOff();
	_delay_ms(3);
}


void sendByteAndLightLED(uint8_t code)
{
	
	if(ledCounter == 20)
	{
		DDRB |= (1 << NOTIFY_LED_PIN);
		PORTB |= (1 << NOTIFY_LED_PIN);
	}
	
	for (int8_t i = CHAR_BIT - 1; i >= 0; i--)
	{
		if((code >> i) & 1)
		{
			SendOne();
		}
		else
		{
			SendZero();
		}
	}
	
	if(ledCounter == 21)
	{
		PORTB &= ~(1 << NOTIFY_LED_PIN);
		DDRB &= ~(1 << NOTIFY_LED_PIN);
		
		ledCounter = 0;
	}
	ledCounter++;
}

void sleep()
{
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	sleep_enable();
	sleep_mode();
	sleep_disable();
}

int main(void)
{
	#ifndef NDEBUG
	if(MCUSR & (1<<WDRF))   // WDT caused reset
	{
		MCUSR &= ~(1 << WDRF);
		WDTCR |= (1 << WDCE) | (1 << WDE); // change enable
		WDTCR = 0x00; // disable WDT
		
		DDRB |= (1 << NOTIFY_LED_PIN);
		while(1)
		{
			// blink led
			PORTB |= (1 << NOTIFY_LED_PIN);
			_delay_ms(100);
			PORTB &= ~(1 << NOTIFY_LED_PIN);
			_delay_ms(100);
		}
	}
	#endif
	
	cli();

	ACSR |= (1 << ACD);		// 16.2.2 disable analog comparator
	ADCSRA &= ~(1 << ADEN); // 17.13.2 disable ADC

	DDRB |= (1 << IR_LED_PIN) | (1 << NOTIFY_LED_PIN); // set outputs
	
	// PWM
	// OC0x toggle on each Compare Match (COM0x[1:0]=1
	// FAST PWM mode 7: WGM0[2:0] = 7 for fixed modulation frequency (38 kHz)
	TCCR0A |= (1 << COM0B1) | (1 << WGM01) | (1 << WGM00);
	TCCR0B |= (1 << CS00) | (1 << WGM02);
	OCR0A = 26; // Count to F_CPU * 1/38kHz
	OCR0B = 13; // 50% duty cycle

	// WDT
	MCUSR &= ~(1 << WDRF);	// clear watchdog reset flag
	WDTCR |= (1 << WDE) | (1 << WDCE); // change enable
	WDTCR &= ~(1 << WDE);
	WDTCR |= (1 << WDIE) | (1 << WDP1) | (1 << WDP0); // 2^14 cycles @ 128 kHz ~ .128 s
	
	sei();
	
	while(1)
	{
		if(wdi)
		{
			wdi = false;
			sendByteAndLightLED(TURN_BACK);
			sleep();
		}
	}
}

ISR(WDT_vect)
{
	wdi = true;
}
