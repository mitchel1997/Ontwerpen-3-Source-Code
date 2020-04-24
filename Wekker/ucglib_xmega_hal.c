/*!
 *  \file    ucglib_xmega_hal.c
 *  \author  Wim Dolman (<a href="mailto:w.e.dolman@hva.nl">w.e.dolman@hva.nl</a>)
 *  \date    29-08-2019
 *  \version 2.0
 *
 *  \brief   Xmega Hardware Abstraction Layer for ucglib from Oli Kraus
 *
 *  \details This Hardware Abstraction Layer is created confirm the
 *           <a href"https://github.com/olikraus/ucglib/wiki/hal">instructions</a> of Oli Kraus.
 *
 *           This file contains a callback function <code>ucg_com_xmega_bb()</code> that handles the
 *           communication with the Xmega using SPI or bit banging.
 *           The Arduino/C++ implementation of ucglib contains extra printing facilities.
 *           This file contains a bunch of functions that implements the same facilities.
 *           So you can use <code>ucg_SetPrintPos()</code>, <code>ucg_SetPrintDir()</code> 
 *           and in stead of <code>print</code> and <code>println</code> 
 *           you can use <code>ucg_Print()</code> which prints a formatstring.
 *
 *           This print facility uses a hook in the struct ucg_t. Add to the struct these
 *           lines:
 *  \verbatim
 *            #ifdef __AVR_XMEGA__
 *            void  *xmega_hook;   // added pointer for print hook
 *            #endif \endverbatim
 *           or uses the ucg.h of the HAL package in stead
 */

#define F_CPU 320000000UL                 //!< System clock is 32 MHz

#include <avr/io.h>
#include <util/delay.h>
#include "csrc/ucg.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "ucglib_xmega_hal.h"

#ifndef DOXYGEN_SKIP
#if UCG_XMEGA_USE==UCG_XMEGA_USING_BB
  #define UCG_XMEGA_SCK_PORT      UCG_XMEGA_BB_SCK_PORT  
  #define UCG_XMEGA_SDA_PORT      UCG_XMEGA_BB_SDA_PORT 
  #define UCG_XMEGA_CS_PORT       UCG_XMEGA_BB_CS_PORT   
  #define UCG_XMEGA_RESET_PORT    UCG_XMEGA_BB_RESET_PORT
  #define UCG_XMEGA_CD_PORT       UCG_XMEGA_BB_CD_PORT   
  #define UCG_XMEGA_BLK_PORT      UCG_XMEGA_BB_BLK_PORT  
  
  #define UCG_XMEGA_SCK_bm        (1 << UCG_XMEGA_BB_SCK_bp)
  #define UCG_XMEGA_SDA_bm        (1 << UCG_XMEGA_BB_SDA_bp)
  #define UCG_XMEGA_CS_bm         (1 << UCG_XMEGA_BB_CS_bp)
  #define UCG_XMEGA_RESET_bm      (1 << UCG_XMEGA_BB_RESET_bp)          
  #define UCG_XMEGA_CD_bm         (1 << UCG_XMEGA_BB_CD_bp)            
  #define UCG_XMEGA_BLK_bm        (1 << UCG_XMEGA_BB_BLK_bp)           
  #define UCG_XMEGA_BLK_bp        UCG_XMEGA_BB_BLK_bp           
#else // using SPI interface
  #define UCG_XMEGA_SCK_PORT      UCG_XMEGA_SPI_SCK_PORT  
  #define UCG_XMEGA_SDA_PORT      UCG_XMEGA_SPI_MOSI_PORT 
  #define UCG_XMEGA_MISO_PORT     UCG_XMEGA_SPI_MISO_PORT 
  #define UCG_XMEGA_CS_PORT       UCG_XMEGA_SPI_SS_PORT   
  #define UCG_XMEGA_RESET_PORT    UCG_XMEGA_SPI_RESET_PORT
  #define UCG_XMEGA_CD_PORT       UCG_XMEGA_SPI_CD_PORT   
  #define UCG_XMEGA_BLK_PORT      UCG_XMEGA_SPI_BLK_PORT
  
  #define UCG_XMEGA_SCK_bm        (1 << UCG_XMEGA_SPI_SCK_bp) 
  #define UCG_XMEGA_MISO_bm       (1 << UCG_XMEGA_SPI_MISO_bp)
  #define UCG_XMEGA_SDA_bm        (1 << UCG_XMEGA_SPI_MOSI_bp)
  #define UCG_XMEGA_CS_bm         (1 << UCG_XMEGA_SPI_SS_bp)  
  #define UCG_XMEGA_RESET_bm      (1 << UCG_XMEGA_SPI_RESET_bp)          
  #define UCG_XMEGA_CD_bm         (1 << UCG_XMEGA_SPI_CD_bp)            
  #define UCG_XMEGA_BLK_bm        (1 << UCG_XMEGA_SPI_BLK_bp)           
  #define UCG_XMEGA_BLK_bp        UCG_XMEGA_SPI_BLK_bp           
#endif
#endif

#if (UCG_XMEGA_USE==UCG_XMEGA_USING_BB)&&(UCG_XMEGA_BLK==UCG_XMEGA_BLK_EXTERN)
  #warning if display is connect directly to Xmega, use UCG_XMEGA_BLK_DISABLED in stead of UCG_XMEGA_BLK_EXTERN to disable the input pin
#endif

/*! \brief  Initialization of the communication 
 *
 *  \return void
 */
static void xmega_init(void)
{
  UCG_XMEGA_RESET_PORT.DIRSET = UCG_XMEGA_RESET_bm;
  UCG_XMEGA_CD_PORT.DIRSET    = UCG_XMEGA_CD_bm;
  #if UCG_XMEGA_BLK==UCG_XMEGA_BLK_DISABLED
    *( (register8_t *) ( &(UCG_XMEGA_BLK_PORT.PIN0CTRL) + UCG_XMEGA_BLK_bp) ) = PORT_ISC_INPUT_DISABLE_gc;
  #elif UCG_XMEGA_BLK==UCG_XMEGA_BLK_CONNECT 
      UCG_XMEGA_BLK_PORT.DIRSET   = UCG_XMEGA_BLK_bm;
      UCG_XMEGA_BLK_PORT.OUTSET   = UCG_XMEGA_BLK_bm;
  #endif
      
  UCG_XMEGA_SCK_PORT.DIRSET   = UCG_XMEGA_SCK_bm;
  UCG_XMEGA_SDA_PORT.DIRSET   = UCG_XMEGA_SDA_bm;
  UCG_XMEGA_CS_PORT.DIRSET    = UCG_XMEGA_CS_bm;
  UCG_XMEGA_CS_PORT.OUTSET    = UCG_XMEGA_CS_bm;

  #if UCG_XMEGA_USE==UCG_XMEGA_USING_SPI
    UCG_XMEGA_MISO_PORT.DIRCLR  = UCG_XMEGA_MISO_bm;
    UCG_XMEGA_INTERFACE.CTRL    =  SPI_ENABLE_bm |  // enable SPI
                                 SPI_MASTER_bm |  // master mode
                              // SPI_CLK2X_bm  |  // no double clock speed
                              // SPI_DORD_bm   |  // MSB first
                                 SPI_MODE_0_gc |  // SPI mode 0
                                 SPI_PRESCALER_DIV4_gc;  // prescaling 4
  #endif
}

/*! \brief  Disable communication
*
*  \return void
*/
static void xmega_disable(void)
{
  #if UCG_XMEGA_USE==UCG_XMEGA_USING_SPI
    UCG_XMEGA_INTERFACE.CTRL    = UCG_XMEGA_INTERFACE.CTRL & ~SPI_ENABLE_bm;
  #endif
  #if UCG_XMEGA_BLK==UCG_XMEGA_BLK_CONNECT
    UCG_XMEGA_BLK_PORT.OUTCLR = UCG_XMEGA_BLK_bm;
  #endif
}

/*! \brief  Transfer a byte 
 *
 *  \return void
 */
static void xmega_transfer(uint8_t data)
{
  #if UCG_XMEGA_USE==UCG_XMEGA_USING_BB
    for (uint8_t i=0; i<8; i++) {
      if (data & 0x80) {
        UCG_XMEGA_SDA_PORT.OUTSET = UCG_XMEGA_SDA_bm; 
      } else {
        UCG_XMEGA_SDA_PORT.OUTCLR = UCG_XMEGA_SDA_bm;
      }
      UCG_XMEGA_SCK_PORT.OUTSET = UCG_XMEGA_SCK_bm;
      UCG_XMEGA_SCK_PORT.OUTCLR = UCG_XMEGA_SCK_bm;
      data <<= 1;
    }
    
    UCG_XMEGA_SDA_PORT.OUTSET = UCG_XMEGA_SDA_bm;
  #else // using SPI interface
    UCG_XMEGA_INTERFACE.DATA = data;
    while(!(UCG_XMEGA_INTERFACE.STATUS & (SPI_IF_bm)));
  #endif
}

/////////////////////////////////////////////////

/*!
 * Struct for compatibility printing facilities with Arduino/C++ version of library 
 */
typedef struct {      
  FILE      *fp;      //!< file pointer for printing 
  ucg_int_t  tx;      //!< current x coordinate of the position
  ucg_int_t  ty;      //!< current y coordinate of the position
  uint8_t    tdir;    //!< current printing direction
} ucg_print_t;           

ucg_t *curr_ucg;    //!< global pointer necessary for the printing facilities 

/*! \brief  Sets the position for next "print" command.
 *
 *  \param  ucg      pointer to struct for the display
 *  \param  x        x-coordinate of the position
 *  \param  y        y-coordinate of the position
 *
 *  \return void
 */
void ucg_SetPrintPos(ucg_t *ucg, ucg_int_t x, ucg_int_t y)
{
  ((ucg_print_t *)ucg->xmega_hook)->tx = x;
  ((ucg_print_t *)ucg->xmega_hook)->ty = y;
}

/*! \brief  Gets the current position of the 'print cursor'
 *
 *  \param  ucg      pointer to struct for the display
 *  \param  x        pointer to a variable for the x-coordinate of the current position
 *  \param  y        pointer to a variable for the y-coordinate of the current position
 *
 *  \return void
 */
void ucg_GetPrintPos(ucg_t *ucg, ucg_int_t *x, ucg_int_t *y)
{
  *x = ((ucg_print_t *)ucg->xmega_hook)->tx;
  *y = ((ucg_print_t *)ucg->xmega_hook)->ty;
}

/*! \brief  Sets the direction for next "print" command.
 *
 *  \param  ucg      pointer to struct for the display
 *  \param  dir      the direction
 *
 *  \return void
 */
void ucg_SetPrintDir(ucg_t *ucg, uint8_t dir)
{
  ((ucg_print_t *)ucg->xmega_hook)->tdir = dir;
}

/*! \brief  Put a character to the display at the
 *          current position and in the current direction.
 *
 *  \param  ucg      pointer to struct for the display
 *  \param  dir      the direction
 *
 *  \return void
 */
static int ucg_Putc(char c, FILE *stream)
{
  ucg_int_t delta;
  ucg_print_t *p = (ucg_print_t *) (curr_ucg->xmega_hook);
  
  delta = ucg_DrawGlyph(curr_ucg, p->tx, p->ty, p->tdir, c);
  
  switch(p->tdir) {
    case          0: p->tx += delta; break;
    case          1: p->ty += delta; break;
    case          2: p->tx -= delta; break;
    default: case 3: p->ty -= delta; break;
  }
  
  return 1;
}

/*! \brief  Put a formatted string to the display at the
 *          current position and in the current direction.
 *
 *          This replaces print and println from the Arduino implementation of ucg_lib
 *
 *          The Arduino style:
 * \verbatim
 *          ucg.print("text ");
 *          ucg.print(x);     // x is an int
 *          ucg.print(" more text ");
 *          ucg.print(y);     // y is a float
 *          ucg.println(";"); \endverbatim
 *              
 *          The replacement in Xmega style:
 *\verbatim
 *          ucg_Print(&ucg, "text %d more text %f;\n", x, y); \endverbatim
 *
 *  \param  ucg      pointer to struct for the display
 *  \param  fmt      formatstring with escape sequences
 *  \param  ...      variables that are printed
 *
 *  \return void
 */
void ucg_Print(ucg_t *ucg, char *fmt, ...)
{
  va_list vl;

  curr_ucg = ucg;
  va_start(vl, fmt);
  vfprintf( ((ucg_print_t *) (curr_ucg->xmega_hook))->fp, fmt, vl);
  va_end(vl);
}

/*! \brief  Initializes the printing facilities compatible with Arduino/C++ version of library
 *
 *  \param  ucg      pointer to struct for the display
 *
 *  \return void
 */
void ucg_PrintInit(ucg_t *ucg)
{
  if  (ucg->xmega_hook != NULL) return;
  
  ucg_print_t *p = (ucg_print_t *) malloc(sizeof(ucg_print_t));
  
  if (p == NULL) return;
  
  p->fp    = fdevopen(ucg_Putc, NULL); 
  p->tx    = 0;
  p->ty    = 0;
  p->tdir  = 0;
  
  ucg->xmega_hook = (void *) p;

}
/////////////////////////////////////////////////

/*! \brief  The callback function for communication between the Xmega and the display.
 *
 *  \param  ucg      pointer to struct for the display
 *  \param  msg      number of the message (action to be done) 
 *  \param  arg      depends on msg: number of arguments, number of microseconds, ...
 *  \param  data     pointer to 8-bit data-array with bytes that needs to be send
 *
 *  \return 16-bit value, always 1
 */
int16_t ucg_comm_xmega(ucg_t *ucg, int16_t msg, uint16_t arg, uint8_t *data)
{
  switch(msg)
  {
    case UCG_COM_MSG_POWER_UP:
      xmega_init();
      ucg_PrintInit(ucg);
      break;
    case UCG_COM_MSG_POWER_DOWN:
      xmega_disable();
      break;
    case UCG_COM_MSG_DELAY:
      for(uint16_t i=0; i<arg; i++) {
        _delay_us(1);
      }
      break;
    case UCG_COM_MSG_CHANGE_RESET_LINE:
      if (arg) {
        UCG_XMEGA_RESET_PORT.OUTSET = UCG_XMEGA_RESET_bm;
      } else {
        UCG_XMEGA_RESET_PORT.OUTCLR = UCG_XMEGA_RESET_bm;
      }
      break;
    case UCG_COM_MSG_CHANGE_CS_LINE:
      if (arg) {
        UCG_XMEGA_CS_PORT.OUTSET = UCG_XMEGA_CS_bm;
      } else {
        UCG_XMEGA_CS_PORT.OUTCLR = UCG_XMEGA_CS_bm;
      }
      break;
    case UCG_COM_MSG_CHANGE_CD_LINE:
      if (arg) {
        UCG_XMEGA_CD_PORT.OUTSET = UCG_XMEGA_CD_bm;
      } else {
        UCG_XMEGA_CD_PORT.OUTCLR = UCG_XMEGA_CD_bm;
      }
      break;
    case UCG_COM_MSG_SEND_BYTE:
      xmega_transfer(arg);
      break;
    case UCG_COM_MSG_REPEAT_1_BYTE:
      while( arg > 0 ) {
        xmega_transfer(data[0]);
        arg--;
      }
      break;
    case UCG_COM_MSG_REPEAT_2_BYTES:
      while( arg > 0 ) {
        xmega_transfer(data[0]);
        xmega_transfer(data[1]);
        arg--;
      }
      break;
    case UCG_COM_MSG_REPEAT_3_BYTES:
      while( arg > 0 ) {
        xmega_transfer(data[0]);
        xmega_transfer(data[1]);
        xmega_transfer(data[2]);
        arg--;
      }
      break;
    case UCG_COM_MSG_SEND_STR:
      while( arg > 0 ) {
        xmega_transfer(*data++);
        arg--;
        }
      break;
  }
  
  return 1;
}

/////////////////////////////////////////////////
