#define F_CPU 32000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "serialF0.h"
#include "clock.h"
#include "MQ135.h"
#include "nrf24spiXM2.h"
#include "nrf24L01.h"

void init_nrf(void);
void init_adc(void);
void init_servo(void)
void motor_up(void);
void motor_down(void);
void motor_off(void);
uint16_t read_lichtsensor(void);
uint16_t read_luchtsensor(void);

uint16_t servo = 499;

uint8_t  pipe1[5] = "LAMP";
uint8_t  pipe2[5] = "RAAME";
uint8_t  packet[32];
uint8_t  tgl = 0;
volatile uint8_t  Atgl = 0;
volatile uint8_t flag = 0;

int main(void)
{
	init_clock();
	init_stream(F_CPU);
	init_nrf();
	init_adc();
	init_servo();

	uint8_t position = 'u';											//variable for curtain position
		
	PORTD.DIRSET = PIN1_bm|PIN2_bm;									//output pins for DC-motor
	
	PORTD.DIRCLR = PIN4_bm|PIN6_bm;									//input pin for buttons
	PORTD.PIN4CTRL = PORT_OPC_PULLDOWN_gc;							//internal pullup resistor
	PORTD.PIN6CTRL = PORT_OPC_PULLDOWN_gc;

	PORTE.OUTCLR   = PIN3_bm|PIN0_bm;								//input pin for buttons
	PORTE.PIN0CTRL = PORT_OPC_PULLDOWN_gc;							//internal pullup resistor
	PORTE.PIN3CTRL = PORT_OPC_PULLDOWN_gc;
	
	PORTC.DIRSET = PIN0_bm;											//output pins for indication LED
	
	TCC0.CTRLB     = TC_WGMODE_NORMAL_gc;
	TCC0.CTRLA     = TC_CLKSEL_DIV1024_gc;
	TCC0.PER       = 31249;              //  t = 1024*31250*(1/32) = 1s
	TCC0.INTCTRLA  = TC_OVFINTLVL_LO_gc;
	
	sei();
	
	while (1)
	{
		//printf("Licht: %d\tLuchtvochtigheid: %d\tCO2: %f ppm\n",read_lichtsensor(), read_luchtsensor(),MQ135_getPPM());
		
		while(Atgl == 1){
			motor_up();
			_delay_ms(2000);
			Atgl = 0;
		}
		
		if(flag == 30){
			flag = 0;
			uint16_t msg[3];
			msg[0] = 'r';
			msg[1] = read_luchtsensor();
			msg[2] = MQ135_getPPM();
			nrfStopListening();
			nrfOpenWritingPipe(pipe2);
			printf("Send: %c, %d, %d\n", msg[0], msg[1], msg[2]);
			nrfWrite((uint8_t *) & msg, sizeof(uint16_t) * 3);
			nrfStartListening();
		}
		if (read_lichtsensor() > 175)								
		{
			tgl = 0;
		}
		else
		{
			if(tgl == 0)
			{
				tgl = 1;
				nrfStopListening();
				nrfOpenWritingPipe(pipe1);
				uint8_t f = 'f';
				printf("Send: '%c' \n", f);
				nrfWrite( (uint8_t *) & f, 1);						// little endian: low byte is sent first
				nrfStartListening();
			}
		}
		if (PORTD.IN & PIN4_bm && position == 'd')					//if the button is pressed and the position of the curtain is down the motor can roll the curtain up.
		{
			while (PORTD.IN & PIN4_bm)								//the motor keeps turning until the button is released
			{
				motor_up();
			}
			position = 'u';											//the position of the curtain is up
		}
		
		if (PORTD.IN & PIN6_bm && position == 'u')					//if the button is pressed and the position of the curtain is up the motor can roll the curtain down
		{
			while (PORTD.IN & PIN6_bm)								//the motor keeps turning until the button is released
			{
				motor_down();
			}
			position = 'd';											//the position of the curtain is down
		}
		motor_off();												//if no button is pressed keep motor off
		
		//TODO use CO2 sensor
		if(bit_is_set(PORTE.IN, 0))									//if button is pressed go to the minimum value
		{
			servo = 101;
		}
		if(bit_is_set(PORTE.IN, 3))									// if button is pressed go to maximum value
		{
			servo = 499;
		}
		TCD0.CCA = servo;											//current value is normal value

 	}
}

void init_adc(void)
{
	PORTA.DIRCLR        = PIN1_bm;                                  // PIN A1 as input for ADC
	ADCA.CH0.MUXCTRL    = ADC_CH_MUXPOS_PIN1_gc;                    // Bind PIN A1 to channel 0
	ADCA.CH0.CTRL        = ADC_CH_INPUTMODE_SINGLEENDED_gc;         // Configure channel 0 as single-ended
	ADCA.REFCTRL        = ADC_REFSEL_INTVCC_gc;                     // Use internal VCC/1.6 reference
	ADCA.CTRLB            = ADC_RESOLUTION_12BIT_gc;                // 12 bits conversion, unsigned, no freerun
	ADCA.PRESCALER        = ADC_PRESCALER_DIV256_gc;                // 32000000/256 = 125kHz
	ADCA.CTRLA            = ADC_ENABLE_bm;                          // Enable ADC

	PORTB.DIRCLR        = PIN1_bm;                                  // PIN B1 as input for ADC
	ADCB.CH0.MUXCTRL    = ADC_CH_MUXPOS_PIN1_gc;                    // Bind PIN B1 to channel 0
	ADCB.CH0.CTRL        = ADC_CH_INPUTMODE_SINGLEENDED_gc;         // Configure channel 0 as single-ended
	ADCB.REFCTRL        = ADC_REFSEL_INTVCC_gc;                     // Use internal VCC/1.6 reference
	ADCB.CTRLB            = ADC_RESOLUTION_12BIT_gc;                // 12 bits conversion, unsigned, no freerun
	ADCB.PRESCALER        = ADC_PRESCALER_DIV256_gc;                // 32000000/256 = 125kHz
	ADCB.CTRLA            = ADC_ENABLE_bm;							// Enable ADC
	
	PORTB.DIRCLR        = PIN5_bm;                                  // PIN B5 as input for ADC
	ADCB.CH1.MUXCTRL    = ADC_CH_MUXPOS_PIN5_gc;                    // Bind PIN B5 to channel 1
	ADCB.CH1.CTRL        = ADC_CH_INPUTMODE_SINGLEENDED_gc;         // Configure channel 1 as single-ended
	ADCB.REFCTRL        = ADC_REFSEL_INTVCC_gc;                     // Use internal VCC/1.6 reference
	ADCB.CTRLB            = ADC_RESOLUTION_8BIT_gc;                 // 8 bits conversion, unsigned, no freerun
	ADCB.PRESCALER        = ADC_PRESCALER_DIV256_gc;                // 32000000/256 = 125kHz
	ADCB.CTRLA            = ADC_ENABLE_bm;							// Enable ADC
}

void init_nrf(void)
{

	nrfspiInit();													// Initialize SPI
	nrfBegin();														// Initialize radio module

	nrfSetRetries(NRF_SETUP_ARD_1000US_gc,							// Auto Retransmission Delay: 1000 us
	NRF_SETUP_ARC_8RETRANSMIT_gc);									// Auto Retransmission Count: 8 retries
	nrfSetPALevel(NRF_RF_SETUP_PWR_6DBM_gc);						// Power Control: -6 dBm
	nrfSetDataRate(NRF_RF_SETUP_RF_DR_250K_gc);						// Data Rate: 250 Kbps
	nrfSetCRCLength(NRF_CONFIG_CRC_16_gc);							// CRC Check
	nrfSetChannel(32);												// Channel: 32
	nrfSetAutoAck(1);												//	Auto Acknowledge on
	nrfEnableDynamicPayloads();										// Enable Dynamic Payloads

	nrfClearInterruptBits();										// Clear interrupt bits
	nrfFlushRx();													// Flush fifo's
	nrfFlushTx();
	
	PORTF.INT0MASK |= PIN6_bm;
	PORTF.PIN6CTRL  = PORT_ISC_FALLING_gc;
	PORTF.INTCTRL   = (PORTF.INTCTRL & ~PORT_INT0LVL_gm) | PORT_INT0LVL_LO_gc;
	// Pipe for sending
	nrfOpenReadingPipe(0, pipe1);
	nrfOpenReadingPipe(1, pipe2);
	nrfStartListening();
}

uint16_t read_luchtsensor(void)
{
	uint16_t result;
	
	ADCA.CH0.CTRL |= ADC_CH_START_bm;								//start analog digital conversion
	while(!(ADCA.CH0.INTFLAGS));									//if not called do nothing
	result = ADCA.CH0.RES;											//save result
	ADCA.CH0.INTFLAGS |= ADC_CH_CHIF_bm;
	
	return result;
}

uint16_t read_lichtsensor(void)
{
	uint16_t result;
	
	ADCB.CH0.CTRL |= ADC_CH_START_bm;								//start analog digital conversion
	while(!(ADCB.CH0.INTFLAGS));									//if not called do nothing
	result = ADCB.CH0.RES;											//save result
	ADCB.CH0.INTFLAGS |= ADC_CH_CHIF_bm;
	
	return result;
}

void motor_up(void)
{
	PORTD.OUTCLR = PIN1_bm;
	PORTD.OUTSET = PIN2_bm;
}
void motor_down(void)	{
	PORTD.OUTCLR = PIN2_bm;
	PORTD.OUTSET = PIN1_bm;
}

void motor_off(void)
{
	PORTD.OUTCLR = PIN2_bm|PIN1_bm;
}

void init_servo(void)
{
	PORTD.DIRSET = PIN0_bm;
	PORTD.OUTSET = PIN0_bm;
	TCD0.CTRLB   = TC0_CCAEN_bm | TC_WGMODE_DSBOTH_gc;				//dual slope mode
	TCD0.CTRLA   = TC_CLKSEL_DIV64_gc;								//prescalling 64
	TCD0.PER     = 5000;
	TCD0.CCA     = servo;
}

ISR(TCC0_OVF_vect)
{
	flag++;
}

ISR(PORTF_INT0_vect)
{
	uint8_t  tx_ds, max_rt, rx_dr;

	nrfWhatHappened(&tx_ds, &max_rt, &rx_dr);
	if ( rx_dr )
	{
		nrfRead(packet, nrfGetDynamicPayloadSize());
		uint8_t res = packet[0];
		if(res == 'm'){
			Atgl = 1;
		}
	}
}