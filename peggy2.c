 /*
    Title:              peggy2.c 
    Date Created:   3/11/08
    Last Modified:  3/11/08
    Target:             Atmel ATmega168 
    Environment:        AVR-GCC
    Purpose: Drive 25x25 LED array uniformly



    Written by Windell Oskay, http://www.evilmadscientist.com/

    Copyright 2008 Windell H. Oskay
    Distributed under the terms of the GNU General Public License, please see below.

    Additional license terms may be available; please contact us for more information.



    More information about this project is at 
    http://www.evilmadscientist.com/article.php/peggy



    -------------------------------------------------
    USAGE: How to compile and install



    A makefile is provided to compile and install this program using AVR-GCC and avrdude.

    To use it, follow these steps:
    1. Update the header of the makefile as needed to reflect the type of AVR programmer that you use.
    2. Open a terminal window and move into the directory with this file and the makefile.  
    3. At the terminal enter
    make clean   <return>
    make all     <return>
    make program <return>
    4. Make sure that avrdude does not report any errors.  If all goes well, the last few lines output by avrdude
    should look something like this:

    avrdude: verifying ...
    avrdude: XXXX bytes of flash verified

    avrdude: safemode: lfuse reads as E2
    avrdude: safemode: hfuse reads as D9
    avrdude: safemode: efuse reads as FF
    avrdude: safemode: Fuses OK

    avrdude done.  Thank you.


    If you a different programming environment, make sure that you copy over 
    the fuse settings from the makefile.


    -------------------------------------------------

    This code should be relatively straightforward, so not much documentation is provided.  If you'd like to ask 
    questions, suggest improvements, or report success, please use the evilmadscientist forum:
    http://www.evilmadscientist.com/forum/

    -------------------------------------------------


    Revision hitory:

    3/11/2008  Initial version
    Derived (partially) from peggy.c

    -------------------------------------------------


    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA


  */



#include <avr/io.h>
#include <avr/interrupt.h>


void SPI_TX(char cData)
{
	// Start Transmission
	SPDR = cData;
	// Wait for transmission complete:
	while (!(SPSR & _BV(SPIF)));

}

volatile unsigned int mode;

SIGNAL(SIG_PIN_CHANGE0)
{				// Sleep with the "OFF" Button

	mode++;

	// Note: Each button press+release will generate TWO interrupts

	// So, to have the thing turn off after n cycles, you need to
	// wait for mode == 2n.

	if (mode == 4) {
		PORTD = 0;

		SPI_TX(0);
		SPI_TX(0);
		SPI_TX(0);
		SPI_TX(0);

		PORTB |= _BV(1);	//Latch Pulse
		PORTB &= ~(_BV(1));

		SMCR = 5U;	// Select power-down mode
		asm("sleep");	//Go to sleep!
	}
}

void delayLong()
{
	unsigned int delayvar;
	delayvar = 0;
	while (delayvar <= 10) {
		asm("nop");
		delayvar++;
	}
}

int main(void)
{
	asm("cli");		// DISABLE global interrupts

	unsigned int j, k;	// General purpose indexing dummy variables

	unsigned long int p0[25];
	unsigned long int p1[25];

	mode = 0;

	PORTD = 0U;
	DDRD = 255U;


	// General Hardware Initialization:

	// MCUCR |= (1 << 4); // Disable pull-ups

	PORTB = 1;
	PORTC = 0;

	DDRB = 254U;
	DDRC = 255U;

	////SET MOSI, SCK Output, all other SPI as input:
	DDRB = (1 << 3) | (1 << 5) | (1 << 2) | (1 << 1);

	// ENABLE SPI, MASTER, CLOCK RATE fck/4:
	SPCR = (1 << SPE) | (1 << MSTR);

	SPI_TX(0);
	SPI_TX(0);
	SPI_TX(0);
	SPI_TX(0);

	PORTB |= _BV(1);	// Latch Pulse
	PORTB &= ~(_BV(1));

	j = 0;

	while (j < 25) {
		// Set up initial mode-- all LEDs on.
		p0[j] = 0x0f0f0f0f;
		p1[j] = 0x00ff00ff;

		j++;
	}

	PCICR = 1;		// Enable pin change interrupt on PCINT 0-7
	PCMSK0 = 1;		// Look only for change on pin B0, PCINT0.
	asm("sei");		// ENABLE global interrupts

	// main loop
	for (;;) {
	// paint a frame
		// PORTD = 255;
		int field = 0;

		while (field < 4) {
			// paint a field
			j = 0;

			while (j < 25) {
				// a, b, c, d are contents of the led driver registers.
				unsigned char a, b, c, d;

				// paint a row

				/* for grayscale, run fields 0, 1, 2 with planes 0, 1, 1 */
				switch (field) {
				case 0:
					// field 0 = plane 0
					a = (p0[j] >> 24) & 0xff;
					b = (p0[j] >> 16) & 0xff;
					c = (p0[j] >>  8) & 0xff;
					d = (p0[j] >>  0) & 0xff;
					break;

				case 1:
				case 2:
				case 3:
					// fields 1, 2 = plane 1
					a = (p1[j] >> 24) & 0xff;
					b = (p1[j] >> 16) & 0xff;
					c = (p1[j] >>  8) & 0xff;
					d = (p1[j] >>  0) & 0xff;
					break;
				}

				SPI_TX(a);
				SPI_TX(b);
				SPI_TX(c);
				SPI_TX(d);

				PORTD = 0;	// Turn displays off

				PORTB |= _BV(1);	// Latch Pulse
				PORTB &= ~(_BV(1));

				// paint one row
				if (j < 15)
					PORTD = j + 1;
				else
					PORTD = (j - 14) << 4;

				j++;

				delayLong();	// Delay with new data
			}

			field++;
		}
	}
	// End main loop.
	return 0;
}
