/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2006 Altera Corporation, San Jose, California, USA.           *
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

#include <string.h>
#include <errno.h>
#include <limits.h>

#include <priv/alt_file.h>

#include "altera_up_avalon_character_lcd.h"
#include "altera_up_avalon_character_lcd_regs.h"

/////////////////////////////////////////////////////////////////////////////
/*
 * Internal Utility Functions:
 * these functions are not supposed to be called directly by the user so they
 * are not defined in the header file
 */
/////////////////////////////////////////////////////////////////////////////

/**
 * @brief calculate the DDRAM address according to the provided x and y coordinates
 *
 * @param x_pos  x coordinate ( 0 to 15, from left to right )
 * @param y_pos  y coordinate ( 0 for the top row, 1 for the bottom row )
 *
 * @return the converted address
 * @sa the datasheet for the LCD Display Controller on the DE2 Board
 * @note the function requires that the input are in the valid range
 *
 **/
unsigned char get_DDRAM_addr(unsigned x_pos, unsigned y_pos)
{
	//assume valid inputs
	unsigned char addr = 0x00000000;
	if (y_pos == 0)
	{
		addr |= x_pos;
	}
	else
	{
		addr |= x_pos;
		addr |= 0x00000040;
	}
	// b_7 is always 1 for DDRAM address, see datasheet
	return (addr | 0x00000080);
}

/**
 * @brief send a command to the Instruction Register of the LCD controller
 *
 * @param lcd -- struct for the LCD Controller device 
 * @param cmd -- the command bits 
 *
 * @return nothing
 **/
void alt_up_character_lcd_send_cmd(alt_up_character_lcd_dev *lcd, unsigned char cmd)
{
 	// NOTE: We use the term Instruction Register and Control Register interchangeably
	IOWR_ALT_UP_CHARACTER_LCD_COMMAND(lcd->base, cmd);
}

////////////////////////////////////////////////////////////////////////////
/*
 * Interface Functions : 
 * The interface of these functions are provided to the user. See the header
 * file for a detailed description of each function
 */
////////////////////////////////////////////////////////////////////////////

void alt_up_character_lcd_init(alt_up_character_lcd_dev *lcd)
{
	IOWR_ALT_UP_CHARACTER_LCD_COMMAND(lcd->base, ALT_UP_CHARACTER_LCD_COMM_CLEAR_DISPLAY);
	// register the device 
	// see "Developing Device Drivers for the HAL" in "Nios II Software Developer's Handbook"
}

alt_up_character_lcd_dev* alt_up_character_lcd_open_dev(const char* name)
{
  // find the device from the device list 
  // (see altera_hal/HAL/inc/priv/alt_file.h 
  // and altera_hal/HAL/src/alt_find_dev.c 
  // for details)
  alt_up_character_lcd_dev *dev = (alt_up_character_lcd_dev*)alt_find_dev(name, &alt_dev_list);

  return dev;
}

void alt_up_character_lcd_write(alt_up_character_lcd_dev *dev, const char *ptr, unsigned int len)
{
	unsigned int i;
	for (i = 0; i < len; i++)
	{
		IOWR_ALT_UP_CHARACTER_LCD_DATA(dev->base, *(ptr+i));
	}
}

void alt_up_character_lcd_string(alt_up_character_lcd_dev *dev, const char *ptr)
{
	while ( *ptr )
	{
		IOWR_ALT_UP_CHARACTER_LCD_DATA(dev->base, *(ptr));
		++ptr;
	}
}

// this function isn't used, and is included for future upgrades
int alt_up_character_lcd_write_fd(alt_fd *fd, const char *ptr, int len)
{
	alt_up_character_lcd_write( (alt_up_character_lcd_dev *) fd->dev, ptr, (unsigned int) len);
	return 0;
}

int alt_up_character_lcd_set_cursor_pos(alt_up_character_lcd_dev *lcd, unsigned x_pos, 
	 unsigned y_pos)
{
	//boundary check
	if (x_pos > 39 || y_pos > 1 )
		// invalid argument
		return -1;
	// calculate address
	unsigned char addr = get_DDRAM_addr(x_pos, y_pos);
	// set the cursor
	alt_up_character_lcd_send_cmd(lcd, addr);
	return 0;
}

void alt_up_character_lcd_shift_cursor(alt_up_character_lcd_dev *lcd, int x_right_shift_offset)
{
	if (x_right_shift_offset == 0) 
		// don't ask me to do nothing 
		return;

	// see shift right or left
	unsigned char shift_cmd = (x_right_shift_offset > 0) ? 
		ALT_UP_CHARACTER_LCD_COMM_CURSOR_SHIFT_RIGHT : ALT_UP_CHARACTER_LCD_COMM_CURSOR_SHIFT_LEFT;
	// see how many to shift
	unsigned char num_offset = (x_right_shift_offset > 0) ? x_right_shift_offset : 
		-x_right_shift_offset;
	// do the shift
	while (num_offset-- > 0)
		alt_up_character_lcd_send_cmd(lcd, shift_cmd);
}

void alt_up_character_lcd_shift_display(alt_up_character_lcd_dev *lcd, int x_right_shift_offset)
{
	if (x_right_shift_offset == 0) 
		// don't ask me to do nothing 
		return;

	// see shift right or left
	unsigned char shift_cmd = (x_right_shift_offset > 0) ? 
		ALT_UP_CHARACTER_LCD_COMM_DISPLAY_SHIFT_RIGHT : ALT_UP_CHARACTER_LCD_COMM_DISPLAY_SHIFT_LEFT;
	// see how many to shift
	unsigned char num_offset = (x_right_shift_offset > 0) ? x_right_shift_offset : 
		-x_right_shift_offset;
	// do the shift
	while (num_offset-- > 0)
		alt_up_character_lcd_send_cmd(lcd, shift_cmd);
}

int alt_up_character_lcd_erase_pos(alt_up_character_lcd_dev *lcd, unsigned x_pos, unsigned y_pos)
{
	// boundary check
	if (x_pos > 39 || y_pos > 1 )
		return -1;

	// get address
	unsigned char addr = get_DDRAM_addr(x_pos, y_pos);
	// set cursor to dest point
	alt_up_character_lcd_send_cmd(lcd, addr);
	//send an empty char as erase (refer to the Character Generator ROM part of the Datasheet)
	IOWR_ALT_UP_CHARACTER_LCD_DATA(lcd->base, (0x00000002) );
	return 0;
}

void alt_up_character_lcd_cursor_off(alt_up_character_lcd_dev *lcd)
{
	alt_up_character_lcd_send_cmd(lcd, ALT_UP_CHARACTER_LCD_COMM_CURSOR_OFF);
}

void alt_up_character_lcd_cursor_blink_on(alt_up_character_lcd_dev *lcd)
{
	alt_up_character_lcd_send_cmd(lcd, ALT_UP_CHARACTER_LCD_COMM_CURSOR_BLINK_ON);
}

