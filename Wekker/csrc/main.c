/*!
 *  \file    main.c
 *  \author  Wim Dolman (<a href="mailto:w.e.dolman@hva.nl">w.e.dolman@hva.nl</a>)
 *  \date    29-08-2019
 *  \version 2.0
 *
 *  \brief   Test file for Xmega Hardware Abstraction Layer for ucglib from Oli Kraus
 *           for graphical color displays
 * 
 *  \details Basis example how to use ucglib with HAL for Xmega from Wim Dolman
 *           See for more info: 
 *           <a href="https://dolman-wim.nl/xmega/libraries/index.php">https://dolman-wim.nl/xmega/libraries/index.php</a> 
 *
 *           This examples uses 1.8 inch TFT LCD Module and must be connected this way:
 * \verbatim
 *             Display           Xmega
 *             VCC               3V3
 *             GND               GND
 *             CS                D4  (SS)
 *             RESET             D3
 *             A0   (DC or CD)   D2
 *             SDA               D5  (MOSI)
 *             SCK               D7  (SCK)
 *             LED  (BLK)        D1
 *                               D6  (MISO)  not connected
 * \endverbatim
 *            
 *  \note    Usage with Atmel Studio:
 *              - Add main.c, ucglib_hal_xmega.c and ucglib_hal_xmega.h to an Atmel Studio project 
 *              - Add clock.c an d clock.h to this project
 *              - Add a empty folder to this project
 *              - Rename this folder as csrc
 *              - Add all files from the csrc folder of Oli Kraus in the csrc folder from the project
 *                You can find csrc folder of Oli Kraus on Github: 
 *                <a href="https://github.com/olikraus/ucglib/tree/master/csrc">https://github.com/olikraus/ucglib/tree/master/csrc</a>
 *              - Overwrite the ucg.h in csrc with the ucg.h from this HAL. 
 *               
 *           After this you can build the example.
 */ 

#define  F_CPU   32000000UL     //!< System clock is 32 MHz

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "clock.h"              // see also https://dolman-wim.nl/xmega/libraries/index.php
#include "csrc/ucg.h"
#include "ucglib_xmega_hal.h"

ucg_t    ucg;                  //!< ucg is a necessary global structure for ucg_lib

/*!
 *  Main function to demonstrate the usage of ucg_lib with the Xmega
 */
int main(void)
{
  ucg_int_t x, y;
  uint8_t a;

  init_clock();
             
  ucg_Init(&ucg, ucg_dev_st7735_18x128x160, ucg_ext_st7735_18,
           (int (*)(struct _ucg_t *, int,  unsigned int,  unsigned char *)) ucg_comm_xmega);

  ucg_SetFontMode(&ucg, UCG_FONT_MODE_TRANSPARENT);  
  ucg_ClearScreen(&ucg);
  ucg_SetFont(&ucg, ucg_font_8x13_tr);
  ucg_SetColor(&ucg, 0, 255, 255, 0);
  ucg_SetRotate90(&ucg);
  ucg_DrawString(&ucg, 2, 20, 0, "TEST DISPLAY");
  _delay_ms(1000);
  
  ucg_SetPrintDir(&ucg, 0);
  ucg_SetPrintPos(&ucg,  2, 40);
  ucg_Print(&ucg, "Hello");
  ucg_Print(&ucg, " ");
  ucg_Print(&ucg, "World!");
  _delay_ms(1000);
  
  ucg_SetColor(&ucg, 0, 255, 180, 180);
  ucg_DrawLine(&ucg, 20, 50, 140, 50);
  ucg_DrawLine(&ucg, 30, 54, 130, 54);
  ucg_DrawLine(&ucg, 40, 58, 120, 58);
  ucg_SetColor(&ucg, 0,   0, 255, 0);
  _delay_ms(1000);
  
  ucg_SetFontMode(&ucg, UCG_FONT_MODE_SOLID);  // overwrite background
  ucg_SetFont(&ucg, ucg_font_8x13_mr);         // proportional font
  ucg_SetPrintPos(&ucg, 2, 80);                
  a = 'A';
  while (1) {
    ucg_Print(&ucg, "%c", a++);
    ucg_GetPrintPos(&ucg, &x, &y);
    if (x > ucg_GetWidth(&ucg)-8) {            // test if line is full
      ucg_SetPrintPos(&ucg, 2, 80);
    }
    if (a > 'Z') {
      a = 'A';
    }
    _delay_ms(500);
  }
}

