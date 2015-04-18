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

#ifndef VIDEO_MODES_H_
#define VIDEO_MODES_H_

#include <alt_types.h>

typedef enum {
	FORMAT_RGBS = 0,
	FORMAT_RGBHV = 1,
	FORMAT_RGsB = 2,
	FORMAT_YPbPr = 3
} video_format;

typedef enum {
	VIDEO_LDTV = (1<<0),
	VIDEO_SDTV = (1<<1),
	VIDEO_HDTV = (1<<2),
	VIDEO_PC   = (1<<3)
} video_type;

typedef enum {
	L3_MODE0 = 0,
	L3_MODE1 = 1,
	L3_MODE2 = 2,
	L3_MODE3 = 3,
	L3_OFF = 8
} linetriplemode;

typedef enum {
	MODE_INTERLACED = (1<<31),
	MODE_MALFORMED	= (1<<30),
	MODE_L2ENABLE	= (1<<29),
	MODE_L3ENABLE	= (1<<28),
	MODE_PLLDIVBY2	= (1<<27),
	MODE_DTV480P	= (1<<26),
	MODE_VGA480P	= (1<<25)
} mode_flags;

typedef struct {
    const char *name;
    alt_u16 h_active;
    alt_u16 v_active;
    alt_u8 hz;
    alt_u16 h_total;
    alt_u16 v_total;
    alt_u8 h_backporch;
    alt_u8 v_backporch;
    alt_u8 h_synclen;
    alt_u8 v_synclen;
    video_type type;
    linetriplemode l3mode;
    mode_flags flags;
} mode_data_t;

alt_8 get_mode_id(alt_u32 totlines, alt_u8 progressive, alt_u32 hz, video_type typemask, alt_u8 linemult_target, alt_u8 l3_mode, alt_u8 s480p_mode);

#endif /* VIDEO_MODES_H_ */
