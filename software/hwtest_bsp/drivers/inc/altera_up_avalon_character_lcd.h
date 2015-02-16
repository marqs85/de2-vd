#ifndef __ALTERA_UP_AVALON_CHARACTER_LCD_H__
#define __ALTERA_UP_AVALON_CHARACTER_LCD_H__

#include <stddef.h>

#include "sys/alt_dev.h"
#include "sys/alt_alarm.h"
#include "sys/alt_warning.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*
 * Device structure definition. Each instance of the driver uses one
 * of these structures to hold its associated state.
 */
typedef struct alt_up_character_lcd_dev {
	/// @brief character mode device structure 
	/// @sa Developing Device Drivers for the HAL in Nios II Software Developer's Handbook
	alt_dev dev;
	/// @brief the base address of the device
	unsigned int base;
} alt_up_character_lcd_dev;

// system functions
/**
 * @brief Initialize the LCD by clearing its display
 *
 * @param lcd -- struct for the LCD Controller device 
 *
 **/
void alt_up_character_lcd_init(alt_up_character_lcd_dev *lcd);

// direct operation functions
/**
 * @brief Open the character LCD device specified by <em> name </em>
 *
 * @param name -- the character LCD name. For example, if the character LCD name in SOPC Builder is "character_lcd_0", then <em> name </em> should be "/dev/character_lcd_0"
 *
 * @return The corresponding device structure, or NULL if the device is not found
 **/
alt_up_character_lcd_dev* alt_up_character_lcd_open_dev(const char* name);

/**
 * @brief Write the characters in the buffer pointed to by <em> ptr </em> to
 * the LCD, starting from where the current cursor points to 
 *
 * @param lcd -- struct for the LCD Controller device 
 * @param ptr  -- the pointer to the char buffer
 * @param len  -- the length of the char buffer
 *
 * @return nothing
 **/
void alt_up_character_lcd_write(alt_up_character_lcd_dev *lcd, const char *ptr, unsigned int len);

/**
 * @brief Write the characters in the NULL-terminated string to the LCD
 *
 * @param lcd -- struct for the LCD Controller device 
 * @param ptr  -- the pointer to the char buffer
 *
 * @return nothing
 **/
void alt_up_character_lcd_string(alt_up_character_lcd_dev *lcd, const char *ptr);

/**
 * reserved for future use
 *
 **/
int alt_up_character_lcd_write_fd(alt_fd *fd, const char *ptr, int len);

/**
 * @brief Set the cursor position
 *
 * @param lcd -- struct for the LCD Controller device 
 * @param x_pos   -- x coordinate ( 0 to 15, from left to right )
 * @param y_pos   -- y coordinate ( 0 for the top row, 1 for the bottom row )
 *
 * @return 0 for success
 **/
int alt_up_character_lcd_set_cursor_pos(alt_up_character_lcd_dev *lcd, unsigned x_pos, 
	unsigned y_pos);

/**
 * @brief Shift the cursor to left or right
 *
 * @param lcd -- struct for the LCD Controller device 
 * @param x_right_shift_offset  -- the number of spaces to shift to the right. If the offset is
 * negative, then the cursor shifts to the left.
 *
 * @return nothing
 **/
void alt_up_character_lcd_shift_cursor(alt_up_character_lcd_dev *lcd, int x_right_shift_offset);

/**
 * @brief Shift the entire display to left or right
 *
 * @param lcd -- struct for the LCD Controller device 
 * @param x_right_shift_offset  -- the number of spaces to shift to the right. If the offset is
 * negative, then the display shifts to the left.
 *
 * @return nothing
 **/
void alt_up_character_lcd_shift_display(alt_up_character_lcd_dev *lcd, int x_right_shift_offset);

/**
 * @brief Erase the character at the specified coordinate 
 *
 * @param lcd -- struct for the LCD Controller device 
 * @param x_pos   -- x coordinate ( 0 to 15, from left to right )
 * @param y_pos   -- y coordinate ( 0 for the top row, 1 for the bottom row )
 *
 * @return 0 for success
 **/
int alt_up_character_lcd_erase_pos(alt_up_character_lcd_dev *lcd, unsigned x_pos, unsigned y_pos);

/**
 * @brief Turn off the cursor
 *
 * @param lcd -- struct for the LCD Controller device 
 *
 * @return nothing
 **/
void alt_up_character_lcd_cursor_off(alt_up_character_lcd_dev *lcd);

/**
 * @brief Turn on the cursor
 *
 * @param lcd -- struct for the LCD Controller device 
 *
 * @return nothing
 **/
void alt_up_character_lcd_cursor_blink_on(alt_up_character_lcd_dev *lcd);

/*
 * Macros used by alt_sys_init 
 */
#define ALTERA_UP_AVALON_CHARACTER_LCD_INSTANCE(name, device) \
	static alt_up_character_lcd_dev device =         	\
	{                                              \
		{                                          \
		  ALT_LLIST_ENTRY,                         \
		  name##_NAME,                             \
		  NULL, /* open */                         \
		  NULL, /* close */                        \
		  NULL, /* read */                         \
		  alt_up_character_lcd_write_fd,           \
		  NULL, /* lseek */                        \
		  NULL, /* fstat */                        \
		  NULL, /* ioctl */                        \
		},                                         \
		name##_BASE,                               \
	}

#define ALTERA_UP_AVALON_CHARACTER_LCD_INIT(name, device) \
  {                                      			\
      alt_up_character_lcd_init(&device);    		\
	  alt_dev_reg(&device.dev);						\
  }


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ALTERA_UP_AVALON_CHARACTER_LCD_H__ */


