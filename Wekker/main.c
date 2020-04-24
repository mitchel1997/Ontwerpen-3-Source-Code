#define  F_CPU 32000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <math.h>

#include "clock.h"
#include "csrc/ucg.h"
#include "ucglib_xmega_hal.h"
#include "serialF0.h"
#include "nrf24spiXM2.h"
#include "nrf24L01.h"

uint8_t  pipe[5] = "CLOCK";
uint8_t  pipe2[5] = "RAAM";
uint8_t  packet[32];
volatile uint8_t tgl = 0;

ucg_t	ucg;

int s = 0;
int m = 0;
int h = 0;

int am = 0;
int ah = 0;
int as = 10;

int px = 7;
int py = 48;

int f;

uint16_t co2 = 500;
uint16_t hum = 5;

uint32_t map(uint32_t x, uint32_t in_min, uint32_t in_max, uint32_t out_min, uint32_t out_max);
uint16_t convert8to16(uint8_t one, uint8_t two);

ISR(TCE0_OVF_vect)							// clock visualizing
{
	s++;
	if (s == 60) 
	{
		s= 0;
		m++;
	}
	if (m == 60) 
	{
		m = 0;
		h++;
	}
	if (h == 24) 
	{
		h = 0;
	}
}					

ISR(TCD0_OVF_vect)											// toggle speaker
{
	PORTE.OUTTGL = PIN7_bm;	
}

int main(void)
{
	PORTB.DIRSET	= PIN7_bm;
	PORTB.DIRCLR	= PIN2_bm;
	PORTC.DIRSET	= PIN0_bm;
	PORTE.DIRSET	= PIN7_bm;
	PORTA.PIN1CTRL = PORT_OPC_PULLDOWN_gc;
	PORTA.PIN2CTRL = PORT_OPC_PULLDOWN_gc;
	PORTA.PIN7CTRL = PORT_OPC_PULLDOWN_gc;
	PORTB.PIN0CTRL = PORT_OPC_PULLDOWN_gc;
	init_nrf();
	init_stream(F_CPU);
	init_clock();
	init_klokje();
	
	ucg_Init(&ucg, ucg_dev_st7735_18x128x160, ucg_ext_st7735_18,
		(int (*)(struct _ucg_t *, int,  unsigned int,  unsigned char *)) ucg_comm_xmega);
	prepare_screen();
	while (1)
	{
		if (bit_is_set (PORTA.IN, PIN1_bp ))							// If button is pressed allow alarm to be set
		{			 
			ucg_SetColor(&ucg, 0, 50, 50, 255);
			ucg_SetPrintPos(&ucg, px, py);
			ucg_Print(&ucg,	"%.2d : %.2d", ah, am);
			set_alarm();
		}
		else if (bit_is_set (PORTA.IN, PIN2_bp))						// If button is pressed allow time to be set
		{
			ucg_SetColor(&ucg, 0, 50, 255, 50);
			ucg_SetPrintPos(&ucg, px, py);	
			ucg_Print(&ucg,	"%.2d : %.2d", h, m);
			set_time();
		}
		else															// If no button is pressed, show time
		{
			ucg_SetColor(&ucg, 0, 255, 50, 50);
			ucg_SetPrintPos(&ucg, px, py);
			ucg_Print(&ucg,	"%.2d : %.2d", h, m);
 			if (tgl == 1)
 			{
				tgl = 0;
				print_info();											// Print info from other devices
			}
		}
		if (bit_is_set(PORTB.IN, PIN2_bp))								// If switch is on, alarm is activated
		{
			PORTB.OUTSET	= PIN7_bm;
			
			if (m == am && h == ah && s == as)
			{
				alarm();
				while (bit_is_set(PORTB.IN, PIN2_bp))					// If alarm and time are the same, make noise
				{
					deuntje(f);
				}
			}
		}
		if (bit_is_clear(PORTB.IN, PIN2_bp))							// If switch off, alarm off
		{
			PORTB.OUTCLR	= PIN7_bm;
			TCD0.INTCTRLA  = TC_OVFINTLVL_OFF_gc;
		}
 	}
}

void alarm()
{
	nrfStopListening();
	nrfOpenWritingPipe(pipe);
	uint8_t f = 'f';
	printf("Send: %c\n", f);
	nrfWrite((uint8_t *) & f, 1);
	nrfOpenWritingPipe(pipe2);
	uint8_t m = 'm';
	printf("Send: %c\n", m);
	nrfWrite((uint8_t *) & m, 1);
	nrfStartListening();
}

void init_klokje(void)
{
	TCE0.CTRLB     = TC_WGMODE_NORMAL_gc;		
	TCE0.CTRLA     = TC_CLKSEL_DIV1024_gc;		// Pre-scaling 1024
	TCE0.INTCTRLA  = TC_OVFINTLVL_LO_gc;		
	TCE0.PER       = 31249;						// t = 1024*31250/32000000 = 1,00 s
		
	PMIC.CTRL     |= PMIC_LOLVLEN_bm;			// set low level interrupts
	sei();										// activate interrupts
}

void geluid(int f)
{
	TCD0.CTRLB     = TC_WGMODE_NORMAL_gc;		
	TCD0.CTRLA     = TC_CLKSEL_DIV4_gc;			// Pre-scaling 10
	TCD0.INTCTRLA  = TC_OVFINTLVL_LO_gc;	
	TCD0.PER       = f;							// t = 1*f/32000000
	
	PMIC.CTRL     |= PMIC_LOLVLEN_bm;			// set low level interrupts
	sei();										// activate interrupts
}

void prepare_screen(void)
{
	ucg_ClearScreen(&ucg);
	ucg_SetRotate90(&ucg);
	ucg_SetColor(&ucg, 0, 255, 0, 0);
	ucg_SetFontMode(&ucg, UCG_FONT_MODE_SOLID);
	ucg_SetFont(&ucg, ucg_font_fur35_hf);
}

void print_info(void)
{
	ucg_SetColor(&ucg, 0, 255, 255, 10);
	ucg_SetPrintPos(&ucg, 10, 100);
	ucg_SetFont(&ucg, ucg_font_fur17_hf);
	ucg_Print(&ucg, "%.3d%%", hum);
				
	ucg_SetPrintPos(&ucg, 75, 100);
	ucg_Print(&ucg, "%.4d", co2);
	ucg_SetFont(&ucg, ucg_font_fur11_hf);
	ucg_Print(&ucg, "ppm");
	ucg_SetFont(&ucg, ucg_font_fur35_hf);
}

void set_time(void)														// Functie om tijd te zetten
{
	if (bit_is_set (PORTA.IN, PIN7_bp))
	{
		m++;	
 		s = 0;	
		if (m >= 60) m = 0;
		ucg_SetPrintPos(&ucg, px, py);
		ucg_Print(&ucg,	"%.2d : %.2d", h, m);
		_delay_ms(10);
	}
	if (bit_is_set (PORTB.IN, PIN0_bp))
	{
		h++;
 		s = 0;
		if (h >= 24) h = 0;
		ucg_SetPrintPos(&ucg, px, py);
		ucg_Print(&ucg,	"%.2d : %.2d", h, m);
		_delay_ms(10);
	}
}

void set_alarm(void)													// Functie om alarm te zetten
{
	if (bit_is_set ( PORTA.IN, PIN7_bp))
	{
		am++;
		as = 0;
		if (am >= 60) am = 0;
		ucg_SetPrintPos(&ucg, px, py);
		ucg_Print(&ucg,	"%.2d : %.2d", ah, am);
		_delay_ms(10);
	}
	if (bit_is_set (PORTB.IN, PIN0_bp))
	{
		ah++;
		as = 0;
		if (ah >= 24) ah = 0;
		ucg_SetPrintPos(&ucg, px, py);
		ucg_Print(&ucg,	"%.2d : %.2d", ah, am);
		_delay_ms(10);
	}
}

uint16_t convert8to16(uint8_t one, uint8_t two){
	uint16_t num = 0;
	uint8_t a = one;
	uint8_t b = two;
	num = a;
	num <<= 8;
	b<<=4;
	num |= b;
	num >>=4;
	return num;
}

void init_nrf(void)
{

	nrfspiInit();                                        // Initialize SPI
	nrfBegin();                                          // Initialize radio module

	nrfSetRetries(NRF_SETUP_ARD_1000US_gc,               // Auto Retransmission Delay: 1000 us
	NRF_SETUP_ARC_8RETRANSMIT_gc);         // Auto Retransmission Count: 8 retries
	nrfSetPALevel(NRF_RF_SETUP_PWR_6DBM_gc);             // Power Control: -6 dBm
	nrfSetDataRate(NRF_RF_SETUP_RF_DR_250K_gc);          // Data Rate: 250 Kbps
	nrfSetCRCLength(NRF_CONFIG_CRC_16_gc);               // CRC Check
	nrfSetChannel(32);                                   // Channel: 32
	nrfSetAutoAck(1);                                    // Auto Acknowledge on
	nrfEnableDynamicPayloads();                          // Enable Dynamic Payloads

	nrfClearInterruptBits();                             // Clear interrupt bits
	nrfFlushRx();                                        // Flush fifo's
	nrfFlushTx();
	
	PORTF.INT0MASK |= PIN6_bm;
	PORTF.PIN6CTRL  = PORT_ISC_FALLING_gc;
	PORTF.INTCTRL   = (PORTF.INTCTRL & ~PORT_INT0LVL_gm) | PORT_INT0LVL_LO_gc;
	// Pipe for sending
	nrfOpenReadingPipe(0, pipe);
	nrfOpenReadingPipe(1, pipe2);
	nrfStartListening();
}

ISR(PORTF_INT0_vect)
{
	uint8_t  tx_ds, max_rt, rx_dr;

	nrfWhatHappened(&tx_ds, &max_rt, &rx_dr);
	
	if ( rx_dr )
	{
		nrfRead(packet, 6);
		if(packet[0] == 'r')
		{
			uint8_t a, b, c, d;
			a = packet[2];
			b = packet[3];
			c = packet[4];
			d = packet[5];
			if(b==0)
			{
				hum = map(convert8to16(a, b), 0, 4095, 0, 100);
			}
			else
			{
				hum = a;
			}
			if(d==0){
				co2 = convert8to16(c, d);
			}
			else
			{
				co2 = c;
			}
		}
	}
	
}

/*! Brief Re-maps a number from one range to another
*
* \Param x				the number to map
* \Param in_min			the lower bound of the value’s current range
* \Param in_max			the upper bound of the value’s current range
* \Param out_min		the lower bound of the value’s target range
* \param out_max		the upper bound of the value’s target range
*
* \return				mapped value
*
* From: https://www.arduino.cc/reference/en/language/functions/math/map/
*/
uint32_t map(uint32_t x, uint32_t in_min, uint32_t in_max, uint32_t out_min, uint32_t out_max)
{
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void deuntje(int f)
{
	ucg_SetPrintPos(&ucg, px, py);
	ucg_Print(&ucg,	"%.2d : %.2d", h, m);
	
	f = 27303;
	geluid(f);
	_delay_ms(70);
	TCD0.INTCTRLA  = TC_OVFINTLVL_OFF_gc;
	_delay_ms(70);
	
	f = 27303;
	geluid(f);
	_delay_ms(61);
	TCD0.INTCTRLA  = TC_OVFINTLVL_OFF_gc;
	_delay_ms(61);	 
	
	f = 13628;
	geluid(f);
	_delay_ms(124);
	TCD0.INTCTRLA  = TC_OVFINTLVL_OFF_gc;
	_delay_ms(124);	
	
	f = 18181;
	geluid(f);
	_delay_ms(184);
	TCD0.INTCTRLA  = TC_OVFINTLVL_OFF_gc;
	_delay_ms(184);
	
	f = 19276;
	geluid(f);
	_delay_ms(128);
	TCD0.INTCTRLA  = TC_OVFINTLVL_OFF_gc;
	_delay_ms(128);
	
	f = 20407;
	geluid(f);
	_delay_ms(121);
	TCD0.INTCTRLA  = TC_OVFINTLVL_OFF_gc;
	_delay_ms(121);
	
	f = 22921;
	geluid(f);
	_delay_ms(125);
	TCD0.INTCTRLA  = TC_OVFINTLVL_OFF_gc;
	_delay_ms(125);
	
	f = 27303;
	geluid(f);
	_delay_ms(61);
	TCD0.INTCTRLA  = TC_OVFINTLVL_OFF_gc;
	_delay_ms(61);
	
	f = 22921;
	geluid(f);
	_delay_ms(61);
	TCD0.INTCTRLA  = TC_OVFINTLVL_OFF_gc;
	_delay_ms(61);
	
	f = 20407;
	geluid(f);
	_delay_ms(64);
	TCD0.INTCTRLA  = TC_OVFINTLVL_OFF_gc;
	_delay_ms(64);

	f = 30650;
	geluid(f);
	_delay_ms(70);
	TCD0.INTCTRLA  = TC_OVFINTLVL_OFF_gc;
	_delay_ms(70);
	
	f = 30650;
	geluid(f);
	_delay_ms(61);
	TCD0.INTCTRLA  = TC_OVFINTLVL_OFF_gc;
	_delay_ms(61);
	
	f = 13628;
	geluid(f);
	_delay_ms(124);
	TCD0.INTCTRLA  = TC_OVFINTLVL_OFF_gc;
	_delay_ms(124);
	
	f = 18181;
	geluid(f);
	_delay_ms(184);
	TCD0.INTCTRLA  = TC_OVFINTLVL_OFF_gc;
	_delay_ms(184);
	
	f = 19276;
	geluid(f);
	_delay_ms(128);
	TCD0.INTCTRLA  = TC_OVFINTLVL_OFF_gc;
	_delay_ms(128);
	
	f = 20407;
	geluid(f);
	_delay_ms(121);
	TCD0.INTCTRLA  = TC_OVFINTLVL_OFF_gc;
	_delay_ms(121);
	
	f = 22921;
	geluid(f);
	_delay_ms(125);
	TCD0.INTCTRLA  = TC_OVFINTLVL_OFF_gc;
	_delay_ms(125);
	
	f = 27303;
	geluid(f);
	_delay_ms(61);
	TCD0.INTCTRLA  = TC_OVFINTLVL_OFF_gc;
	_delay_ms(61);
	
	f = 22921;
	geluid(f);
	_delay_ms(61);
	TCD0.INTCTRLA  = TC_OVFINTLVL_OFF_gc;
	_delay_ms(61);
	
	f = 20407;
	geluid(f);
	_delay_ms(64);
	TCD0.INTCTRLA  = TC_OVFINTLVL_OFF_gc;
	_delay_ms(64);

	f = 32519;
	geluid(f);
	_delay_ms(70);
	TCD0.INTCTRLA  = TC_OVFINTLVL_OFF_gc;
	_delay_ms(70);
	
	f = 32519;
	geluid(f);
	_delay_ms(61);
	TCD0.INTCTRLA  = TC_OVFINTLVL_OFF_gc;
	_delay_ms(61);
	
	f = 13628;
	geluid(f);
	_delay_ms(124);
	TCD0.INTCTRLA  = TC_OVFINTLVL_OFF_gc;
	_delay_ms(124);
	
	f = 18181;
	geluid(f);
	_delay_ms(184);
	TCD0.INTCTRLA  = TC_OVFINTLVL_OFF_gc;
	_delay_ms(184);
	
	f = 19276;
	geluid(f);
	_delay_ms(128);
	TCD0.INTCTRLA  = TC_OVFINTLVL_OFF_gc;
	_delay_ms(128);
	
	f = 20407;
	geluid(f);
	_delay_ms(121);
	TCD0.INTCTRLA  = TC_OVFINTLVL_OFF_gc;
	_delay_ms(121);
	
	f = 22921;
	geluid(f);
	_delay_ms(125);
	TCD0.INTCTRLA  = TC_OVFINTLVL_OFF_gc;
	_delay_ms(125);
	
	f = 27303;
	geluid(f);
	_delay_ms(61);
	TCD0.INTCTRLA  = TC_OVFINTLVL_OFF_gc;
	_delay_ms(61);
	
	f = 22921;
	geluid(f);
	_delay_ms(61);
	TCD0.INTCTRLA  = TC_OVFINTLVL_OFF_gc;
	_delay_ms(61);	
	
	f = 20407;
	geluid(f);
	_delay_ms(64);
	TCD0.INTCTRLA  = TC_OVFINTLVL_OFF_gc;
	_delay_ms(64);
	
	f = 34333;
	geluid(f);
	_delay_ms(70);
	TCD0.INTCTRLA  = TC_OVFINTLVL_OFF_gc;
	_delay_ms(70);
	
	f = 34333;
	geluid(f);
	_delay_ms(61);
	TCD0.INTCTRLA  = TC_OVFINTLVL_OFF_gc;
	_delay_ms(61);
	
	f = 13628;
	geluid(f);
	_delay_ms(124);
	TCD0.INTCTRLA  = TC_OVFINTLVL_OFF_gc;
	_delay_ms(124);
	
	f = 18181;
	geluid(f);
	_delay_ms(184);
	TCD0.INTCTRLA  = TC_OVFINTLVL_OFF_gc;
	_delay_ms(184);
	
	f = 19276;
	geluid(f);
	_delay_ms(128);
	TCD0.INTCTRLA  = TC_OVFINTLVL_OFF_gc;
	_delay_ms(128);
	
	f = 20407;
	geluid(f);
	_delay_ms(121);
	TCD0.INTCTRLA  = TC_OVFINTLVL_OFF_gc;
	_delay_ms(121);
	
	f = 22921;
	geluid(f);
	_delay_ms(125);
	TCD0.INTCTRLA  = TC_OVFINTLVL_OFF_gc;
	_delay_ms(125);
	
	f = 27303;
	geluid(f);
	_delay_ms(61);
	TCD0.INTCTRLA  = TC_OVFINTLVL_OFF_gc;
	_delay_ms(61);
	
	f = 22921;
	geluid(f);
	_delay_ms(61);
	TCD0.INTCTRLA  = TC_OVFINTLVL_OFF_gc;
	_delay_ms(61);
	
	f = 20407;
	geluid(f);
	_delay_ms(64);
	TCD0.INTCTRLA  = TC_OVFINTLVL_OFF_gc;
	_delay_ms(64);
}