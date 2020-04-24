/*
 * Main
 *
 * Created: 25-2-2020 13:02:16
 * Author : Joris Bruinsma
 */ 
#define F_CPU 32000000UL
#include "header.h"

#define STEP 10
#define BOUND 2000
#define UPPER 300
#define BUFFER_SIZE 255
#define POT_SIZE 100

uint8_t  pipe[5] = "CLOCK";
uint8_t  pipe1[5] = "LAMP";

volatile uint8_t stateChange = 0;
int lamp  = 0;
int indexArray = 0;

int potArray[POT_SIZE];
int buffer[BUFFER_SIZE];

int main(void)
{
	init();
	uint8_t state = 0;
	while (1) 
	{
		set_state(state);
	}    
}

/*!Brief Set the state according to the inputs
*
* \return				void
*/
void set_state(uint8_t state)
{
	if(PORTB.IN & PIN2_bm)
	{
		state = 1;
	}
	else if(PORTB.IN & PIN3_bm)
	{
		state = 2;
	}
	else if(PORTB.IN & PIN4_bm)
	{
		state = 3;
	}
	else if(PORTB.IN & PIN5_bm)
	{
		state = 4;
	}
	else if(stateChange == 3){
		state = 3;
	}
	else if(stateChange == 1){
		state = 1;
	}
	run_state(state);
}

/*!Brief execute functions according to state
*
* \return				void
*/
void run_state(uint8_t state)
{
	switch(state)
	{
		case 1:
			lamp_with_sensor();
			break;
		case 2:
			lamp_with_pot();
			break;
		case 3:
			lamp_on();
			break;
		case 4:
			lamp_off();
			break;
		default:
			break;
	}
}

/*!Brief Turn the lamp on
*
* \return				void
*/
void lamp_on(void)
{
	TCD0.CCA = 9999;
}

/*!Brief turn the lamp off
*
* \return				void
*/
void lamp_off(void)
{
	TCD0.CCA = 0;
}

/*!Brief calculate the average of the last 10 read values of the potentiometer
*
* \return				void
*/
uint16_t average_pot(void)
{
	uint16_t value = read_pot();
	uint16_t sum = 0;
	
	if(indexArray >= POT_SIZE)
	{
		indexArray = 0;
	}
	
	potArray[indexArray] = value;
	indexArray++;
	
	for(int x = 0; x < POT_SIZE; x++)
	{
		sum += potArray[x];
	}
	
	return sum/POT_SIZE;
}

/*!Brief Use the averagePots value to control the brightens level of the Lamp
*
* \return				void
*/
void lamp_with_pot(void)
{
	uint16_t value = average_pot();
	TCD0.CCA = map((uint32_t)value, (uint32_t)0, (uint32_t)4095, (uint32_t)0 , (uint32_t)9999);
}

/*!Brief Use the light sensors value to keep the brightens level of the Lamp the same
*
* \return				void
*/
void lamp_with_sensor()
{
	uint16_t value = read_sensor();
	if(value < BOUND - UPPER){
		lamp += STEP;
		if(lamp > 9999)
		{
			lamp = 9999;
		}
		TCD0.CCA = lamp;
	}
	else if(value > BOUND + UPPER)
	{
		lamp -= STEP;
		if(lamp < 0)
		{
			lamp = 0;
		}
		TCD0.CCA = lamp;
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

/*!Brief reads the value of the light sensor connected to pin
*
* \return				result of light sensor
*/
uint16_t read_sensor(void)
{
	uint16_t res;

	ADCA.CH0.CTRL |= ADC_CH_START_bm;								//Start ADC conversion
	while(!(ADCA.CH0.INTFLAGS));									//Wait for conversion to finish
	res = ADCA.CH0.RES;												//Store result in res
	ADCA.CH0.INTFLAGS |= ADC_CH_CHIF_bm;							//Reset interrupt register
	
	return res;
}

/*!Brief reads the value of the potentiometer connected to pin
*
* \return				result of potentiometer
*/
uint16_t read_pot(void)
{
	uint16_t res;
	
	ADCB.CH0.CTRL |= ADC_CH_START_bm;								//Start ADC conversion
	while(!(ADCB.CH0.INTFLAGS));									//Wait for conversion to finish
	res = ADCB.CH0.RES;												//Store result in res
	ADCB.CH0.INTFLAGS |= ADC_CH_CHIF_bm;							//Reset interrupt register
	
	return res;
}

/*!Brief initializes the IO-Pins, the ADC and the PWM signals
*
* \return				void
*/
void init(void)
{
	PORTB.DIRCLR   = PIN2_bm|PIN3_bm|PIN4_bm|PIN5_bm;				// Configure PIN B2, B3, B4 and B5 as input pins
	PORTB.PIN5CTRL = PORT_OPC_PULLDOWN_gc;							// Use the internal pulldown resistor for PIN B5
	PORTB.PIN4CTRL = PORT_OPC_PULLDOWN_gc;							// Use the internal pulldown resistor for PIN B4
	PORTB.PIN3CTRL = PORT_OPC_PULLDOWN_gc;							// Use the internal pulldown resistor for PIN B3
	PORTB.PIN2CTRL = PORT_OPC_PULLDOWN_gc;							// Use the internal pulldown resistor for PIN B2
	init_nrf();
	init_adc();
	init_pwm();
	PMIC.CTRL |= PMIC_LOLVLEN_bm;
	sei();
}


/*!Brief initializes the ADC
*
* \return				void
*/
void init_adc(void)
{
	PORTA.DIRCLR		= PIN1_bm;									// PIN A1 as input for ADC
	ADCA.CH0.MUXCTRL	= ADC_CH_MUXPOS_PIN1_gc;					// Bind PIN A1 to channel 0
	ADCA.CH0.CTRL		= ADC_CH_INPUTMODE_SINGLEENDED_gc;			// Configure channel 0 as single-ended
	ADCA.REFCTRL		= ADC_REFSEL_INTVCC_gc;						// Use internal VCC/1.6 reference
	ADCA.CTRLB			= ADC_RESOLUTION_12BIT_gc;					// 12 bits conversion, unsigned, no freerun
	ADCA.PRESCALER		= ADC_PRESCALER_DIV256_gc;					// 32000000/256 = 125kHz
	ADCA.CTRLA			= ADC_ENABLE_bm;							// Enable ADC

	PORTB.DIRCLR		= PIN1_bm;									// PIN B1 as input for ADC
	ADCB.CH0.MUXCTRL	= ADC_CH_MUXPOS_PIN1_gc;					// Bind PIN B1 to channel 0
	ADCB.CH0.CTRL		= ADC_CH_INPUTMODE_SINGLEENDED_gc;			// Configure channel 0 as single-ended
	ADCB.REFCTRL		= ADC_REFSEL_INTVCC_gc;						// Use internal VCC/1.6 reference
	ADCB.CTRLB			= ADC_RESOLUTION_12BIT_gc;					// 12 bits conversion, unsigned, no freerun
	ADCB.PRESCALER		= ADC_PRESCALER_DIV256_gc;					// 32000000/256 = 125kHz
	ADCB.CTRLA			= ADC_ENABLE_bm;							// Enable ADC
}

/*!Brief initializes a PWM signal
*
* \return				void
*/
void init_pwm(void)
{
	PORTD.DIRSET = PIN0_bm;											// PIN D0 as output
	
	TCD0.CTRLB   = TC0_CCAEN_bm | TC_WGMODE_SS_gc;					// Enable CCA in singleslope mode
	TCD0.CTRLA   = TC_CLKSEL_DIV4_gc;
	TCD0.PER     = 9999;											// Top value
	TCD0.CCA     = 0;												// Duty cycle 0%
}


/*!Brief initializes the NRF module
*
* \return				void
*/
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
	nrfSetAutoAck(1);												// Auto Acknowledge on
	nrfEnableDynamicPayloads();										// Enable Dynamic Payloads

	nrfClearInterruptBits();										// Clear interrupt bits
	nrfFlushRx();													// Flush fifo's
	nrfFlushTx();
	
	PORTF.INT0MASK |= PIN6_bm;
	PORTF.PIN6CTRL  = PORT_ISC_FALLING_gc;
	PORTF.INTCTRL   = (PORTF.INTCTRL & ~PORT_INT0LVL_gm) | PORT_INT0LVL_LO_gc;
	
	// Pipe for sending
	nrfOpenReadingPipe(0, pipe);
	nrfOpenReadingPipe(1, pipe1);
	nrfStartListening();
}


/*!Brief Interrupt that triggers when a messages is received
*/
ISR(PORTF_INT0_vect)
{
	uint8_t tx_ds, max_rt, rx_dr;
	
	nrfWhatHappened(&tx_ds, &max_rt, &rx_dr);						//Check what happened
	
	if(rx_dr)														//messages received correctly
	{																
		nrfRead(buffer, nrfGetDynamicPayloadSize());
		uint8_t res = buffer[0];									//store first byte				
		if(res == 'c')												//Store is 'c'
		{														
			stateChange = 3;										//Lamp on
		}
		if(res == 'f'){
			stateChange = 1;										
		}
	}
}