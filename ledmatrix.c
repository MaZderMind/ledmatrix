#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdint.h>
#include <string.h>

#include "ledmatrix.h"
#include "bits.h"
#include "data.h"

volatile uint8_t framebuffer[8];
uint8_t isr_x = 0, isr_y = 0;

void c_sleep(uint16_t t) {
	for(uint16_t n = 0; n < t; n++)
		_delay_ms(1);
}

void c_on() {
	cli();
	memset((void*)framebuffer, 0xFF, sizeof(framebuffer));
	sei();
}

void c_off() {
	cli();
	memset((void*)framebuffer, 0x00, sizeof(framebuffer));
	sei();
}

void c_frame(uint16_t ptr) {
	cli();
	for(uint8_t z = 0; z < 7; z++)
		framebuffer[z] = pgm_read_byte(ptr+z);
	sei();
}

void c_t2b_scroll(uint16_t ptr, uint16_t len, uint16_t sleep) {
	for(uint8_t n = 0; n < len-6; n++) {
		cli();
		for(uint8_t z = 0; z < 7; z++)
			framebuffer[z] = pgm_read_byte(ptr+z+n);
		sei();

		c_sleep(sleep);
	}
}

int __attribute__((OS_main))
main(void)
{
	TCCR0 = (1<<CS00);
	TIMSK |= (1<<TOIE0);

	while(1) {
		for(uint8_t n = 0; n < ncmd; n++) {
			uint16_t cmd = pgm_read_word(&cmds[n]);

			switch(cmd) {
				case C_SLEEP:
					c_sleep(
						pgm_read_word(&cmds[++n])
					);
					break;

				case C_ON:
					c_on();
					break;

				case C_OFF:
					c_off();
					break;

				case C_FRAME:
					c_frame(
						pgm_read_word(&cmds[++n])
					);
					break;

				case C_T2B_SCROLL:
					c_t2b_scroll(
						// mind the argument evaluation order
						pgm_read_word(&cmds[n+1]),
						pgm_read_word(&cmds[n+2]),
						pgm_read_word(&cmds[n+3])
					);
					n += 3;
					break;
			}
		}
	}
}

ISR(TIMER0_OVF_vect)
{
	if(BITCLEAR(framebuffer[isr_y], isr_x))
	{
		DDR = 0;
	}
	else if(isr_x == isr_y) {
		DDR = (1<<7) | (1<<isr_x);
		PORT = (1<<isr_x);
	}
	else {
		DDR = (1<<isr_x) | (1<<isr_y);
		PORT = (1<<isr_x);
	}

	if(++isr_x == 7) {
		isr_x = 0;
		if(++isr_y == 7) {
			isr_y = 0;
		}
	}
}
