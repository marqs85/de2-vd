/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2003 Altera Corporation, San Jose, California, USA.           *
* All rights reserved.                                                        *
*                                                                             *
* Permission is hereby granted, free of charge, to any person obtaining a     *
* copy of this software and associated documentation files (the "Software"),  *
* to deal in the Software without restriction, including without limitation   *
* the rights to use, copy, modify, merge, publish, distribute, sublicense,    *
* and/or sell copies of the Software, and to permit persons to whom the       *
* Software is furnished to do so, subject to the following conditions:        *
*                                                                             *
* The above copyright notice and this permission notice shall be included in  *
* all copies or substantial portions of the Software.                         *
*                                                                             *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  *
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,    *
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE *
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER      *
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING     *
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER         *
* DEALINGS IN THE SOFTWARE.                                                   *
*                                                                             *
* This agreement shall be governed in all respects by the laws of the State   *
* of California and by the laws of the United States of America.              *
*                                                                             *
******************************************************************************/

#ifndef __ALT_UP_CHARACTER_LCD_REGS_H__
#define __ALT_UP_CHARACTER_LCD_REGS_H__

#include <io.h>
/*
 * Command Register (When RS = 0)
 * (In the document, we used the name Instruction Register)
 */
#define ALT_UP_CHARACTER_LCD_COMMAND_REG               0
#define IOADDR_ALT_UP_CHARACTER_LCD_COMMAND(base)      \
        __IO_CALC_ADDRESS_DYNAMIC(base, ALT_UP_CHARACTER_LCD_COMMAND_REG)
#define IORD_ALT_UP_CHARACTER_LCD_COMMAND(base)        \
        IORD_8DIRECT(base, ALT_UP_CHARACTER_LCD_COMMAND_REG)
#define IOWR_ALT_UP_CHARACTER_LCD_COMMAND(base, data)  \
        IOWR_8DIRECT(base, ALT_UP_CHARACTER_LCD_COMMAND_REG, data)

//#define ALT_UP_CHARACTER_LCD_COMMAND_MSK			    (0xFF)
//#define ALT_UP_CHARACTER_LCD_COMMAND_OFST			    (0x00)

// Clear Display
#define	ALT_UP_CHARACTER_LCD_COMM_CLEAR_DISPLAY			(0x01)
// Return Home
#define	ALT_UP_CHARACTER_LCD_COMM_RETURN_HOME			(0x02)
//Entry Mode 
#define	ALT_UP_CHARACTER_LCD_COMM_ENTRY_DIR_RIGHT		(0x06)
#define	ALT_UP_CHARACTER_LCD_COMM_ENTRY_DIR_LEFT 		(0x04)
#define	ALT_UP_CHARACTER_LCD_COMM_ENTRY_SHIFT_ENABLE	(0x05)
#define	ALT_UP_CHARACTER_LCD_COMM_ENTRY_SHIFT_DISABLE	(0x04)

// Display ON/OFF Control
#define	ALT_UP_CHARACTER_LCD_COMM_DISPLAY_ON			(0x0C)
#define	ALT_UP_CHARACTER_LCD_COMM_CURSOR_ON				(0x0E)
#define	ALT_UP_CHARACTER_LCD_COMM_CURSOR_BLINK_ON		(0x0F)

#define	ALT_UP_CHARACTER_LCD_COMM_DISPLAY_OFF			(0x08)

#define	ALT_UP_CHARACTER_LCD_COMM_CURSOR_OFF			(0x0C) //equivalent to ALT_UP_CHARACTER_LCD_COMM_DISPLAY_ON
#define	ALT_UP_CHARACTER_LCD_COMM_CURSOR_BLINK_OFF		(0x0E) //equivalent to ALT_UP_CHARACTER_LCD_COMM_CURSOR_ON

// Cursor/Display Shift
// cause the entire display to move to left or right (the origin of the display is also changed)
#define	ALT_UP_CHARACTER_LCD_COMM_DISPLAY_SHIFT_RIGHT	(0x1C)
#define	ALT_UP_CHARACTER_LCD_COMM_DISPLAY_SHIFT_LEFT	(0x18)
// move the cursor to left or right
#define	ALT_UP_CHARACTER_LCD_COMM_CURSOR_SHIFT_RIGHT	(0x14)
#define	ALT_UP_CHARACTER_LCD_COMM_CURSOR_SHIFT_LEFT		(0x10)

/* 
 * Data Register (When RS = 1)
 */
#define ALT_UP_CHARACTER_LCD_DATA_REG                  1
#define IOADDR_ALT_UP_CHARACTER_LCD_DATA(base)         \
        __IO_CALC_ADDRESS_DYNAMIC(base, ALT_UP_CHARACTER_LCD_DATA_REG)
#define IORD_ALT_UP_CHARACTER_LCD_DATA(base)           \
        IORD_8DIRECT(base, ALT_UP_CHARACTER_LCD_DATA_REG) 
#define IOWR_ALT_UP_CHARACTER_LCD_DATA(base, data)     \
        IOWR_8DIRECT(base, ALT_UP_CHARACTER_LCD_DATA_REG, data)

//#define ALT_UP_CHARACTER_LCD_DATA_MSK             	(0xFF)
//#define ALT_UP_CHARACTER_LCD_DATA_OFST            	(0)

#define ALT_UP_CHARACTER_LCD_BF_MSK				(0x80)
#define ALT_UP_CHARACTER_LCD_BF_OFST			(7)



#endif /* __ALT_UP_CHARACTER_LCD_REGS_H__ */
