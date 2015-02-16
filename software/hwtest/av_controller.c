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
#include "altera_up_avalon_character_lcd.h"
#include "i2c_opencores.h"
#include "tvp7002.h"
#include "video_modes.h"

#ifdef HDMITX
#include "it6613.h"
#include "hdmitx.h"
#endif

// AV control word:
//    [20:17]         [16:13]         [12:10]            [9:8]          [7]      [6]        [5:4]     [3]     [2]    [1]    [0]
// | H_MASK[3:0] | V_MASK[3:0] | SCANLINESTR[2:0] | RESERVED[1:0] | TVP_CLKSEL | SLEN | L3MODE[1:0] | L3EN || BTN3 | BTN2 | BTN1 |

#define H_MASK_MASK		 		0x0f
#define H_MASK_LSB 				17

#define V_MASK_MASK		 		0x0f
#define V_MASK_LSB 				13

#define SCANLINESTR_MASK 		0x07
#define SCANLINESTR_LSB 		10

#define TVP_CLKSEL_BIT	 		(1<<7)

#define SCANLINES_ENABLE_BIT 	(1<<6)

#define LINETRIPLE_MODE_MASK	0x03
#define LINETRIPLE_MODE_LSB		4

#define LINETRIPLE_ENABLE_BIT 	(1<<3)

#define AV_SEL_MASK 			0x07
#define BTN3_BIT 				(1<<2)
#define BTN2_BIT 				(1<<1)
#define BTN1_BIT				(1<<0)

typedef enum {
   AV_KEEP,
   AV1_RGBs,
   AV2_RGBHV,
   AV2_RGBs,
   AV3_YPBPR,
   AV3_RGsB
} avinput_t;

static const char *avinput_str[] = { "NO MODE", "AV1: RGBS", "AV2: RGBHV", "AV2: RGBS", "AV3: YPbPr", "AV3: RGsB" };

#define STABLE_THOLD 1

// In reverse order of importance
typedef enum {
	NO_CHANGE			= 0,
	INFO_CHANGE			= 1,
	MODE_CHANGE			= 2,
	REFCLK_CHANGE		= 3,
	ACTIVITY_CHANGE		= 4
} status_t;

//TODO: transform binary values into flags
typedef struct {
	alt_u32 totlines;
    alt_u32 clkcnt;
    alt_u8 progressive;
    alt_u8 macrovis;
    alt_u8 refclk;
    alt_8 id;
    alt_u8 sync_active;
    alt_u8 scanlines_enable;
	alt_u8 linemult_target;
	alt_u8 linemult;
	alt_u8 l3_mode;
	alt_u8 h_mask;
	alt_u8 v_mask;
	alt_u8 scanlinestr;
    avinput_t avinput;
} avmode_t;

avmode_t cm;

alt_up_character_lcd_dev * char_lcd_dev;

alt_u8 target_typemask;
alt_u8 stable_frames;

// Check if input video status / target configuration has changed
status_t get_status(tvp_input_t input) {
	alt_u32 data1, data2;
	alt_u32 totlines, clkcnt;
	alt_u8 macrovis, progressive;
	alt_u8 scanlines_enable;
	alt_u8 linemult_target;
	alt_u8 l3_mode;
	alt_u8 refclk;
	alt_u8 sync_active;
	alt_u8 h_mask;
	alt_u8 v_mask;
	alt_u8 scanlinestr;
	alt_32 cword;
	status_t status;

	status = NO_CHANGE;

	cword = IORD_ALTERA_AVALON_PIO_DATA(PIO_1_BASE);

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
	//refclk = !!(cword & TVP_CLKSEL_BIT);
	refclk = 0;

	if (refclk != cm.refclk)
		status = (status < REFCLK_CHANGE) ? REFCLK_CHANGE : status;

	cm.refclk = refclk;

	// TODO: avoid random sync losses?
	if ((totlines != cm.totlines) || (clkcnt != cm.clkcnt) || (progressive != cm.progressive)) {
		stable_frames = 0;
	} else if (stable_frames != STABLE_THOLD) {
		stable_frames++;
		if (stable_frames == STABLE_THOLD)
			status = (status < MODE_CHANGE) ? MODE_CHANGE : status;
	}

	linemult_target = !!(cword & LINETRIPLE_ENABLE_BIT);
	l3_mode = (cword >> LINETRIPLE_MODE_LSB) & LINETRIPLE_MODE_MASK;

	if ((linemult_target != cm.linemult_target) || (l3_mode != cm.l3_mode))
		status = (status < MODE_CHANGE) ? MODE_CHANGE : status;

	cm.linemult_target = linemult_target;
	cm.l3_mode = l3_mode;

	cm.totlines = totlines;
	cm.clkcnt = clkcnt;
	cm.progressive = progressive;

	scanlines_enable = !!(cword & SCANLINES_ENABLE_BIT);
	scanlinestr = (cword >> SCANLINESTR_LSB) & SCANLINESTR_MASK;
	h_mask = (cword >> H_MASK_LSB) & H_MASK_MASK;
	v_mask = (cword >> V_MASK_LSB) & V_MASK_MASK;

	if ((scanlines_enable != cm.scanlines_enable) ||
		(scanlinestr != cm.scanlinestr) ||
		(h_mask != cm.h_mask) ||
		(v_mask != cm.v_mask))
		status = (status < INFO_CHANGE) ? INFO_CHANGE : status;

	cm.scanlines_enable = scanlines_enable;
	cm.scanlinestr = scanlinestr;
	cm.h_mask = h_mask;
	cm.v_mask = v_mask;

	return status;
}

// h_info:     [31:30]          [29:28]      [27]      [26:16]      [15:12]      [11:8]             [7:0]
// 		  | H_LINEMULT[1:0] | H_L3MODE[1:0] |    | H_ACTIVE[10:0] |         | H_MASK[3:0] |  H_BACKPORCH[7:0] |
//
// v_info:     [31]         [30]           [29:27]             [26:16]       [15:10]       [9:6]         [5:0]
// 		  | V_MISMODE | V_SCANLINES | V_SCANLINESTR[2:0] | V_ACTIVE[10:0] |         | V_MASK[3:0]|  V_BACKPORCH[5:0] |
void set_videoinfo() {
	if (video_modes[cm.id].flags & MODE_L3ENABLE)
		cm.linemult = 3;
	else if (video_modes[cm.id].flags & MODE_L2ENABLE)
		cm.linemult = 2;
	else
		cm.linemult = 0;

	IOWR_ALTERA_AVALON_PIO_DATA(PIO_2_BASE, (cm.linemult<<30) | (cm.l3_mode<<28) | (video_modes[cm.id].h_active<<16) | (cm.h_mask)<<8 | video_modes[cm.id].h_backporch);
	IOWR_ALTERA_AVALON_PIO_DATA(PIO_3_BASE, (!!(video_modes[cm.id].flags & MODE_MALFORMED)<<31) | (cm.scanlines_enable<<30) | (cm.scanlinestr<<27) | (video_modes[cm.id].v_active<<16) | (cm.v_mask<<6) | video_modes[cm.id].v_backporch);
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
	led_row |= (0x00e00000 | (cm.linemult<<16));

	//L3 mode
	led_row |= (cm.l3_mode<<24);

	//Scanline sign
	led_row |= cm.scanlines_enable ? 0xa0000000 : 0xf0000000;

	//led_row = lines;
	IOWR_ALTERA_AVALON_PIO_DATA(PIO_5_BASE, led_row);
}

// Configure TVP7002 and scan converter logic based on the video mode
void program_mode() {
	alt_u32 data1, data2;
	char row1[20], row2[20];
	float hz;

	// Mark as stable (needed after sync up to avoid unnecessary mode switch)
	stable_frames = STABLE_THOLD;

	if ((cm.clkcnt != 0) && (cm.totlines != 0)) { //prevent div by 0
		printf("\nLines: %u %c\n", cm.totlines, cm.progressive ? 'p' : 'i');
		printf("Clocks per line: %u : HS %.2f kHz  VS %.2f Hz\n", cm.clkcnt, (0.001f*clkrate[cm.refclk])/cm.clkcnt, cm.progressive ? (clkrate[cm.refclk]/cm.clkcnt)/cm.totlines : (2*clkrate[cm.refclk]/cm.clkcnt)/cm.totlines);

		data1 = tvp_readreg(TVP_HSINWIDTH);
		data2 = tvp_readreg(TVP_VSINWIDTH);
		printf("Hswidth: %u  Vswidth: %u  Macrovision: %u\n", data1, data2 & 0x1F, cm.macrovis);

		snprintf(row1, 17, "%s %u%c   ", avinput_str[cm.avinput], cm.totlines, cm.progressive ? 'p' : 'i');
		snprintf(row2, 17, "%.1fkHz  %.1fHz   ", (0.001f*clkrate[cm.refclk])/cm.clkcnt, cm.progressive ? (clkrate[cm.refclk]/cm.clkcnt)/cm.totlines : (2*clkrate[cm.refclk]/cm.clkcnt)/cm.totlines);
		alt_up_character_lcd_set_cursor_pos(char_lcd_dev, 0, 0);
		alt_up_character_lcd_string(char_lcd_dev, row1);
		alt_up_character_lcd_set_cursor_pos(char_lcd_dev, 0, 1);
		alt_up_character_lcd_string(char_lcd_dev, row2);
	}

	if ((cm.clkcnt != 0) && (cm.totlines != 0))
		hz = cm.progressive ? (clkrate[cm.refclk]/cm.clkcnt)/cm.totlines : (2*clkrate[cm.refclk]/cm.clkcnt)/cm.totlines;
	else
		hz = 60.0f;

	//printf ("Get mode id with %u %u %f\n", totlines, progressive, hz);
	cm.id = get_mode_id(cm.totlines, cm.progressive, hz, target_typemask, cm.linemult_target, cm.l3_mode);

	if ( cm.id == -1) {
		printf ("Error: no suitable mode found, defaulting to first\n");
		cm.id=0;
	}

	printf("Mode %s selected\n", video_modes[cm.id].name);

	tvp_source_setup(cm.id, (cm.progressive ? cm.totlines : cm.totlines/2), hz, cm.refclk);
	set_videoinfo();
}

// Initialize hardware
int init_hw() {
	alt_u32 chiprev;

	// Open the Character LCD port
	char_lcd_dev = alt_up_character_lcd_open_dev ("/dev/character_lcd_0");
	if ( char_lcd_dev == NULL) {
		printf ("Error: could not open character LCD device\n");
		return -1;
	}

	/* Initialize the character display */
	alt_up_character_lcd_init (char_lcd_dev);
	alt_up_character_lcd_cursor_off(char_lcd_dev);

	// Reset error vector
	IOWR_ALTERA_AVALON_PIO_DATA(PIO_0_BASE, 0x00);

	// Reset 7seg row
	IOWR_ALTERA_AVALON_PIO_DATA(PIO_5_BASE, 0xffffffff);

	// Reset scan converter
	IOWR_ALTERA_AVALON_PIO_DATA(PIO_2_BASE, 0x00000000);
	IOWR_ALTERA_AVALON_PIO_DATA(PIO_3_BASE, 0x00000000);

	I2C_init(I2CA_BASE,ALT_CPU_FREQ,100000);
	chiprev = tvp_readreg(TVP_CHIPREV);

	if ( chiprev == 0xff) {
		printf ("Error: could not read from TVP7002\n");
		return -1;
	}

#ifdef HDMITX
	I2C_init(I2CA_HDMI_BASE,ALT_CPU_FREQ,100000);
	InitIT6613();
#endif

	return 1;
}

// Enable chip outputs
void enable_outputs() {
	// program video mode
	program_mode();
	// enable TVP output
	tvp_enable_output();
#ifdef HDMITX
	// enable and unmute HDMITX
	// TODO: check pclk
	EnableVideoOutput(PCLK_MEDIUM, COLOR_RGB444, COLOR_RGB444, 1);
	HDMITX_SetAVIInfoFrame(1, F_MODE_RGB444, 0, 0);
	SetAVMute(FALSE);
#endif
}

int main()
{
	alt_u8 av_sel_vec, av_sel_vec_prev, target_input, target_format;
	avinput_t target_mode;
	alt_u8 av_init = 0;
	status_t status;
	char row1[20], row2[20];

	if (init_hw() == 1) {
		printf("### DIY VIDEO DIGITIZER / SCANCONVERTER INIT OK ###\n\n");
		IOWR_ALTERA_AVALON_PIO_DATA(PIO_5_BASE, 0xffff055C); //OSSC
		snprintf(row1, 17, "##### OSSC #####");
		snprintf(row2, 17, "2014-2015  marqs");
		alt_up_character_lcd_set_cursor_pos(char_lcd_dev, 0, 0);
		alt_up_character_lcd_string(char_lcd_dev, row1);
		alt_up_character_lcd_set_cursor_pos(char_lcd_dev, 0, 1);
		alt_up_character_lcd_string(char_lcd_dev, row2);
	}

	//RESET
	//tvp_reset();
	tvp_disable_output();

	// Write global default values to registers
	tvp_set_globaldefs();

	while(1) {
		// Select target input and mode
		av_sel_vec = ~IORD_ALTERA_AVALON_PIO_DATA(PIO_1_BASE) & AV_SEL_MASK;
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
				target_typemask = VIDEO_PC;
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
			snprintf(row1, 17, "%s       ", avinput_str[cm.avinput]);
			snprintf(row2, 17, "    NO SYNC     ");
			alt_up_character_lcd_set_cursor_pos(char_lcd_dev, 0, 0);
			alt_up_character_lcd_string(char_lcd_dev, row1);
			alt_up_character_lcd_set_cursor_pos(char_lcd_dev, 0, 1);
			alt_up_character_lcd_string(char_lcd_dev, row2);
		}

		usleep(20000);

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
					}
				  break;
				case REFCLK_CHANGE:
					if (cm.sync_active) {
						printf("Refclk change\n");
						tvp_sel_clk(cm.refclk);
					}
				  break;
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
	}

	return 0;
}
