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

module seg7_ctrl(
	input [3:0] char_id,
	output [6:0] segs
);

always @(*)
	case (char_id)
		4'h0: segs = ~7'b0111111;
		4'h1: segs = ~7'b0000110;
		4'h2: segs = ~7'b1011011;
		4'h3: segs = ~7'b1001111;
		4'h4: segs = ~7'b1100110;
		4'h5: segs = ~7'b1101101;
		4'h6: segs = ~7'b1111101;
		4'h7: segs = ~7'b0000111;
		4'h8: segs = ~7'b1111111;
		4'h9: segs = ~7'b1101111;
		4'hA: segs = ~7'b1001001;	// ≡ (Scanline sign)
		4'hB: segs = ~7'b1010100;	// n
		4'hC: segs = ~7'b0111001;	// C
		4'hD: segs = ~7'b1100110;	// y
		4'hE: segs = ~7'b0111000;	// L
		default: segs = ~7'b0000000;
	endcase

endmodule
