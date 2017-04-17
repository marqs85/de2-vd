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

`define STATE_IDLE		        2'b00
`define STATE_LEADVERIFY		2'b01
`define STATE_DATARCV			2'b10

module ir_rcv (
    input clk50,
    input reset_n,
    input ir_rx,
	output reg [31:0] ir_code,
	output reg ir_code_ack
);

// 20ns clock period

parameter LEADCODE_LO_THOLD     = 230000;  //4.60ms
parameter LEADCODE_HI_THOLD     = 210000;  //4.20ms
parameter LEADCODE_HI_RPT_THOLD = 105000;  //2.1ms
parameter RPT_RELEASE_THOLD     = 6000000; //120ms
parameter BIT_ONE_THOLD         = 41500;   //0.83ms
parameter BIT_DETECT_THOLD      = 20000;   //0.4ms
parameter IDLE_THOLD            = 262143;  //5.24ms

reg [1:0] state;        // 3 states
reg [31:0] databuf;     // temp. buffer
reg [5:0] bits_detected;    // max. 63, effectively between 0 and 33
reg [17:0] act_cnt;     // max. 5.2ms
reg [17:0] leadvrf_cnt; // max. 5.2ms
reg [17:0] datarcv_cnt; // max. 5.2ms
reg [22:0] rpt_cnt;     // max. 166ms

// activity when signal is low
always @(posedge clk50 or negedge reset_n)
begin
    if (!reset_n)
        act_cnt <= 0;
	else
    begin
        if ((state == `STATE_IDLE) & (~ir_rx))
            act_cnt <= act_cnt + 1'b1;
        else
            act_cnt <= 0;
    end
end

// lead code verify counter
always @(posedge clk50 or negedge reset_n)
begin
    if (!reset_n)
        leadvrf_cnt <= 0;
	else
    begin
        if ((state == `STATE_LEADVERIFY) & ir_rx)
            leadvrf_cnt <= leadvrf_cnt + 1'b1;
        else
            leadvrf_cnt <= 0;
    end
end


// '0' and '1' are differentiated by high phase duration preceded by constant length low phase
always @(posedge clk50 or negedge reset_n)
begin
    if (!reset_n)
    begin
        datarcv_cnt <= 0;
        bits_detected <= 0;
        databuf <= 0;
    end
	else
    begin
        if (state == `STATE_DATARCV)
        begin
            if (ir_rx)
                datarcv_cnt <= datarcv_cnt + 1'b1;
            else
                datarcv_cnt <= 0;

            if (datarcv_cnt == BIT_DETECT_THOLD)
                bits_detected <= bits_detected + 1'b1;
                
            if (datarcv_cnt == BIT_ONE_THOLD)
                databuf[32-bits_detected] <= 1'b1;
        end
        else
        begin
            datarcv_cnt <= 0;
            bits_detected <= 0;
            databuf <= 0;
        end
    end
end

// read and validate data after 32 bits detected (last bit may change to '1' at any time)
always @(posedge clk50 or negedge reset_n)
begin
    if (!reset_n)
    begin
        ir_code_ack <= 1'b0;
        ir_code <= 32'h00000000;
    end
	else
    begin
        if ((bits_detected == 32) & (databuf[15:8] == ~databuf[7:0]))
        begin
            ir_code <= databuf;
            ir_code_ack <= 1'b1;
        end
        else if (rpt_cnt >= RPT_RELEASE_THOLD)
        begin
            ir_code <= 32'h00000000;
            ir_code_ack <= 1'b0;
        end
        else
            ir_code_ack <= 1'b0;
    end
end

always @(posedge clk50 or negedge reset_n)
begin
    if (!reset_n)
    begin
        state <= `STATE_IDLE;
        rpt_cnt <= 0;
    end
    else
    begin
        rpt_cnt <= rpt_cnt + 1'b1;
    
        case (state)
            `STATE_IDLE:
                if (act_cnt >= LEADCODE_LO_THOLD)
                    state <= `STATE_LEADVERIFY;
            `STATE_LEADVERIFY:
            begin
                if (leadvrf_cnt == LEADCODE_HI_RPT_THOLD)
                    rpt_cnt <= 0;
                if (leadvrf_cnt >= LEADCODE_HI_THOLD)
                    state <= `STATE_DATARCV;
            end    
            `STATE_DATARCV:
                if ((datarcv_cnt >= IDLE_THOLD) | (bits_detected >= 33))
                    state <= `STATE_IDLE;
            default:
                state <= `STATE_IDLE;
        endcase
    end
end

endmodule
