/*
 * header.h
 *
 * Created: 27-2-2020 14:46:25
 * Author: Joris
 */ 
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "nrf24spiXM2.h"
#include "nrf24L01.h"

// Prototypes
void init(void);
void init_pwm(void);
void dim_lamp(float percentage);
void init_adc(void);
void init_nrf(void);
uint16_t read_sensor(void);
void set_state(uint8_t state);
void run_state(uint8_t state);
void lamp_with_pot(void);
void lamp_with_sensor();
uint32_t map(uint32_t x, uint32_t in_min, uint32_t in_max, uint32_t out_min, uint32_t out_max); 
void lamp_on(void);
void lamp_off(void);
uint16_t average_pot(void);
uint16_t read_pot(void);
uint16_t read_sensor(void);



