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

#include <stdio.h>
#include <unistd.h>
#include "system.h"
#include "string.h"
#include "altera_avalon_pio_regs.h"
#include "Altera_UP_SD_Card_Avalon_Interface.h"
#include "altera_epcq_controller.h"
#include "i2c_opencores.h"
#include "tvp7002.h"
#include "video_modes.h"
#include "lcd.h"
#include "sysconfig.h"

#ifdef HDMITX
#include "it6613.h"
#include "hdmitx.h"
#endif

// Must be defined here again - cannot access statically defined epcq_controller_0 in alt_sys_init.c
ALTERA_EPCQ_CONTROLLER_AVL_MEM_AVL_CSR_INSTANCE ( EPCQ_CONTROLLER_0, EPCQ_CONTROLLER_0_AVL_MEM, EPCQ_CONTROLLER_0_AVL_CSR, epcq_controller_1);

#define FW_FILE "vp.rbf"
#define FW_VER "0.28"

#define LINECNT_THOLD 1
#define STABLE_THOLD 1

// AV control word:
//    [20:17]         [16:13]         [12:10]            [9:8]          [7]      [6]        [5:4]     [3]     [2]    [1]    [0]
// | H_MASK[3:0] | V_MASK[3:0] | SCANLINESTR[2:0] | RESERVED[1:0] | TVP_CLKSEL | SLEN | L3MODE[1:0] | L3EN || BTN3 | BTN2 | BTN1 |

#define SCANLINESTR_MAX 		0x07
#define HV_MASK_MAX		 		0x0f
#define L3_MODE_MAX				3
#define S480P_MODE_MAX			2
#define SL_MODE_MAX				2
#define SYNC_LPF_MAX			3

//#define TVP_CLKSEL_BIT	 		(1<<7)

#define H_MASK_MASK		 		0x0f
#define H_MASK_LSB 				17

#define V_MASK_MASK		 		0x0f
#define V_MASK_LSB 				13

#define SCANLINESTR_MASK 		0x07
#define SCANLINESTR_LSB 		10

//#define TVP_CLKSEL_BIT	 		(1<<9)

#define SCANLINES_ENABLE_BIT 	(1<<6)

#define LINETRIPLE_MODE_MASK	0x03
#define LINETRIPLE_MODE_LSB		4

#define LINETRIPLE_ENABLE_BIT 	(1<<3)
#define L3_MODE_MAX				3

#define SAMPLER_PHASE_MIN		-16
#define SAMPLER_PHASE_MAX		15

#define REMOTE_ENABLE_BIT	 	(1<<7)

#define TX_MODE_BIT			 	(1<<8)

#define AV_SEL_MASK 			0x07
#define BTN3_BIT 				(1<<2)
#define BTN2_BIT 				(1<<1)
#define BTN1_BIT				(1<<0)

#define BTN3_CODE				0x00FFF807
#define BTN2_CODE				0x00FFB847
#define BTN1_CODE				0x00FF906F
#define BTN_SRC_CODE			0x00FF2AD5
#define BTN_MUTE_CODE 			0x00FF6897
#define BTN_RECORD_CODE			0X00FF32CD
#define BTN_TSHIFT_CODE			0X00FF30CF
#define BTN_VOLPLUS_CODE		0x00FF7887
#define BTN_VOLMINUS_CODE		0x00FF50AF
#define BTN_CHPLUS_CODE			0x00FFA05F
#define BTN_CHMINUS_CODE		0x00FF40BF
#define BTN_RECALL_CODE			0x00FF38C7

#define lcd_write_menu() lcd_write((char*)&menu_row1, (char*)&menu_row2)
#define lcd_write_status() lcd_write((char*)&row1, (char*)&row2)

static const char *avinput_str[] = { "NO MODE", "AV1: RGBS", "AV2: RGBHV", "AV2: RGBS", "AV3: YPbPr", "AV3: RGsB" };
static const char *l3_mode_desc[] = { "Generic 16:9", "Generic 4:3", "320x240 optim.", "256x240 optim." };
static const char *s480p_desc[] = { "Auto", "DTV 480p", "VGA 640x480" };
static const char *sl_mode_desc[] = { "Off", "Horizontal", "Vertical" };
static const char *sync_lpf_desc[] = { "Off", "33MHz", "10MHz", "2.5MHz" };

typedef enum {
   AV_KEEP				= 0,
   AV1_RGBs				= 1,
   AV2_RGBHV			= 2,
   AV2_RGBs				= 3,
   AV3_YPBPR			= 4,
   AV3_RGsB				= 5
} avinput_t;

// In reverse order of importance
typedef enum {
	NO_CHANGE			= 0,
	INFO_CHANGE			= 1,
	MODE_CHANGE			= 2,
	TX_MODE_CHANGE		= 3,
	ACTIVITY_CHANGE		= 4
} status_t;

typedef enum {
	TX_MODE,
	SCANLINE_MODE,
	SCANLINE_STRENGTH,
	SCANLINE_ID,
	H_MASK,
	V_MASK,
	SAMPLER_480P,
	SAMPLER_PHASE,
	YPBPR_COLORSPACE,
	LPF,
	SYNC_LPF,
	LINETRIPLE_ENABLE,
	LINETRIPLE_MODE,
	FW_UPDATE
} menuitem_id;

typedef enum {
	NO_ACTION			= 0,
	NEXT_PAGE			= 1,
	PREV_PAGE			= 2,
	VAL_PLUS			= 3,
	VAL_MINUS			= 4,
} menucode_id;

typedef enum {
	TX_HDMI				= 0,
	TX_DVI				= 1
} tx_mode_t;

typedef struct {
    alt_u8 sl_mode;
	alt_u8 sl_str;
	alt_u8 sl_id;
	alt_u8 linemult_target;
	alt_u8 l3_mode;
	alt_u8 h_mask;
	alt_u8 v_mask;
	alt_u8 tx_mode;
	alt_u8 s480p_mode;
	alt_8 sampler_phase;
	alt_u8 ypbpr_cs;
	alt_u8 lpf;
	alt_u8 sync_lpf;
} avconfig_t;

// Target configuration
avconfig_t tc;

//TODO: transform binary values into flags
typedef struct {
	alt_u32 totlines;
    alt_u32 clkcnt;
    alt_u8 progressive;
    alt_u8 macrovis;
    alt_u8 refclk;
    alt_8 id;
    alt_u8 sync_active;
    alt_u8 linemult;
    alt_u8 remote_ctrl;
    avinput_t avinput;
    // Current configuration
    avconfig_t cc;
} avmode_t;

// Current mode
avmode_t cm;

typedef struct {
    const menuitem_id id;
    const char *desc;
} menuitem_t;

const menuitem_t menu[] = {
#ifdef HDMITX
	{ TX_MODE,				"TX mode" },
#endif
	{ SCANLINE_MODE,		"Scanlines" },
	{ SCANLINE_STRENGTH,	"Scanline str." },
	{ SCANLINE_ID,			"Scanline id" },
	{ H_MASK,				"Horizontal mask" },
	{ V_MASK,				"Vertical mask" },
	{ SAMPLER_480P,			"480p in sampler" },
	{ SAMPLER_PHASE,		"Sampling phase" },
	{ YPBPR_COLORSPACE,		"YPbPr in ColSpa" },
	{ LPF,					"Video LPF" },
	{ SYNC_LPF,				"Integ. sync LPF" },
	{ LINETRIPLE_ENABLE,	"240p linetriple" },
	{ LINETRIPLE_MODE,		"Linetriple mode" },
#ifndef DEBUG
	{ FW_UPDATE,			"Firmware update" },
#endif
};

#define MENUITEMS (sizeof(menu)/sizeof(menuitem_t))

extern mode_data_t video_modes[];
extern ypbpr_to_rgb_csc_t csc_coeffs[];
//extern alt_llist alt_dev_list;

alt_epcq_controller_dev *epcq_controller_dev;
alt_up_sd_card_dev *sdcard_dev;

alt_u8 target_typemask;
alt_u8 stable_frames;

char row1[LCD_ROW_LEN+1], row2[LCD_ROW_LEN+1], menu_row1[LCD_ROW_LEN+1], menu_row2[LCD_ROW_LEN+1];

// EPCS64 pagesize is 256 bytes and EP4CE115 uses 4215 pages
#define PAGESIZE 256
#define BITSTREAM_PAGES 4215
#define PAGES_PER_SECTOR 256

short int sd_fw_handle;

alt_u8 menu_active, menu_page;
alt_u32 remote_code, remote_code_prev;

alt_u32 frameno;

alt_u8 reverse_bits(alt_u8 b) {
   b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
   b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
   b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
   return b;
}

int check_flash() {
	ALTERA_EPCQ_CONTROLLER_INIT ( EPCQ_CONTROLLER_0, epcq_controller_1);

	//epcq_controller_dev = (alt_epcq_controller_dev*)alt_find_dev(EPCQ_CONTROLLER_0_AVL_MEM_NAME, &alt_dev_list);
	epcq_controller_dev = &epcq_controller_1;

	if ((epcq_controller_dev == NULL))
		return 0;

	//printf("Flash size in bytes: %d\nSector size: %d (%d pages)\nPage size: %d\n", epcq_controller_dev->size_in_bytes, epcq_controller_dev->sector_size, epcq_controller_dev->sector_size/epcq_controller_dev->page_size, epcq_controller_dev->page_size);

	return (epcq_controller_dev->is_epcs && (epcq_controller_dev->page_size == PAGESIZE));
}

int write_flash_page(alt_u8 *page, alt_u32 pagenum) {
	int retval, i, progress;
	alt_u32 led_row;

#ifdef FLASHDEBUG
	alt_u8 readbuf[PAGESIZE];
	if (pagenum == 0) {
		retval = alt_epcq_controller_read(&epcq_controller_dev->dev, pagenum, readbuf, PAGESIZE);
		printf("Flash read: status: %d sid: %u pagesize %u\n", retval, epcq_controller_dev->is_epcs, epcq_controller_dev->page_size);
		for (i=0; i<PAGESIZE; i++) {
			if (i%16 == 0)
				printf("\n");
			printf("0x%.2x ",  readbuf[i]);
		}
		printf("\n");
	}
#endif

	if ((pagenum % PAGES_PER_SECTOR) == 0) {
		printf("Erasing sector %d\n", pagenum/PAGES_PER_SECTOR);
		retval = alt_epcq_controller_erase_block(&epcq_controller_dev->dev, pagenum*PAGESIZE);

		if (retval != 0) {
			strncpy(menu_row1, "Flash erase", LCD_ROW_LEN+1);
			sniprintf(menu_row2, LCD_ROW_LEN+1, "error %d", retval);
			lcd_write_menu();
			printf("Flash erase error, sector %d\nRetval %d\n", pagenum/PAGES_PER_SECTOR, retval);
			usleep(1000000);
			return retval;
		}
	}

	retval = alt_epcq_controller_write_block(&epcq_controller_dev->dev, (pagenum/PAGES_PER_SECTOR)*PAGES_PER_SECTOR*PAGESIZE, pagenum*PAGESIZE, page, PAGESIZE);

	if (retval != 0) {
		strncpy(menu_row1, "Flash write", LCD_ROW_LEN+1);
		strncpy(menu_row2, "error", LCD_ROW_LEN+1);
		lcd_write_menu();
		printf("Flash write error, page %d\nRetval %d\n", pagenum, retval);
		usleep(1000000);
	} else {
		printf("Wrote page %d\n", pagenum);
	}

	// Update progress of 7-seg display
	progress = 100*pagenum / BITSTREAM_PAGES;
	led_row = ((progress / 10)<<28) | ((progress % 10)<<24) | 0x00ffffff;
	IOWR_ALTERA_AVALON_PIO_DATA(PIO_5_BASE, led_row);
	printf("Wrote page %d\n", pagenum);

#ifdef FLASHDEBUG
	if (pagenum == 256)
		return -1;
#endif

	return retval;
}

int check_sdcard() {
	sdcard_dev = alt_up_sd_card_open_dev(ALTERA_UP_SD_CARD_AVALON_INTERFACE_0_NAME);

	if ((sdcard_dev == NULL) || !alt_up_sd_card_is_Present() || !alt_up_sd_card_is_FAT16())
		return 0;

	sd_fw_handle = alt_up_sd_card_fopen(FW_FILE, 0);

	if (sd_fw_handle == -1)
		return 0;

	return 1;
}

int check_fw_update() {
	short int read_val;
	int retval;
	alt_u32 bytes_read, tot_bytes_read, pagenum;
	alt_u8 pagebuf[PAGESIZE];
	alt_u32 btn_vec;

	retval = 0;

	if (!check_sdcard()) {
		printf("No FW update available\n\n");
		return 1;
	}

	if (!check_flash()) {
		printf("Flash access error\n\n");
		retval = -6;
		strncpy(menu_row1, "Flash access", LCD_ROW_LEN+1);
		strncpy(menu_row2, "error", LCD_ROW_LEN+1);
		lcd_write_menu();
		usleep(1000000);
		goto close_fd;
	}

	printf("FW found on SD card. Update?\nY=KEY1, N=KEY2\n");
	strncpy(menu_row1, "Update FW?", LCD_ROW_LEN+1);
	strncpy(menu_row2, "Y=KEY1, N=KEY2", LCD_ROW_LEN+1);
	lcd_write_menu();

	while (1) {
		btn_vec = ~IORD_ALTERA_AVALON_PIO_DATA(PIO_1_BASE) & AV_SEL_MASK;

		if (btn_vec & BTN3_BIT)
			break;
		else if (btn_vec & BTN2_BIT)
			goto close_fd;

		usleep(20000);
	}

	strncpy(menu_row1, "Updating FW", LCD_ROW_LEN+1);
	strncpy(menu_row2, "please wait...", LCD_ROW_LEN+1);
	lcd_write_menu();

	bytes_read = 0;
	pagenum = 0;

	while (1) {
		read_val = alt_up_sd_card_read(sd_fw_handle);
		if (read_val < 0)
			break;

		pagebuf[bytes_read] = reverse_bits((alt_u8)read_val);
		bytes_read++;

		if (bytes_read == 256) {
			if (write_flash_page(pagebuf, pagenum) != 0)
				goto close_fd;
			pagenum++;
			bytes_read = 0;
		}
	}

	if (bytes_read != 0) {
		if (write_flash_page(pagebuf, pagenum) != 0)
			goto close_fd;
	}

	tot_bytes_read = 256*pagenum + bytes_read;

	printf("%u bytes written\n", tot_bytes_read);

close_fd:
	alt_up_sd_card_fclose(sd_fw_handle);

	return retval;
}

void display_menu(alt_u8 forcedisp) {
	menucode_id code;
	int fw_upd_stat;

	if (remote_code_prev == 0) {
		switch (remote_code) {
			case BTN_CHPLUS_CODE:
				code = PREV_PAGE;
			  break;
			case BTN_CHMINUS_CODE:
				code = NEXT_PAGE;
			  break;
			case BTN_VOLPLUS_CODE:
				code = VAL_PLUS;
			  break;
			case BTN_VOLMINUS_CODE:
				code = VAL_MINUS;
			  break;
			default:
				code = NO_ACTION;
			  break;
		}
	} else {
		code = NO_ACTION;
	}

	if (!forcedisp && (code == NO_ACTION))
		return;

	if (code == PREV_PAGE)
		menu_page = (menu_page+MENUITEMS-1) % MENUITEMS;
	else if (code == NEXT_PAGE)
		menu_page = (menu_page+1) % MENUITEMS;

	strncpy(menu_row1, menu[menu_page].desc, LCD_ROW_LEN+1);

	switch (menu[menu_page].id) {
		case TX_MODE:
			if ((code == VAL_MINUS) || (code == VAL_PLUS))
				tc.tx_mode = !tc.tx_mode;
			sniprintf(menu_row2, LCD_ROW_LEN+1, tc.tx_mode ? "DVI" : "HDMI");
		  break;
		case SCANLINE_MODE:
			if ((code == VAL_MINUS) && (tc.sl_mode > 0))
				tc.sl_mode--;
			else if ((code == VAL_PLUS) && (tc.sl_mode < SL_MODE_MAX))
				tc.sl_mode++;
			strncpy(menu_row2, sl_mode_desc[tc.sl_mode], LCD_ROW_LEN+1);
		  break;
		case SCANLINE_STRENGTH:
			if ((code == VAL_MINUS) && (tc.sl_str > 0))
				tc.sl_str--;
			else if ((code == VAL_PLUS) && (tc.sl_str < SCANLINESTR_MAX))
				tc.sl_str++;
			sniprintf(menu_row2, LCD_ROW_LEN+1, "%u%%", ((tc.sl_str+1)*125)/10);
		  break;
		case SCANLINE_ID:
			if ((code == VAL_MINUS) || (code == VAL_PLUS))
				tc.sl_id = !tc.sl_id;
			sniprintf(menu_row2, LCD_ROW_LEN+1, tc.sl_id ? "Odd" : "Even");
		  break;
		case H_MASK:
			if ((code == VAL_MINUS) && (tc.h_mask > 0))
				tc.h_mask--;
			else if ((code == VAL_PLUS) && (tc.h_mask < HV_MASK_MAX))
				tc.h_mask++;
			sniprintf(menu_row2, LCD_ROW_LEN+1, "%u pixels", tc.h_mask<<2);
		  break;
		case V_MASK:
			if ((code == VAL_MINUS) && (tc.v_mask > 0))
				tc.v_mask--;
			else if ((code == VAL_PLUS) && (tc.v_mask < HV_MASK_MAX))
				tc.v_mask++;
			sniprintf(menu_row2, LCD_ROW_LEN+1, "%u pixels", tc.v_mask<<2);
		  break;
		case SAMPLER_480P:
			if ((code == VAL_MINUS) && (tc.s480p_mode > 0))
				tc.s480p_mode--;
			else if ((code == VAL_PLUS) && (tc.s480p_mode < S480P_MODE_MAX))
				tc.s480p_mode++;
			strncpy(menu_row2, s480p_desc[tc.s480p_mode], LCD_ROW_LEN+1);
		  break;
		case SAMPLER_PHASE:
			if ((code == VAL_MINUS) && (tc.sampler_phase > SAMPLER_PHASE_MIN))
				tc.sampler_phase--;
			else if ((code == VAL_PLUS) && (tc.sampler_phase < SAMPLER_PHASE_MAX))
				tc.sampler_phase++;
			sniprintf(menu_row2, LCD_ROW_LEN+1, "%d deg", ((tc.sampler_phase-SAMPLER_PHASE_MIN)*1125)/100);
		  break;
		case YPBPR_COLORSPACE:
			if ((code == VAL_MINUS) || (code == VAL_PLUS))
				tc.ypbpr_cs = !tc.ypbpr_cs;
			strncpy(menu_row2, csc_coeffs[tc.ypbpr_cs].name, LCD_ROW_LEN+1);
		  break;
		case LPF:
			if ((code == VAL_MINUS) || (code == VAL_PLUS))
				tc.lpf = !tc.lpf;
			sniprintf(menu_row2, LCD_ROW_LEN+1, tc.lpf ? "Max" : "Min");
		  break;
		case SYNC_LPF:
			if ((code == VAL_MINUS) && (tc.sync_lpf > 0))
				tc.sync_lpf--;
			else if ((code == VAL_PLUS) && (tc.sync_lpf < SYNC_LPF_MAX))
				tc.sync_lpf++;
			strncpy(menu_row2, sync_lpf_desc[tc.sync_lpf], LCD_ROW_LEN+1);
			break;
		case LINETRIPLE_ENABLE:
			if ((code == VAL_MINUS) || (code == VAL_PLUS))
				tc.linemult_target = !tc.linemult_target;
			sniprintf(menu_row2, LCD_ROW_LEN+1, tc.linemult_target ? "On" : "Off");
		  break;
		case LINETRIPLE_MODE:
			if ((code == VAL_MINUS) && (tc.l3_mode > 0))
				tc.l3_mode--;
			else if ((code == VAL_PLUS) && (tc.l3_mode < L3_MODE_MAX))
				tc.l3_mode++;
			strncpy(menu_row2, l3_mode_desc[tc.l3_mode], LCD_ROW_LEN+1);
		  break;
#ifndef DEBUG
		case FW_UPDATE:
			if ((code == VAL_MINUS) || (code == VAL_PLUS)) {
				fw_upd_stat = check_fw_update();
				if (fw_upd_stat == 0) {
					sniprintf(menu_row1, LCD_ROW_LEN+1, "Fw update OK");
					sniprintf(menu_row2, LCD_ROW_LEN+1, "Please restart");
				} else if (fw_upd_stat > 0) {
					sniprintf(menu_row1, LCD_ROW_LEN+1, "FW not found");
					sniprintf(menu_row2, LCD_ROW_LEN+1, "on SD card");
				} else {
					sniprintf(menu_row1, LCD_ROW_LEN+1, "FW update error");
					sniprintf(menu_row2, LCD_ROW_LEN+1, "Code %d", fw_upd_stat);
				}
			} else {
				sniprintf(menu_row2, LCD_ROW_LEN+1,  "press <- or ->");
			}
		  break;
#endif
		default:
			sniprintf(menu_row2, LCD_ROW_LEN+1, "MISSING ITEM");
		  break;
	}

	lcd_write_menu();

	return;
}

void read_control() {
	alt_u32 cword;

	cword = IORD_ALTERA_AVALON_PIO_DATA(PIO_1_BASE);

	cm.remote_ctrl = !!(cword & REMOTE_ENABLE_BIT);

	if (!cm.remote_ctrl) {
#ifdef HDMITX
		tc.tx_mode = !!(cword & TX_MODE_BIT);
#endif
		tc.linemult_target = !!(cword & LINETRIPLE_ENABLE_BIT);
		tc.l3_mode = ((cword >> LINETRIPLE_MODE_LSB) & LINETRIPLE_MODE_MASK);
		tc.sl_mode = !!(cword & SCANLINES_ENABLE_BIT);
		tc.sl_str = ((cword >> SCANLINESTR_LSB) & SCANLINESTR_MASK);
		tc.sl_id = 0;
		tc.h_mask = ((cword >> H_MASK_LSB) & H_MASK_MASK);
		tc.v_mask = ((cword >> V_MASK_LSB) & V_MASK_MASK);
		return;
	}

	if (remote_code_prev == 0) {
		if (remote_code == BTN_RECALL_CODE) {
			menu_active = !menu_active;

			if (menu_active) {
				display_menu(1);
			} else {
				lcd_write_status();
			}
		}
	}

	if (menu_active) {
		display_menu(0);
		return;
	}

	if (remote_code_prev != 0)
		return;

	switch (remote_code) {
		case BTN_MUTE_CODE:
			tc.sl_mode = !tc.sl_mode;
		  break;
		case BTN_TSHIFT_CODE:
			if (tc.sl_str < SCANLINESTR_MAX)
				tc.sl_str++;
		  break;
		case BTN_RECORD_CODE:
			if (tc.sl_str > 0)
				tc.sl_str--;
		  break;
		case BTN_VOLPLUS_CODE:
			if (tc.h_mask < HV_MASK_MAX)
				tc.h_mask++;
		  break;
		case BTN_VOLMINUS_CODE:
			if (tc.h_mask > 0)
				tc.h_mask--;
		  break;
		case BTN_CHPLUS_CODE:
			if (tc.v_mask < HV_MASK_MAX)
				tc.v_mask++;
		  break;
		case BTN_CHMINUS_CODE:
			if (tc.v_mask > 0)
				tc.v_mask--;
		  break;
		default:
		  // Code
		  break;
	}
}

// Check if input video status / target configuration has changed
status_t get_status(tvp_input_t input) {
	alt_u32 data1, data2;
	alt_u32 totlines, clkcnt;
	alt_u8 macrovis, progressive;
	alt_u8 refclk;
	alt_u8 sync_active;
	status_t status;

	status = NO_CHANGE;

	sync_active = tvp_check_sync(input);

	if (cm.sync_active != sync_active)
		status = ACTIVITY_CHANGE;

	cm.sync_active = sync_active;

	data1 = tvp_readreg(TVP_LINECNT1);
	data2 = tvp_readreg(TVP_LINECNT2);
	totlines = ((data2 & 0x0f) << 8) | data1;
	progressive = !!(data2 & (1<<5));
	cm.macrovis = !!(data2 & (1<<6));

	data1 = tvp_readreg(TVP_CLKCNT1);
	data2 = tvp_readreg(TVP_CLKCNT2);
	clkcnt = ((data2 & 0x0f) << 8) | data1;

	//Not fully implemented yet
	/*refclk = !!(cword & TVP_CLKSEL_BIT);
	refclk = 0;

	if (refclk != cm.refclk)
		status = (status < REFCLK_CHANGE) ? REFCLK_CHANGE : status;*/

	if (tc.tx_mode != cm.cc.tx_mode)
		status = (status < TX_MODE_CHANGE) ? TX_MODE_CHANGE : status;

	// TODO: avoid random sync losses?
	if ((abs((alt_16)totlines - (alt_16)cm.totlines) > LINECNT_THOLD) || (clkcnt != cm.clkcnt) || (progressive != cm.progressive)) {
		stable_frames = 0;
	} else if (stable_frames != STABLE_THOLD) {
		stable_frames++;
		if (stable_frames == STABLE_THOLD)
			status = (status < MODE_CHANGE) ? MODE_CHANGE : status;
	}

	if ((tc.linemult_target != cm.cc.linemult_target) || (tc.l3_mode != cm.cc.l3_mode))
		status = (status < MODE_CHANGE) ? MODE_CHANGE : status;

	if ((tc.s480p_mode != cm.cc.s480p_mode) && (video_modes[cm.id].flags & (MODE_DTV480P|MODE_VGA480P)))
		status = (status < MODE_CHANGE) ? MODE_CHANGE : status;

	cm.totlines = totlines;
	cm.clkcnt = clkcnt;
	cm.progressive = progressive;

	if ((tc.sl_mode != cm.cc.sl_mode) ||
		(tc.sl_str != cm.cc.sl_str) ||
		(tc.sl_id != cm.cc.sl_id) ||
		(tc.h_mask != cm.cc.h_mask) ||
		(tc.v_mask != cm.cc.v_mask))
		status = (status < INFO_CHANGE) ? INFO_CHANGE : status;

	if (tc.sampler_phase != cm.cc.sampler_phase)
		tvp_set_hpll_phase(tc.sampler_phase-SAMPLER_PHASE_MIN);

	if (tc.ypbpr_cs != cm.cc.ypbpr_cs)
		tvp_sel_csc(&csc_coeffs[tc.ypbpr_cs]);

	if (tc.lpf != cm.cc.lpf)
		tvp_set_lpf(tc.lpf ? 0x0F : 0);

	if (tc.sync_lpf != cm.cc.sync_lpf)
		tvp_set_sync_lpf(tc.sync_lpf);

	// use memcpy instead?
	cm.cc = tc;

	return status;
}

// h_info:     [31:30]          [29:28]      [27]      [26:16]      [15:12]      [11:8]             [7:0]
// 		  | H_LINEMULT[1:0] | H_L3MODE[1:0] |    | H_ACTIVE[10:0] |         | H_MASK[3:0] |  H_BACKPORCH[7:0] |
//
// v_info:     [31]         [29]                                         [26:24]             [23:13]       [15:10]       [9:6]         [5:0]
// 		  | V_MISMODE | V_SCANLINES | V_SCANLINEDIR | V_SCANLINEID | V_SCANLINESTR[2:0] | V_ACTIVE[10:0] |         | V_MASK[3:0]|  V_BACKPORCH[5:0] |
void set_videoinfo() {
	alt_u8 slid_target;

	if (video_modes[cm.id].flags & MODE_L3ENABLE) {
		cm.linemult = 2;
		slid_target = cm.cc.sl_id ? 2 : 0;
	} else if (video_modes[cm.id].flags & MODE_L2ENABLE) {
		cm.linemult = 1;
		slid_target = cm.cc.sl_id;
	} else {
		cm.linemult = 0;
		slid_target = cm.cc.sl_id;
	}

    IOWR_ALTERA_AVALON_PIO_DATA(PIO_2_BASE, (cm.linemult<<30) | (cm.cc.l3_mode<<28) | (video_modes[cm.id].h_active<<16) | (cm.cc.h_mask)<<8 | video_modes[cm.id].h_backporch);
	IOWR_ALTERA_AVALON_PIO_DATA(PIO_3_BASE, (!!(video_modes[cm.id].flags & MODE_MALFORMED)<<31) | ((!!cm.cc.sl_mode)<<29) | (cm.cc.sl_mode > 0 ? (cm.cc.sl_mode-1)<<28 : 0) | (slid_target<<27) | (cm.cc.sl_str<<24) | (video_modes[cm.id].v_active<<13) | (cm.cc.v_mask<<6) | video_modes[cm.id].v_backporch);
}

// Update onboard 7-segment display
void set_7seg() {
	alt_u32 led_row;
	alt_u32 lines;
	alt_u8 digit, leadzero;
	int i, j;

	if (!cm.sync_active) {
		IOWR_ALTERA_AVALON_PIO_DATA(PIO_5_BASE, 0xffb05dbc); //no sync
		return;
	}

	lines = IORD_ALTERA_AVALON_PIO_DATA(PIO_4_BASE) & 0x7ff;

	led_row = 0;

	leadzero = 1;
	j=3;

	//output line count
	for (i=1000; i>=1; i/=10) {
		digit = (lines%(i*10))/i;

		if (leadzero && !digit) {
			led_row |= 0xf<<(4*j);
		} else {
			led_row |= digit<<(4*j);
			leadzero = 0;
		}
		j--;
	}

	//L0, L2 or L3
	led_row |= (0x00e00000 | ((cm.linemult+1)<<16));

	//L3 mode
	led_row |= (cm.cc.l3_mode<<24);

	//Scanline sign
	led_row |= cm.cc.sl_mode ? 0xa0000000 : 0xf0000000;

	//led_row = lines;
	IOWR_ALTERA_AVALON_PIO_DATA(PIO_5_BASE, led_row);
}

// Configure TVP7002 and scan converter logic based on the video mode
void program_mode() {
	alt_u32 data1, data2;
	alt_u32 h_hz, v_hz_x10;

	// Mark as stable (needed after sync up to avoid unnecessary mode switch)
	stable_frames = STABLE_THOLD;

	if ((cm.clkcnt != 0) && (cm.totlines != 0)) { //prevent div by 0
		h_hz = clkrate[cm.refclk]/cm.clkcnt;
		v_hz_x10 = cm.progressive ? (10*clkrate[cm.refclk]/cm.clkcnt)/cm.totlines : (20*clkrate[cm.refclk]/cm.clkcnt)/cm.totlines;
	} else {
		h_hz = 15700;
		v_hz_x10 = 600;
	}

	printf("\nLines: %u %c\n", cm.totlines, cm.progressive ? 'p' : 'i');
	printf("Clocks per line: %u : HS %u.%u kHz  VS %u.%u Hz\n", cm.clkcnt, h_hz/1000, h_hz%1000, v_hz_x10/10, v_hz_x10%10);

	data1 = tvp_readreg(TVP_HSINWIDTH);
	data2 = tvp_readreg(TVP_VSINWIDTH);
	printf("Hswidth: %u  Vswidth: %u  Macrovision: %u\n", data1, data2 & 0x1F, cm.macrovis);

	//TODO: rewrite with strncpy to reduce code size
	sniprintf(row1, LCD_ROW_LEN+1, "%s %u%c", avinput_str[cm.avinput], cm.totlines, cm.progressive ? 'p' : 'i');
	sniprintf(row2, LCD_ROW_LEN+1, "%u.%ukHz  %u.%uHz", h_hz/1000, (h_hz%1000)/100, v_hz_x10/10, v_hz_x10%10);
	//strncpy(row1, avinput_str[cm.avinput], LCD_ROW_LEN+1);
	//strncpy(row2, avinput_str[cm.avinput], LCD_ROW_LEN+1);
	if (!menu_active)
		lcd_write_status();

	//printf ("Get mode id with %u %u %f\n", totlines, progressive, hz);
	cm.id = get_mode_id(cm.totlines, cm.progressive, v_hz_x10/10, target_typemask, cm.cc.linemult_target, cm.cc.l3_mode, cm.cc.s480p_mode);

	if ( cm.id == -1) {
		printf ("Error: no suitable mode found, defaulting to first\n");
		cm.id=0;
	}

	printf("Mode %s selected\n", video_modes[cm.id].name);

	printf("Frame: %u\n", frameno);

	tvp_source_setup(cm.id, (cm.progressive ? cm.totlines : cm.totlines/2), v_hz_x10/10, cm.refclk);
	set_videoinfo();
}

// Initialize hardware
int init_hw() {
	alt_u32 chiprev;

	// Reset error vector and scan converter
	//IOWR_ALTERA_AVALON_PIO_DATA(PIO_0_BASE, 0x00);
	IOWR_ALTERA_AVALON_PIO_DATA(PIO_2_BASE, 0x00000000);
	IOWR_ALTERA_AVALON_PIO_DATA(PIO_3_BASE, 0x00000000);

	// Reset 7seg row
	IOWR_ALTERA_AVALON_PIO_DATA(PIO_5_BASE, 0xffffffff);

	//wait >500ms for SD card interface to be stable
	usleep(200000);

	// IT6613 supports only 100kHz
	I2C_init(I2CA_BASE,ALT_CPU_FREQ,100000);

	/* Initialize the character display */
	lcd_init();

	/* check if TVP is found */
	chiprev = tvp_readreg(TVP_CHIPREV);
	//printf("chiprev %d\n", chiprev);

	if ( chiprev == 0xff) {
		printf("Error: could not read from TVP7002\n");
		return -5;
	}

	tvp_init();

#ifdef HDMITX
	I2C_init(I2CA_HDMI_BASE,ALT_CPU_FREQ,100000);

	chiprev = read_it2(IT_DEVICEID);

	if ( chiprev != 0x13) {
		printf("Error: could not read from IT6613\n");
		return -5;
	}

	InitIT6613();
#endif

	return 0;
}

#ifdef HDMITX
inline void TX_enable(tx_mode_t mode) {
	//SetAVMute(TRUE);
	if (mode == TX_HDMI) {
		EnableVideoOutput(PCLK_MEDIUM, COLOR_RGB444, COLOR_RGB444, 1);
		HDMITX_SetAVIInfoFrame(1, F_MODE_RGB444, 0, 0);
	} else {
		EnableVideoOutput(PCLK_MEDIUM, COLOR_RGB444, COLOR_RGB444, 0);
	}
	SetAVMute(FALSE);
}
#endif

// Enable chip outputs
void enable_outputs() {
	// program video mode
	program_mode();
	// enable TVP output
	tvp_enable_output();

#ifdef HDMITX
	// enable and unmute HDMITX
	// TODO: check pclk
	TX_enable(cm.cc.tx_mode);
#endif
}

int main()
{
	alt_u8 av_sel_vec, av_sel_vec_prev, target_input, target_format;
	avinput_t target_mode;
	alt_u8 av_init = 0;
	status_t status;

	int init_stat;

	init_stat = init_hw();

	if (init_stat >= 0) {
		printf("### DIY VIDEO DIGITIZER / SCANCONVERTER INIT OK ###\n\n");
		IOWR_ALTERA_AVALON_PIO_DATA(PIO_5_BASE, 0xffff055C); //OSSC
		strncpy(row1, "DE2-vd  fw. "FW_VER, LCD_ROW_LEN+1);
		strncpy(row2, "2014-2015  marqs", LCD_ROW_LEN+1);
		lcd_write_status();
	} else {
		sniprintf(row1, LCD_ROW_LEN+1, "Init error  %d", init_stat);
		strncpy(row2, "", LCD_ROW_LEN+1);
		lcd_write_status();
		while (1) {}
	}

//write default TX as DVI (effective when operated via remote)
#ifdef HDMITX
#ifdef DVI_INIT
	cm.cc.tx_mode = TX_DVI;
	tc.tx_mode = TX_DVI;
#endif
#endif

	while(1) {
		// Select target input and mode
		av_sel_vec = ~IORD_ALTERA_AVALON_PIO_DATA(PIO_1_BASE) & AV_SEL_MASK;
		remote_code = IORD_ALTERA_AVALON_PIO_DATA(PIO_0_BASE);
		if (av_sel_vec_prev == 0) {
			if (av_sel_vec & BTN3_BIT) {
				target_mode = AV1_RGBs;
			} else if (av_sel_vec & BTN2_BIT) {
				if (cm.avinput == AV2_RGBHV)
					target_mode = AV2_RGBs;
				else
					target_mode = AV2_RGBHV;
			} else if (av_sel_vec & BTN1_BIT) {
				if (cm.avinput == AV3_YPBPR)
					target_mode = AV3_RGsB;
				else
					target_mode = AV3_YPBPR;
			} else {
				target_mode = AV_KEEP;
			}
		} else {
			target_mode = AV_KEEP;
		}
		av_sel_vec_prev = av_sel_vec;

		if (target_mode == cm.avinput)
			target_mode = AV_KEEP;

		if (target_mode != AV_KEEP)
			printf("### SWITCH MODE TO %s ###\n", avinput_str[target_mode]);

		switch (target_mode) {
			case AV1_RGBs:
				target_input = TVP_INPUT1;
				target_format = FORMAT_RGBS;
				target_typemask = VIDEO_LDTV|VIDEO_SDTV|VIDEO_HDTV;
			  break;
			case AV2_RGBHV:
				target_input = TVP_INPUT2;
				target_format = FORMAT_RGBHV;
				target_typemask = VIDEO_PC;
			  break;
			case AV2_RGBs:
				target_input = TVP_INPUT2;
				target_format = FORMAT_RGBS;
				target_typemask = VIDEO_LDTV|VIDEO_SDTV|VIDEO_HDTV;
			  break;
			case AV3_YPBPR:
				target_input = TVP_INPUT3;
				target_format = FORMAT_YPbPr;
				target_typemask = VIDEO_LDTV|VIDEO_SDTV|VIDEO_HDTV;
			  break;
			case AV3_RGsB:
				target_input = TVP_INPUT3;
				target_format = FORMAT_RGsB;
				target_typemask = VIDEO_LDTV|VIDEO_SDTV|VIDEO_HDTV;
			  break;
			default:
			  // Code
			  break;
		}

		if (target_mode != AV_KEEP) {
			av_init = 1;
			cm.avinput = target_mode;
			cm.sync_active = 0;
			tvp_disable_output();
			tvp_source_sel(target_input, target_format, cm.refclk);
			cm.clkcnt = 0; //TODO: proper invalidate
			strncpy(row1, avinput_str[cm.avinput], LCD_ROW_LEN+1);
			strncpy(row2, "    NO SYNC", LCD_ROW_LEN+1);
			if (!menu_active)
				lcd_write_status();
		}

		usleep(20000);
		read_control();
		frameno++;

		if (av_init) {
			status = get_status(target_input);

			switch (status) {
				case ACTIVITY_CHANGE:
					if (cm.sync_active) {
						printf("Sync up\n");
						enable_outputs();
					} else {
						printf("Sync lost\n");
						cm.clkcnt = 0; //TODO: proper invalidate
						tvp_disable_output();
						strncpy(row1, avinput_str[cm.avinput], LCD_ROW_LEN+1);
						strncpy(row2, "    NO SYNC", LCD_ROW_LEN+1);
						if (!menu_active)
							lcd_write_status();
					}
				  break;
#ifdef HDMITX
				case TX_MODE_CHANGE:
					if (cm.sync_active)
						TX_enable(cm.cc.tx_mode);
					printf("TX mode change\n");
				  break;
#endif
				/*case REFCLK_CHANGE:
					if (cm.sync_active) {
						printf("Refclk change\n");
						tvp_sel_clk(cm.refclk);
					}
				  break;*/
				case MODE_CHANGE:
					if (cm.sync_active) {
						printf("Mode change\n");
						program_mode();
					}
				  break;
				case INFO_CHANGE:
					if (cm.sync_active) {
						printf("Info change\n");
						set_videoinfo();
					}
				  break;
				default:
				  break;
			}

			set_7seg();
		}

		remote_code_prev = remote_code;
	}

	return 0;
}
