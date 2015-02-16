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

/* TODO: rewrite, check hz etc. */
alt_8 get_mode_id(alt_u32 totlines, alt_u8 progressive, float hz, video_type typemask, alt_u8 linemult_target, alt_u8 l3_mode) {
	alt_8 i;
	alt_u8 num_modes = sizeof(video_modes)/sizeof(struct mode_data);

	// TODO: a better check
	for (i=0; i<num_modes; i++) {
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
