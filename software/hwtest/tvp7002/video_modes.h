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
	MODE_PLLDIVBY2	= (1<<27)
} mode_flags;

struct mode_data {
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
};

static const struct mode_data video_modes[] = {
	{ "240p_L3M0",	1280,  240, 60, 1704,   263, 196, 15, 124,  3, VIDEO_SDTV, L3_MODE0, (MODE_L3ENABLE) },
	{ "240p_L3M1",	 960,  240, 60, 1278,   263, 147, 15,  93,  3, VIDEO_SDTV, L3_MODE1, (MODE_L3ENABLE) },
	{ "240p_L3M2",	 320,  240, 60,  426,   263,  49, 15,  31,  3, VIDEO_LDTV, L3_MODE2, (MODE_L3ENABLE|MODE_PLLDIVBY2) },
	{ "240p_L3M3",	 256,  240, 60,  341,   263,  39, 15,  25,  3, VIDEO_LDTV, L3_MODE3, (MODE_L3ENABLE|MODE_PLLDIVBY2) },
	{ "240p", 		 720,  240, 60,  858,   263,  65, 16,  60,  6, VIDEO_SDTV, L3_OFF, (MODE_L2ENABLE) },
	{ "288p", 		 720,  288, 50,  858,   313,  65, 16,  60,  6, VIDEO_SDTV, L3_OFF, (MODE_L2ENABLE) },
	{ "240p-forced", 720,  240, 60,  858,   263,  65, 16,  60,  6, VIDEO_SDTV, L3_OFF, (MODE_L2ENABLE|MODE_INTERLACED|MODE_MALFORMED) },
	{ "288p-forced", 720,  288, 50,  858,   313,  65, 16,  60,  6, VIDEO_SDTV, L3_OFF, (MODE_L2ENABLE|MODE_INTERLACED|MODE_MALFORMED) },
	{ "480i", 		 720,  240, 60,  858,   525,  65, 16,  60,  6, VIDEO_SDTV, L3_OFF, (MODE_L2ENABLE|MODE_INTERLACED) },
	{ "576i", 		 720,  288, 50,  858,   625,  65, 16,  60,  6, VIDEO_SDTV, L3_OFF, (MODE_L2ENABLE|MODE_INTERLACED) },
	{ "480p", 		 720,  480, 60,  858,   525,  65, 32,  60, 12, VIDEO_SDTV, L3_OFF, 0 },
	{ "576p", 		 720,  576, 50,  858,   625,  65, 32,  60, 12, VIDEO_SDTV, L3_OFF, 0 },
	{ "720p", 		1280,  720, 60, 1650,   750, 220, 20,  40,  5, VIDEO_HDTV, L3_OFF, 0 },
	{ "1080i", 		1920, 1080, 60, 2200,  1125, 148, 16,  44,  5, VIDEO_HDTV, L3_OFF, (MODE_L2ENABLE|MODE_INTERLACED) },
	{ "1080p", 		1920, 1080, 60, 2200,  1125, 148, 36,  44,  5, VIDEO_HDTV, L3_OFF, 0 },

	{ "720x240", 	 720,  240, 60,  858,   263,  65, 16,  60,  6,   VIDEO_PC, L3_OFF, (MODE_L2ENABLE) },
	{ "720x288", 	 720,  288, 50,  858,   313,  65, 16,  60,  6,   VIDEO_PC, L3_OFF, (MODE_L2ENABLE) },
	{ "640x480", 	 640,  480, 60,  800,   525,  48, 33,  96,  2,   VIDEO_PC, L3_OFF, 0 },
	{ "800x600", 	 800,  600, 60, 1056,   628,  88, 23, 128,  4,   VIDEO_PC, L3_OFF, 0 },
	{ "1280x720", 	1280,  720, 60, 1650,   750, 220, 20,  40,  5,   VIDEO_PC, L3_OFF, 0 },
	{ "1024x768", 	1024,  768, 60, 1344,   806, 160, 29, 136,  6,   VIDEO_PC, L3_OFF, 0 },
	{ "1280x1024", 	1280, 1024, 60, 1688,  1066, 248, 38, 112,  3,   VIDEO_PC, L3_OFF, 0 },
	{ "1920x1080", 	1920, 1080, 60, 2200,  1125, 148, 36,  44,  5,   VIDEO_PC, L3_OFF, 0 },
};

alt_8 get_mode_id(alt_u32 totlines, alt_u8 progressive, float hz, video_type typemask, alt_u8 linemult_target, alt_u8 l3_mode);

#endif /* VIDEO_MODES_H_ */
