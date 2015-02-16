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
#include "altera_avalon_pio_regs.h"
#include "altera_up_avalon_character_lcd.h"
#include "i2c_opencores.h"
#include "tvp7002.h"

//#define SYNCBYPASS	// Bypass VGA syncs (for debug - needed for interlace?)
//#define EXTADCCLK		// Use external ADC clock (external osc)
//#define ADCPOWERDOWN	// Power-down ADCs
//#define PLLPOSTDIV	// Double-rate PLL with div-by-2 (decrease jitter?)

static inline void tvp_set_hpllcoast(alt_u8 pre, alt_u8 post) {
	tvp_writereg(TVP_HPLLPRECOAST, pre);
	tvp_writereg(TVP_HPLLPOSTCOAST, post);
}

static inline void tvp_set_ssthold(alt_u8 vsdetect_thold) {
	tvp_writereg(TVP_SSTHOLD, vsdetect_thold);
}

static void tvp_set_clamp(video_format fmt) {
	switch (fmt) {
		case FORMAT_RGBS:
		case FORMAT_RGBHV:
		case FORMAT_RGsB:
			//select bottom clamp (RGB)
			tvp_writereg(TVP_SOGTHOLD, 0x58);
		  break;
		case FORMAT_YPbPr:
			//select mid clamp for Pb & Pr
			tvp_writereg(TVP_SOGTHOLD, 0x5D);
		  break;
		default:
		  break;
	}
}

static void tvp_set_clamp_position(video_type type) {
	switch (type) {
		case VIDEO_LDTV:
			tvp_writereg(TVP_CLAMPSTART, 0x2);
			tvp_writereg(TVP_CLAMPWIDTH, 0x6);
		  break;
		case VIDEO_SDTV:
		case VIDEO_PC:
			tvp_writereg(TVP_CLAMPSTART, 0x6);
			tvp_writereg(TVP_CLAMPWIDTH, 0x10);
		  break;
		case VIDEO_HDTV:
			tvp_writereg(TVP_CLAMPSTART, 0x32);
			tvp_writereg(TVP_CLAMPWIDTH, 0x20);
		  break;
		default:
		  break;
	}
}

static void tvp_set_alc(video_type type) {
	//disable ALC
	//tvp_writereg(TVP_ALCEN, 0x00);
	//tvp_writereg(TVP_ALCEN, 0x80);

	//set analog (coarse) gain to max. value without clipping
	tvp_writereg(TVP_BG_CGAIN, 0x88);
	tvp_writereg(TVP_R_CGAIN, 0x08);

	//select ALC placement
	switch (type) {
		case VIDEO_LDTV:
			tvp_writereg(TVP_ALCPLACE, 0x9);
		  break;
		case VIDEO_SDTV:
		case VIDEO_PC:
			tvp_writereg(TVP_ALCPLACE, 0x18);
		  break;
		case VIDEO_HDTV:
			tvp_writereg(TVP_ALCPLACE, 0x5A);
		  break;
		default:
		  break;
	}
}


inline alt_u32 tvp_readreg(alt_u32 regaddr) {
	I2C_start(I2CA_BASE, TVP_BASE, 0);
	I2C_write(I2CA_BASE, regaddr, 0);
	I2C_start(I2CA_BASE, TVP_BASE, 1);
	return I2C_read(I2CA_BASE,1);
}

inline void tvp_writereg(alt_u32 regaddr, alt_u8 data) {
	I2C_start(I2CA_BASE, TVP_BASE, 0);
	I2C_write(I2CA_BASE, regaddr, 0);
	I2C_write(I2CA_BASE, data, 1);
}

inline void tvp_reset() {
	usleep(100000);
	IOWR_ALTERA_AVALON_PIO_DATA(PIO_0_BASE, 0x00);
	usleep(100000);
	IOWR_ALTERA_AVALON_PIO_DATA(PIO_0_BASE, 0x01);
	usleep(100000);
}

inline void tvp_disable_output() {
	usleep(100000);
	tvp_writereg(TVP_MISCCTRL1, 0x13);
	usleep(100000);
	tvp_writereg(TVP_MISCCTRL2, 0x03);
	usleep(100000);
}

inline void tvp_enable_output() {
	usleep(100000);
	tvp_writereg(TVP_MISCCTRL1, 0x11);
	usleep(100000);
	tvp_writereg(TVP_MISCCTRL2, 0x02);
	usleep(100000);
}

void tvp_set_globaldefs() {
	// Hsync input->output delay (horizontal shift)
	// Default is 13, which maintains alignment of RGB and hsync at output
	//tvp_writereg(TVP_HSOUTSTART, 0);

	// Hsync edge->Vsync edge delay
	tvp_writereg(TVP_VSOUTALIGN, 0);

	/* Y'Pb'Pr' to R'G'B' CSC coefficients.
	 *
	 * Coefficients from "Colour Space Conversions" (http://www.poynton.com/PDFs/coloureq.pdf), eq. 74.
	 * TODO: Should we use eq. 111/101 for SD, and eq. 105 for HD?
	 */
	tvp_writereg(TVP_CSC1HI, 0x20);
	tvp_writereg(TVP_CSC1LO, 0x00);
	tvp_writereg(TVP_CSC2HI, 0xF3);
	tvp_writereg(TVP_CSC2LO, 0x54);
	tvp_writereg(TVP_CSC3HI, 0xED);
	tvp_writereg(TVP_CSC3LO, 0x68);

	tvp_writereg(TVP_CSC4HI, 0x20);
	tvp_writereg(TVP_CSC4LO, 0x00);
	tvp_writereg(TVP_CSC5HI, 0x00);
	tvp_writereg(TVP_CSC5LO, 0x00);
	tvp_writereg(TVP_CSC6HI, 0x24);
	tvp_writereg(TVP_CSC6LO, 0x7B);

	tvp_writereg(TVP_CSC7HI, 0x20);
	tvp_writereg(TVP_CSC7LO, 0x00);
	tvp_writereg(TVP_CSC8HI, 0x40);
	tvp_writereg(TVP_CSC8LO, 0xEE);
	tvp_writereg(TVP_CSC9HI, 0x00);
	tvp_writereg(TVP_CSC9LO, 0x00);

	// Increase line length tolerance
	tvp_writereg(TVP_LINELENTOL, 0x06);

	// Common sync separator threshold
	tvp_set_ssthold(0x40);
}

// Configure H-PLL (sampling rate, VCO gain and charge pump current)
void tvp_setup_hpll(alt_u16 h_samplerate, alt_u16 v_lines, float hz, alt_u8 plldivby2) {
	float pclk_est;
	alt_u8 vco_range;
	alt_u8 cp_current;

	// Enable PLL post-div-by-2 with double samplerate
	if (plldivby2) {
		tvp_writereg(TVP_HPLLPHASE, 0x81);
		h_samplerate = 2*h_samplerate;
	} else {
		tvp_writereg(TVP_HPLLPHASE, 0x80);
	}

	tvp_writereg(TVP_HPLLDIV_MSB, (h_samplerate >> 4));
	tvp_writereg(TVP_HPLLDIV_LSB, ((h_samplerate & 0xf) << 4));

	printf("Horizontal samplerate set to %u\n", h_samplerate);

	pclk_est = (h_samplerate * v_lines * hz) / 1000000;

	printf("Estimated PCLK: %.2f MHz\n", pclk_est);

	if (pclk_est < 36) {
		vco_range = 0;
	} else if (pclk_est < 70) {
		vco_range = 1;
	} else if (pclk_est < 135) {
		vco_range = 2;
	} else {
		vco_range = 3;
	}

	cp_current =  (alt_u8)(40.0f*Kvco[vco_range]/h_samplerate + 0.5f) & 0x7;

	printf("VCO range: %s\nCPC: %u\n", Kvco_str[vco_range], cp_current);
	tvp_writereg(TVP_HPLLCTRL, ((vco_range << 6) | (cp_current << 3)));
}

void tvp_sel_clk(alt_u8 refclk) {
	//TODO: set SOG and CLP LPF based on mode
	if (refclk == REFCLK_INTCLK) {
		tvp_writereg(TVP_INPMUX2, 0xD2);
	} else {
#ifdef EXTADCCLK
		tvp_writereg(TVP_INPMUX2, 0xD8);
#else
		tvp_writereg(TVP_INPMUX2, 0xDA);
#endif
	}
}

void tvp_source_setup(alt_8 modeid, alt_u32 vlines, float hz, alt_u8 refclk) {
	video_type type = video_modes[modeid].type;

	// Configure clock settings
	tvp_sel_clk(refclk);

	// Clamp position and ALC
	tvp_set_clamp_position(type);
	tvp_set_alc(type);

	// Macrovision enable/disable, coast disable for RGBHV
	switch (type) {
		case VIDEO_PC:
			tvp_writereg(TVP_MISCCTRL4, 0x04);
		  break;
		case VIDEO_HDTV:
			tvp_writereg(TVP_MISCCTRL4, 0x00);
		  break;
		case VIDEO_LDTV:
		case VIDEO_SDTV:
			tvp_writereg(TVP_MISCCTRL4, 0x08);
			tvp_writereg(TVP_MVSWIDTH, 0x88); // TODO: check mode
		  break;
		default:
		  break;
	}

	tvp_setup_hpll(video_modes[modeid].h_total, vlines, hz, !!(video_modes[modeid].flags & MODE_PLLDIVBY2));

	//Long coast may lead to PLL frequency drift and sync loss (e.g. SNES)
	/*if (video_modes[modeid].v_active < 720)
		tvp_set_hpllcoast(3, 3);
	else*/
		tvp_set_hpllcoast(1, 0);

	// Hsync output width
	tvp_writereg(TVP_HSOUTWIDTH, video_modes[modeid].h_synclen);
}

void tvp_source_sel(tvp_input_t input, video_format fmt, alt_u8 refclk) {
	alt_u8 sync_status;

	// RGB+SOG input select
	tvp_writereg(TVP_INPMUX1, (input | (input<<2) | (input<<4) | (input<<6)));

	// Configure clock settings
	tvp_sel_clk(refclk);

	// Clamp setup
	tvp_set_clamp(fmt);

	// HV/SOG sync select
	if (input == TVP_INPUT2) {
		if (fmt == FORMAT_RGBHV)
			tvp_writereg(TVP_SYNCCTRL1, 0x52);
		else // RGBS
			tvp_writereg(TVP_SYNCCTRL1, 0x53);

		sync_status = tvp_readreg(TVP_SYNCSTAT);
		if (sync_status & (1<<7))
			printf("Hsync detected, %s polarity\n", (sync_status & (1<<5)) ? "pos" : "neg");
		if (sync_status & (1<<4))
			printf("Vsync detected, %s polarity %s\n", (sync_status & (1<<2)) ? "pos" : "neg", (sync_status & (1<<3)) ? "(extracted from CSYNC)" : "");
	} else {
		tvp_writereg(TVP_SYNCCTRL1, 0x5B);
		sync_status = tvp_readreg(TVP_SYNCSTAT);
		if (sync_status & (1<<1))
			printf("SOG detected\n");
		else
			printf("SOG not detected\n");
	}

	// Enable CSC for YPbPr
	if (fmt == FORMAT_YPbPr)
		tvp_writereg(TVP_MISCCTRL3, 0x10);
	else
		tvp_writereg(TVP_MISCCTRL3, 0x00);

	#ifdef SYNCBYPASS
		tvp_writereg(TVP_SYNCBYPASS, 0x03);
	#else
		tvp_writereg(TVP_SYNCBYPASS, 0x00);
	#endif

	//TODO:
	//clamps
	//TVP_ADCSETUP
	//video bandw ctrl

	printf("\n");
}

alt_u8 tvp_check_sync(tvp_input_t input) {
	alt_u8 sync_status;

	sync_status = tvp_readreg(TVP_SYNCSTAT);

	if (input == TVP_INPUT2)
		return !!((sync_status & 0x90) == 0x90);
	else
		return !!(sync_status & (1<<1));
}
