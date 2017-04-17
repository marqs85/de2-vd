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

#ifndef TVP7002_H_
#define TVP7002_H_

#include "tvp7002_regs.h"
#include "video_modes.h"
#include "sysconfig.h"

//#define I2C_DEBUG
#define I2CA_BASE I2C_OPENCORES_0_BASE

typedef enum {
	TVP_INPUT1 = 0,
	TVP_INPUT2 = 1,
	TVP_INPUT3 = 2
} tvp_input_t;

static const alt_u8 Kvco[] = {75, 85, 150, 200};
static const char *Kvco_str[] = { "Ultra low", "Low", "Medium", "High" };

typedef enum {
	REFCLK_EXT27 	= 0,
	REFCLK_INTCLK 	= 1
} tvp_refclk_t;

typedef struct {
	const char *name;
	alt_u16 R_Y;
	alt_u16 R_Pb;
	alt_u16 R_Pr;
	alt_u16 G_Y;
	alt_u16 G_Pb;
	alt_u16 G_Pr;
	alt_u16 B_Y;
	alt_u16 B_Pb;
	alt_u16 B_Pr;
} ypbpr_to_rgb_csc_t;

static const alt_u32 clkrate[] = {27000000, 6500000}; //in MHz


inline alt_u32 tvp_readreg(alt_u32 regaddr);

inline void tvp_writereg(alt_u32 regaddr, alt_u8 data);

inline void tvp_reset();

inline void tvp_disable_output();

inline void tvp_enable_output();

void tvp_init();

void tvp_setup_hpll(alt_u16 h_samplerate, alt_u16 v_lines, alt_u8 hz, alt_u8 plldivby2);

void tvp_sel_clk(alt_u8 refclk);

void tvp_sel_csc(ypbpr_to_rgb_csc_t *csc);

void tvp_set_lpf(alt_u8 val);

void tvp_set_sync_lpf(alt_u8 val);

void tvp_set_hpll_phase(alt_u8 val);

void tvp_source_setup(alt_8 modeid, alt_u32 vlines, alt_u8 hz, alt_u8 refclk);

void tvp_source_sel(tvp_input_t input, video_format fmt, alt_u8 refclk);

alt_u8 tvp_check_sync(tvp_input_t input);

#endif /* TVP7002_H_ */
