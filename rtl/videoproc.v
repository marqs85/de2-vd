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

`include "output_sel.v"

module videoproc(
	input clk50,
	input [3:0] button,
	input [17:0] switch,
	inout scl,
	inout sda,
	output reset_n_TVP,
	inout [7:0] LCD_DATA,
	output LCD_ON,
	//output LCD_BLON,		//No backlight on the default LCD module
	output LCD_EN,
	output LCD_RS,
	output LCD_RW,
	input [7:0] R_in,
	input [7:0] G_in,
	input [7:0] B_in,
	input FID_in,
	input VSYNC_in,
	input HSYNC_in,
	input PCLK_in,
`ifdef OUTPUT_VGA
	output [7:0] VGA_R,
	output [7:0] VGA_G,
	output [7:0] VGA_B,
	output VGA_CLK,
	output VGA_BLANK_N,
	output VGA_SYNC_N,
	output VGA_HS,
	output VGA_VS,
`endif
`ifdef OUTPUT_HDMI
	output HDMI_TX_PCSCL,
	inout HDMI_TX_PCSDA,
	output HDMI_TX_RST_N,
	output [11:0] HDMI_TX_RD,
	output [11:0] HDMI_TX_GD,
	output [11:0] HDMI_TX_BD,
	output HDMI_TX_DE,
	output HDMI_TX_HS,
	output HDMI_TX_VS,
	output HDMI_TX_PCLK,
	inout HDMI_TX_CEC,
	input HDMI_TX_INT_N,
	output [3:0] HDMI_TX_DSD_L,
	output [3:0] HDMI_TX_DSD_R,
	output [3:0] HDMI_TX_I2S,
	output HDMI_TX_SCK,
	output HDMI_TX_SPDIF,
	output HDMI_TX_WS,
	output HDMI_TX_MCLK,
	output HDMI_TX_DCLK,
`endif
`ifdef DEBUG
	output PCLK_out_debug,
	output HSYNC_out_debug,
	output VSYNC_out_debug,
	output pclk_2x_lock_debug,
`endif
	output [7:0] LED_G,
	output [17:0] LED_R,
	output [6:0] HEX0,
	output [6:0] HEX1,
	output [6:0] HEX2,
	output [6:0] HEX3,
	output [6:0] HEX4,
	output [6:0] HEX5,
	output [6:0] HEX6,
	output [6:0] HEX7,
	output SD_CLK,
	inout SD_CMD,
	inout [3:0] SD_DAT,
	inout SD_WP_N
);

wire [7:0] led_out;
wire [31:0] char_vec;
wire [55:0] seg_vec;
wire reset_n;
wire [20:0] controls_in;
wire [1:0] FID_ID;
wire h_unstable;
wire [2:0] pclk_lock;
wire [2:0] pll_lock_lost;
wire [31:0] h_info;
wire [31:0] v_info;
wire [10:0] lines_out;

wire [7:0] R_out, G_out, B_out;
wire HSYNC_out;
wire VSYNC_out;
wire PCLK_out;
wire DATA_enable;

assign reset_n_TVP = reset_n;
assign LCD_BLON = 1'b1;

assign LED_R[17:10] = led_out;
assign LED_G[7:6] = FID_ID;		// Active FID (0 --- 1)
assign LED_G[2] = pclk_lock[2];
assign LED_G[1] = pclk_lock[1];
assign LED_G[0] = pclk_lock[0];
assign LED_R[2] = pll_lock_lost[2];
assign LED_R[1] = pll_lock_lost[1];
assign LED_R[0] = pll_lock_lost[0];
assign LED_R[7:5] = {h_unstable, h_unstable, h_unstable};

assign HEX7 = seg_vec[55:49];
assign HEX6 = seg_vec[48:42];
assign HEX5 = seg_vec[41:35];
assign HEX4 = seg_vec[34:28];
assign HEX3 = seg_vec[27:21];
assign HEX2 = seg_vec[20:14];
assign HEX1 = seg_vec[13:7];
assign HEX0 = seg_vec[6:0];

assign SD_DAT[1] = 1'b0;
assign SD_DAT[2] = 1'b0;
assign SD_WP_N = 1'b1;

assign controls_in = {switch[17:0], button[3:1]};
assign reset_n = button[0];

`ifdef DEBUG
assign PCLK_out_debug = PCLK_in;
assign HSYNC_out_debug = HSYNC_in;
assign VSYNC_out_debug = VSYNC_in;
assign pclk_2x_lock_debug = pclk_lock[2];
`endif

`ifdef OUTPUT_VGA
assign VGA_R = R_out;
assign VGA_G = G_out;
assign VGA_B = B_out;
assign VGA_HS = HSYNC_out;
assign VGA_VS = VSYNC_out;
assign VGA_CLK = PCLK_out;
assign VGA_BLANK_N = DATA_enable;
assign VGA_SYNC_N = 1'b0;
`endif
`ifdef OUTPUT_HDMI
assign HDMI_TX_RD[11:4] = R_out;
assign HDMI_TX_GD[11:4] = G_out;
assign HDMI_TX_BD[11:4] = B_out;
assign HDMI_TX_RD[3:0] = 4'h0;
assign HDMI_TX_GD[3:0] = 4'h0;
assign HDMI_TX_BD[3:0] = 4'h0;
assign HDMI_TX_HS = HSYNC_out;
assign HDMI_TX_VS = VSYNC_out;
assign HDMI_TX_PCLK = PCLK_out;
assign HDMI_TX_DE = DATA_enable;
assign HDMI_TX_RST_N = reset_n;
assign HDMI_TX_DSD_L = 4'b0;
assign HDMI_TX_DSD_R = 4'b0;
assign HDMI_TX_I2S = 4'b0;
assign HDMI_TX_SCK = 1'b0;
assign HDMI_TX_SPDIF = 1'b0;
assign HDMI_TX_WS = 1'b0;
assign HDMI_TX_MCLK = 1'b0;
assign HDMI_TX_DCLK = 1'b0;
`endif

sys sys_inst(
	.clk_clk							(clk50),
	.reset_reset_n						(reset_n),
	.pio_0_error_leds_out_export		(led_out),
	.pio_1_controls_in_export   		({11'h000, controls_in}),
	.pio_2_horizontal_info_out_export   (h_info),
	.pio_3_vertical_info_out_export  	(v_info),
	.pio_4_linecount_in_export   		({21'h000000, lines_out}),
	.pio_5_charsegment_disp_out_export  (char_vec),
	.i2c_opencores_0_export_scl_pad_io	(scl),
	.i2c_opencores_0_export_sda_pad_io	(sda),
`ifdef OUTPUT_HDMI
	.i2c_opencores_1_export_scl_pad_io	(HDMI_TX_PCSCL),
	.i2c_opencores_1_export_sda_pad_io	(HDMI_TX_PCSDA),
`else
	.i2c_opencores_1_export_scl_pad_io	(),
	.i2c_opencores_1_export_sda_pad_io	(),
`endif
    .character_lcd_0_external_interface_DATA (LCD_DATA), // character_lcd_0_external_interface.DATA
    .character_lcd_0_external_interface_ON   (LCD_ON),   //                                   .ON
    .character_lcd_0_external_interface_EN   (LCD_EN),   //                                   .EN
    .character_lcd_0_external_interface_RS   (LCD_RS),   //                                   .RS
    .character_lcd_0_external_interface_RW   (LCD_RW),   //                                   .RW
	.character_lcd_0_external_interface_BLON (), 		 //                                   .BLON
	.sdcard_0_interface_b_SD_cmd             (SD_CMD),    //                 sdcard_0_interface.b_SD_cmd
	.sdcard_0_interface_b_SD_dat             (SD_DAT[0]), //                                   .b_SD_dat
	.sdcard_0_interface_b_SD_dat3            (SD_DAT[3]), //                                   .b_SD_dat3
	.sdcard_0_interface_o_SD_clock           (SD_CLK)     //                                   .o_SD_cloc
);

scaler scaler_inst (
	.HSYNC_in		(HSYNC_in),
	.VSYNC_in		(VSYNC_in),
	.PCLK_in		(PCLK_in),
	.FID_in			(FID_in),
	.R_in			(R_in),
	.G_in			(G_in),
	.B_in			(B_in),
	.h_info			(h_info),
	.v_info			(v_info),
	.R_out			(R_out),
	.G_out			(G_out),
	.B_out			(B_out),
	.HSYNC_out		(HSYNC_out),
	.VSYNC_out		(VSYNC_out),
	.PCLK_out		(PCLK_out),
	.DATA_enable	(DATA_enable),
	.FID_ID			(FID_ID),
	.h_unstable		(h_unstable),
	.pclk_lock  	(pclk_lock),
	.pll_lock_lost	(pll_lock_lost),
	.lines_out		(lines_out)
);

genvar i;
generate
	for (i=0; i<8; i=i+1)
	begin
		:SEGROW seg7_ctrl M (
			.char_id		(char_vec[(4*(i+1)-1):(4*i)]),
			.segs			(seg_vec[(7*(i+1)-1):(7*i)])
		);
	end
endgenerate

endmodule
