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
#include "video_modes.h"

const mode_data_t video_modes[] = {
	{ "240p_L3M0",	1280,  240, 60, 1704,   264, 196, 15, 124,  3, VIDEO_SDTV, L3_MODE0, (MODE_L3ENABLE) },
	{ "240p_L3M1",	 960,  240, 60, 1278,   264, 147, 15,  93,  3, VIDEO_SDTV, L3_MODE1, (MODE_L3ENABLE) },
	{ "240p_L3M2",	 320,  240, 60,  426,   264,  49, 15,  31,  3, VIDEO_LDTV, L3_MODE2, (MODE_L3ENABLE|MODE_PLLDIVBY2) },
	{ "240p_L3M3",	 256,  240, 60,  341,   264,  39, 15,  25,  3, VIDEO_LDTV, L3_MODE3, (MODE_L3ENABLE|MODE_PLLDIVBY2) },
	{ "240p", 		 720,  240, 60,  858,   264,  65, 16,  60,  6, VIDEO_SDTV, L3_OFF, (MODE_L2ENABLE) },
	{ "288p", 		 720,  288, 50,  864,   314,  65, 16,  60,  6, VIDEO_SDTV, L3_OFF, (MODE_L2ENABLE) },
	{ "240p-forced", 720,  240, 60,  858,   264,  65, 16,  60,  6, VIDEO_SDTV, L3_OFF, (MODE_L2ENABLE|MODE_INTERLACED|MODE_MALFORMED) },
	{ "288p-forced", 720,  288, 50,  864,   314,  65, 16,  60,  6, VIDEO_SDTV, L3_OFF, (MODE_L2ENABLE|MODE_INTERLACED|MODE_MALFORMED) },
	{ "480i", 		 720,  240, 60,  858,   525,  65, 16,  60,  6, VIDEO_SDTV, L3_OFF, (MODE_L2ENABLE|MODE_INTERLACED) },
	{ "576i", 		 720,  288, 50,  864,   625,  65, 16,  60,  6, VIDEO_SDTV, L3_OFF, (MODE_L2ENABLE|MODE_INTERLACED) },
	{ "480p", 		 720,  480, 60,  858,   525,  65, 32,  60, 12, VIDEO_SDTV, L3_OFF, (MODE_DTV480P) },
	{ "VGA480p", 	 640,  480, 60,  800,   525,  48, 33,  96,  2, VIDEO_SDTV, L3_OFF, (MODE_VGA480P) },
	{ "576p", 		 720,  576, 50,  864,   625,  65, 32,  60, 12, VIDEO_SDTV, L3_OFF, 0 },
	{ "720p", 		1280,  720, 60, 1650,   750, 220, 20,  40,  5, VIDEO_HDTV, L3_OFF, 0 },
	{ "1080i", 		1920, 1080, 60, 2200,  1125, 148, 16,  44,  5, VIDEO_HDTV, L3_OFF, (MODE_L2ENABLE|MODE_INTERLACED) },
	{ "1080p", 		1920, 1080, 60, 2200,  1125, 148, 36,  44,  5, VIDEO_HDTV, L3_OFF, 0 },

    //{ "640x240", 	 640,  240, 60,  800,   264,  48, 16,  60,  6,   VIDEO_PC, L3_OFF, (MODE_L2ENABLE) },
	{ "720x240", 	 720,  240, 60,  858,   264,  65, 16,  60,  6,   VIDEO_PC, L3_OFF, (MODE_L2ENABLE) },
	{ "720x288", 	 720,  288, 50,  864,   314,  65, 16,  60,  6,   VIDEO_PC, L3_OFF, (MODE_L2ENABLE) },
    { "720x480i", 	 720,  240, 60,  858,   525,  65, 16,  60,  6,   VIDEO_PC, L3_OFF, (MODE_L2ENABLE|MODE_INTERLACED) },
    { "720x576i",	 720,  288, 50,  864,   625,  65, 16,  60,  6,   VIDEO_PC, L3_OFF, (MODE_L2ENABLE|MODE_INTERLACED) },
	{ "640x480", 	 640,  480, 60,  800,   525,  48, 33,  96,  2,   VIDEO_PC, L3_OFF, (MODE_VGA480P) },
	{ "720x480",	 720,  480, 60,  858,   525,  65, 32,  60, 12, 	 VIDEO_PC, L3_OFF, (MODE_DTV480P) },
	{ "800x600", 	 800,  600, 60, 1056,   628,  88, 23, 128,  4,   VIDEO_PC, L3_OFF, 0 },
	{ "1280x720", 	1280,  720, 60, 1650,   750, 220, 20,  40,  5,   VIDEO_PC, L3_OFF, 0 },
	{ "1024x768", 	1024,  768, 60, 1344,   806, 160, 29, 136,  6,   VIDEO_PC, L3_OFF, 0 },
	{ "1280x1024", 	1280, 1024, 60, 1688,  1066, 248, 38, 112,  3,   VIDEO_PC, L3_OFF, 0 },
	{ "1920x1080", 	1920, 1080, 60, 2200,  1125, 148, 36,  44,  5,   VIDEO_PC, L3_OFF, 0 },
};

/* TODO: rewrite, check hz etc. */
alt_8 get_mode_id(alt_u32 totlines, alt_u8 progressive, alt_u32 hz, video_type typemask, alt_u8 linemult_target, alt_u8 l3_mode, alt_u8 s480p_mode) {
	alt_8 i;
	alt_u8 num_modes = sizeof(video_modes)/sizeof(mode_data_t);

	// TODO: a better check
	for (i=0; i<num_modes; i++) {
		// blacklist a 480p mode on request
		if (s480p_mode != 0) {
			if (((s480p_mode == 1) && (video_modes[i].flags & MODE_VGA480P)) || ((s480p_mode == 2) && (video_modes[i].flags & MODE_DTV480P)))
				continue;
		}

		if ((typemask & video_modes[i].type) && (progressive == !(video_modes[i].flags & MODE_INTERLACED)) && (totlines <= video_modes[i].v_total)) {
			if (linemult_target && (video_modes[i].flags & MODE_L3ENABLE)) {
				if (video_modes[i].l3mode == l3_mode)
					return i;
			} else if (!(video_modes[i].flags & MODE_L3ENABLE)) {
				return i;
			}
		}
	}

	return -1;
}
