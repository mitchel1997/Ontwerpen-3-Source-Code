/*!
 *  \file    ucglib_xmega_hal.h
 *  \author  Wim Dolman (<a href="mailto:w.e.dolman@hva.nl">w.e.dolman@hva.nl</a>)
 *  \date    29-08-2019
 *  \version 2.0
 *
 *  \brief   Xmega Hardware Abstraction Layer for ucglib from Oli Kraus
 *
 *  \details This Hardware Abstraction Layer is created confirm the
 *           <a href"https://github.com/olikraus/ucglib/wiki/hal">instructions</a> of Oli Kraus.
 *
 *           This file contains a callback function <code>ucg_com_xmega()</code> that handles the
 *           communication with the Xmega using SPI or bit banging.
 *           The Arduino/C++ implementation of ucglib contains extra printing facilities.
 *           This file contains a bunch of functions that implements the same facilities.
 *           So you can use <code>ucg_SetPrintPos()</code>, <code>ucg_SetPrintDir()</code> 
 *           and in stead of <code>print</code> and <code>println</code> 
 *           you can use <code>ucg_Print()</code> which prints a formatstring.
 *
 *           This print facility uses a hook in the struct ucg_t. Add to the struct these
 *           lines:
 * \verbatim
             #ifdef __AVR_XMEGA__
             void  *xmega_hook;   // added pointer for print hook
             #endif
   \endverbatim
 *           or uses the ucg.h of the HAL package in stead
 */
#ifndef _UCGLIB_HAL_XMEGA_H
#define _UCGLIB_HAL_XMEGA_H

#include "csrc/ucg.h"

#define UCG_XMEGA_USING_SPI       0    //!<  value defining SPI is used
#define UCG_XMEGA_USING_BB        1    //!<  value defining bit banging is used

#define UCG_XMEGA_BLK_CONNECT     0    //!<  value defining BLK/LED is connected to Xmega
#define UCG_XMEGA_BLK_EXTERN      1    //!<  value defining BLK/LED is not connected with Xmega
#define UCG_XMEGA_BLK_DISABLED    2    //!<  value defining BLK/LED is connected with Xmega, but pin is disabled 

// start user specific part
#define UCG_XMEGA_USE             UCG_XMEGA_USING_SPI    //!<  defining the use of SPI or bit banging 
#define UCG_XMEGA_BLK             UCG_XMEGA_BLK_CONNECT  //!<  defining the status of the BLK or LED connection

#define UCG_XMEGA_INTERFACE       SPID                  //!<  SPI interface used
#define UCG_XMEGA_SPI_PORT        PORTD                 //!<  port SPI interface using SPI
#define UCG_XMEGA_BB_PORT         PORTD                 //!<  port SPI interface using BB

#define UCG_XMEGA_SPI_SCK_PORT    UCG_XMEGA_SPI_PORT    //!<  port SCK connection using SPI
#define UCG_XMEGA_SPI_MOSI_PORT   UCG_XMEGA_SPI_PORT    //!<  port SDA or MOSI connection using SPI
#define UCG_XMEGA_SPI_MISO_PORT   UCG_XMEGA_SPI_PORT    //!<  port MISO connection using SPI (not connected)
#define UCG_XMEGA_SPI_SS_PORT     UCG_XMEGA_SPI_PORT    //!<  port CS or SS connection using SPI
#define UCG_XMEGA_SPI_RESET_PORT  UCG_XMEGA_SPI_PORT    //!<  port RESET connection using SPI
#define UCG_XMEGA_SPI_CD_PORT     UCG_XMEGA_SPI_PORT    //!<  port CD or DC or AO connection using SPI
#define UCG_XMEGA_SPI_BLK_PORT    UCG_XMEGA_SPI_PORT    //!<  port BLK or LED connection using SPI (optional)

#define UCG_XMEGA_BB_SCK_PORT     UCG_XMEGA_BB_PORT     //!<  port SCK connection using BB
#define UCG_XMEGA_BB_SDA_PORT     UCG_XMEGA_BB_PORT     //!<  port SDA or MOSI connection using BB
#define UCG_XMEGA_BB_CS_PORT      UCG_XMEGA_BB_PORT     //!<  port CS or SS connection using BB
#define UCG_XMEGA_BB_RESET_PORT   UCG_XMEGA_BB_PORT     //!<  port RESET connection using BB
#define UCG_XMEGA_BB_CD_PORT      UCG_XMEGA_BB_PORT     //!<  port CD or DC or AO connection using BB
#define UCG_XMEGA_BB_BLK_PORT     UCG_XMEGA_BB_PORT     //!<  port BLK or LED connection using BB (optional)

#define UCG_XMEGA_SPI_SCK_bp      PIN7_bp      //!<  pin position of SCK connection using SPI
#define UCG_XMEGA_SPI_MISO_bp     PIN6_bp      //!<  pin position of MISO connection using SPI
#define UCG_XMEGA_SPI_MOSI_bp     PIN5_bp      //!<  pin position of MOSI connection using SPI
#define UCG_XMEGA_SPI_SS_bp       PIN4_bp      //!<  pin position of SS connection using SPI
#define UCG_XMEGA_SPI_RESET_bp    PIN3_bp      //!<  pin position of RESET connection
#define UCG_XMEGA_SPI_CD_bp       PIN2_bp      //!<  pin position of CD or DC or AO connection
#define UCG_XMEGA_SPI_BLK_bp      PIN1_bp      //!<  pin position of BLK or LED connection (optional)
                                
#define UCG_XMEGA_BB_SCK_bp       PIN4_bp      //!<  pin position of SCK connection using BB
#define UCG_XMEGA_BB_SDA_bp       PIN3_bp      //!<  pin position of SDA (MOSI) connection using using BB
#define UCG_XMEGA_BB_CS_bp        PIN0_bp      //!<  pin position of CS or SS connection BB
#define UCG_XMEGA_BB_RESET_bp     PIN1_bp      //!<  pin position of RESET connection using BB
#define UCG_XMEGA_BB_CD_bp        PIN2_bp      //!<  pin position of CD or DC or AO connection using BB
#define UCG_XMEGA_BB_BLK_bp       PIN5_bp      //!<  pin position of BLK or LED connection using BB (optional)
// end user specific part

int16_t ucg_comm_xmega(ucg_t *ucg, int16_t msg, uint16_t arg, uint8_t *data);

void  ucg_PrintInit(ucg_t *ucg);
void  ucg_SetPrintPos(ucg_t *ucg, ucg_int_t x, ucg_int_t y);
void  ucg_SetPrintDir(ucg_t *ucg, uint8_t dir);
void  ucg_Print(ucg_t *ucg, char *fmt, ...);
void  ucg_GetPrintPos(ucg_t *ucg, ucg_int_t *x, ucg_int_t *y);
#endif