/**************************************************************************/
/*!
@file     MQ135.cpp
@author   G.Krocker (Mad Frog Labs)
@license  GNU GPLv3
First version of an Arduino Library for the MQ135 gas sensor
TODO: Review the correction factor calculation. This currently relies on
the datasheet but the information there seems to be wrong.
@section  HISTORY
v1.0 - First release
*/
/*
Deze code is aangepast door Max van den Dolder
Op 17-4-2020
v2.0 aangepaste versie voor atXmega256A3U
*/
/**************************************************************************/
#include <avr/io.h>
#include <math.h>
#include "MQ135.h"

/**************************************************************************/
/*!
@brief  Default constructor

@param[in] pin  The analog input pin for the readout of the sensor
*/
/**************************************************************************/

/**************************************************************************/
/*!
@brief  Get the correction factor to correct for temperature and humidity

@param[in] t  The ambient air temperature
@param[in] h  The relative humidity

@return The calculated correction factor
*/
/**************************************************************************/
float MQ135_getCorrectionFactor(float t, float h)
{
  return CORA * t * t - CORB * t + CORC - (h-33.)*CORD;
}

/**************************************************************************/
/*!
@brief  Get the resistance of the sensor, ie. the measurement value

@return The sensor resistance in kOhm
*/
/**************************************************************************/
uint16_t read_co2sensor(void)
{
	uint16_t result;
	
	ADCB.CH1.CTRL |= ADC_CH_START_bm;		//start analog digital conversion
	while(!(ADCB.CH1.INTFLAGS));			//if not called do nothing
	result = ADCB.CH1.RES;					//save result
	ADCB.CH1.INTFLAGS |= ADC_CH_CHIF_bm;
	
	return result;
}

float MQ135_getResistance(void)
{
  int val = read_co2sensor();
  return ((1023./(float)val) * 5. - 1.)*RLOAD;
}

/**************************************************************************/
/*!
@brief  Get the resistance of the sensor, ie. the measurement value corrected
        for temp/hum

@param[in] t  The ambient air temperature
@param[in] h  The relative humidity

@return The corrected sensor resistance kOhm
*/
/**************************************************************************/
float MQ135_getCorrectedResistance(float t, float h)
{
  return MQ135_getResistance()/MQ135_getCorrectionFactor(t, h);
}

/**************************************************************************/
/*!
@brief  Get the ppm of CO2 sensed (assuming only CO2 in the air)

@return The ppm of CO2 in the air
*/
/**************************************************************************/
float MQ135_getPPM(void)
{
  return PARA * pow((MQ135_getResistance()/RZERO), -PARB);
}

/**************************************************************************/
/*!
@brief  Get the ppm of CO2 sensed (assuming only CO2 in the air), corrected
        for temp/hum

@param[in] t  The ambient air temperature
@param[in] h  The relative humidity

@return The ppm of CO2 in the air
*/
/**************************************************************************/
float MQ135_getCorrectedPPM(float t, float h)
{
  return PARA * pow((MQ135_getCorrectedResistance(t, h)/RZERO), -PARB);
}

/**************************************************************************/
/*!
@brief  Get the resistance RZero of the sensor for calibration purposes

@return The sensor resistance RZero in kOhm
*/
/**************************************************************************/
float MQ135_getRZero(void)
{
  return MQ135_getResistance() * pow((ATMOCO2/PARA), (1./PARB));
}

/**************************************************************************/
/*!
@brief  Get the corrected resistance RZero of the sensor for calibration
        purposes

@param[in] t  The ambient air temperature
@param[in] h  The relative humidity

@return The corrected sensor resistance RZero in kOhm
*/
/**************************************************************************/
float MQ135_getCorrectedRZero(float t, float h)
{
  return MQ135_getCorrectedResistance(t, h) * pow((ATMOCO2/PARA), (1./PARB));
}