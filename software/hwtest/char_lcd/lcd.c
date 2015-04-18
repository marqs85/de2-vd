//
// Copyright (C) 2015  Markus Hiienkari <mhiienka@niksula.hut.fi>
//
// This file is part of Open Source Scan Converter project.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "lcd.h"

void lcd_write(char *r1, char *r2) {
	alt_u8 i;

	// Fill unused space
	for (i=strlen(r1); i<LCD_ROW_LEN; i++)
		r1[i] = ' ';
	for (i=strlen(r2); i<LCD_ROW_LEN; i++)
		r2[i] = ' ';

	alt_up_character_lcd_set_cursor_pos(char_lcd_dev, 0, 0);
	alt_up_character_lcd_write(char_lcd_dev, r1, LCD_ROW_LEN);
	alt_up_character_lcd_set_cursor_pos(char_lcd_dev, 0, 1);
	alt_up_character_lcd_write(char_lcd_dev, r2, LCD_ROW_LEN);
}

void lcd_init() {
	// Open the Character LCD port
	char_lcd_dev = alt_up_character_lcd_open_dev ("/dev/character_lcd_0");
	if ( char_lcd_dev == NULL) {
		printf ("Error: could not open character LCD device\n");
		return -1;
	}

	/* Initialize the character display */
	alt_up_character_lcd_init (char_lcd_dev);
	alt_up_character_lcd_cursor_off(char_lcd_dev);
}
