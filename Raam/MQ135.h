
/**************************************************************************/
/*!
@file     MQ135.h
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
#define MQ135_H

/// The load resistance on the board
#define RLOAD 10.0
/// Calibration resistance at atmospheric CO2 level
#define RZERO 76.63
/// Parameters for calculating ppm of CO2 from sensor resistance
#define PARA 116.6020682
#define PARB 2.769034857

/// Parameters to model temperature and humidity dependence
#define CORA 0.00035
#define CORB 0.02718
#define CORC 1.39538
#define CORD 0.0018

/// Atmospheric CO2 level for calibration purposes
#define ATMOCO2 397.13

uint16_t read_co2sensor(void);
float MQ135_getCorrectionFactor(float t, float h);
float MQ135_getResistance(void);
float MQ135_getCorrectedResistance(float t, float h);
float MQ135_getPPM(void);
float MQ135_getCorrectedPPM(float t, float h);
float MQ135_getRZero(void);
float MQ135_getCorrectedRZero(float t, float h);