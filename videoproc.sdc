### CPU clock constraints ###

create_clock -period 50MHz -name clk50 [get_ports clk50]

set_input_delay -clock clk50 0 [get_ports {button* switch* sda scl LCD_DATA* HDMI_TX_PCSDA}]


### Scaler clock constraints ###

create_clock -period 108MHz -name pclk_hdtv [get_ports PCLK_in]
create_clock -period 27MHz -name pclk_ldtv_hs_M0 [get_ports PCLK_in] -add
create_clock -period 20MHz -name pclk_ldtv_hs_M1 [get_ports PCLK_in] -add
create_clock -period 13.5MHz -name pclk_sdtv [get_ports PCLK_in] -add
create_clock -period 6.7MHz -name pclk_ldtv_M2 [get_ports PCLK_in] -add
create_clock -period 5.4MHz -name pclk_ldtv_M3 [get_ports PCLK_in] -add

#derive_pll_clocks
create_generated_clock -master_clock pclk_sdtv -source {scaler_inst|pll_linedouble|altpll_component|auto_generated|pll1|inclk[0]} -multiply_by 2 -duty_cycle 50.00 -name pclk_2x {scaler_inst|pll_linedouble|altpll_component|auto_generated|pll1|clk[0]}
create_generated_clock -master_clock pclk_ldtv_hs_M0 -source {scaler_inst|pll_linetriple|altpll_component|auto_generated|pll1|inclk[0]} -multiply_by 3 -duty_cycle 50.00 -name pclk_3x_M0 {scaler_inst|pll_linetriple|altpll_component|auto_generated|pll1|clk[0]}
create_generated_clock -master_clock pclk_ldtv_hs_M1 -source {scaler_inst|pll_linetriple|altpll_component|auto_generated|pll1|inclk[0]} -multiply_by 3 -duty_cycle 50.00 -name pclk_3x_M1 {scaler_inst|pll_linetriple|altpll_component|auto_generated|pll1|clk[0]} -add
create_generated_clock -master_clock pclk_ldtv_hs_M1 -source {scaler_inst|pll_linetriple|altpll_component|auto_generated|pll1|inclk[0]} -multiply_by 4 -duty_cycle 50.00 -name pclk_4x_M1 {scaler_inst|pll_linetriple|altpll_component|auto_generated|pll1|clk[1]}
create_generated_clock -master_clock pclk_ldtv_M2 -source {scaler_inst|pll_linetriple_lowfreq|altpll_component|auto_generated|pll1|inclk[0]} -multiply_by 3 -duty_cycle 50.00 -name pclk_3x_h1x_M2 {scaler_inst|pll_linetriple_lowfreq|altpll_component|auto_generated|pll1|clk[0]}
create_generated_clock -master_clock pclk_ldtv_M2 -source {scaler_inst|pll_linetriple_lowfreq|altpll_component|auto_generated|pll1|inclk[0]} -multiply_by 12 -duty_cycle 50.00 -name pclk_3x_h4x_M2 {scaler_inst|pll_linetriple_lowfreq|altpll_component|auto_generated|pll1|clk[1]}
create_generated_clock -master_clock pclk_ldtv_M3 -source {scaler_inst|pll_linetriple_lowfreq|altpll_component|auto_generated|pll1|inclk[0]} -multiply_by 3 -duty_cycle 50.00 -name pclk_3x_h1x_M3 {scaler_inst|pll_linetriple_lowfreq|altpll_component|auto_generated|pll1|clk[0]} -add
create_generated_clock -master_clock pclk_ldtv_M3 -source {scaler_inst|pll_linetriple_lowfreq|altpll_component|auto_generated|pll1|inclk[0]} -multiply_by 15 -duty_cycle 50.00 -name pclk_3x_h5x_M3 {scaler_inst|pll_linetriple_lowfreq|altpll_component|auto_generated|pll1|clk[2]}

derive_clock_uncertainty

# input delay constraints
set critinputs [get_ports {R_in* G_in* B_in* FID_in HSYNC_in VSYNC_in}]
set_input_delay -clock pclk_sdtv -min 0 $critinputs
set_input_delay -clock pclk_sdtv -max 1.5 $critinputs
set_input_delay -clock pclk_hdtv -min 0 $critinputs -add_delay
set_input_delay -clock pclk_hdtv -max 1.5 $critinputs -add_delay
set_input_delay -clock pclk_ldtv_M2 -min 0 $critinputs -add_delay
set_input_delay -clock pclk_ldtv_M2 -max 1.5 $critinputs -add_delay
set_input_delay -clock pclk_ldtv_M3 -min 0 $critinputs -add_delay
set_input_delay -clock pclk_ldtv_M3 -max 1.5 $critinputs -add_delay
set_input_delay -clock pclk_ldtv_hs_M0 -min 0 $critinputs -add_delay
set_input_delay -clock pclk_ldtv_hs_M0 -max 1.5 $critinputs -add_delay
set_input_delay -clock pclk_ldtv_hs_M1 -min 0 $critinputs -add_delay
set_input_delay -clock pclk_ldtv_hs_M1 -max 1.5 $critinputs -add_delay

# output delay constraints (TODO: add vsync)
set critoutputs_vga {VGA_R* VGA_G* VGA_B* VGA_BLANK_N VGA_SYNC_N VGA_HS}
set critoutputs_hdmi {HDMI_TX_RD* HDMI_TX_GD* HDMI_TX_BD* HDMI_TX_DE HDMI_TX_HS}
set_output_delay -reference_pin VGA_CLK -clock pclk_hdtv 0 $critoutputs_vga
set_output_delay -reference_pin VGA_CLK -clock pclk_2x 0 $critoutputs_vga -add_delay
set_output_delay -reference_pin VGA_CLK -clock pclk_3x_M0 0 $critoutputs_vga -add_delay
set_output_delay -reference_pin VGA_CLK -clock pclk_4x_M1 0 $critoutputs_vga -add_delay
set_output_delay -reference_pin VGA_CLK -clock pclk_3x_h4x_M2 0 $critoutputs_vga -add_delay
set_output_delay -reference_pin VGA_CLK -clock pclk_3x_h5x_M3 0 $critoutputs_vga -add_delay
set_output_delay -reference_pin HDMI_TX_PCLK -clock pclk_hdtv 0 $critoutputs_hdmi
set_output_delay -reference_pin HDMI_TX_PCLK -clock pclk_2x 0 $critoutputs_hdmi -add_delay
set_output_delay -reference_pin HDMI_TX_PCLK -clock pclk_3x_M0 0 $critoutputs_hdmi -add_delay
set_output_delay -reference_pin HDMI_TX_PCLK -clock pclk_4x_M1 0 $critoutputs_hdmi -add_delay
set_output_delay -reference_pin HDMI_TX_PCLK -clock pclk_3x_h4x_M2 0 $critoutputs_hdmi -add_delay
set_output_delay -reference_pin HDMI_TX_PCLK -clock pclk_3x_h5x_M3 0 $critoutputs_hdmi -add_delay
set_false_path -to [remove_from_collection [all_outputs] [concat $critoutputs_vga $critoutputs_hdmi]]


### CPU/scaler clock relations ###

# Set pixel clocks as exclusive clocks
set_clock_groups -exclusive \
-group {pclk_hdtv} \
-group {pclk_sdtv pclk_2x} \
-group {pclk_ldtv_hs_M0 pclk_3x_M0} \
-group {pclk_ldtv_hs_M1 pclk_3x_M1 pclk_4x_M1} \
-group {pclk_ldtv_M2 pclk_3x_h1x_M2 pclk_3x_h4x_M2} \
-group {pclk_ldtv_M3 pclk_3x_h1x_M3 pclk_3x_h5x_M3}

# Treat CPU clock asynchronous to pixel clocks 
set_clock_groups -asynchronous -group {clk50}

# Filter out impossible output mux combinations
set clkmuxregs [get_cells {scaler:scaler_inst|R_out[*] scaler:scaler_inst|G_out[*] scaler:scaler_inst|B_out[*] scaler:scaler_inst|HSYNC_out scaler:scaler_inst|DATA_enable scaler:scaler_inst|*_pp1*}]
set clkmuxnodes [get_pins {scaler_inst|linebuf_*|altsyncram_*|auto_generated|ram_*|portbaddr*}]
set_false_path -from [get_clocks {pclk_ldtv* pclk_sdtv}] -through $clkmuxregs
set_false_path -from pclk_3x_M1 -through [remove_from_collection $clkmuxregs {scaler:scaler_inst|DATA_enable_pp1* scaler:scaler_inst|HSYNC_pp1*}] -to pclk_4x_M1
set_false_path -from pclk_3x_M1 -through $clkmuxnodes -to pclk_4x_M1

# Ignore paths from registers which are updated only at the end of vsync
set_false_path -from [get_cells {scaler_inst|H_* scaler_inst|V_* scaler:scaler_inst|lines_*}]

# Ignore paths from registers which are updated only at the end of hsync
set_false_path -from [get_cells {scaler:scaler_inst|vcnt_* scaler:scaler_inst|line_idx}]

# Ignore following clock transfers
set_false_path -from [get_clocks pclk_2x] -to [get_clocks pclk_sdtv]
set_false_path -from [get_clocks pclk_3x_M*] -to [get_clocks {pclk_ldtv_hs_M*}]
set_false_path -from [get_clocks pclk_4x_M1] -to [get_clocks {pclk_ldtv_hs_M1 pclk_3x_M1}]
set_false_path -from [get_clocks pclk_3x_h4x_M2] -to [get_clocks {pclk_ldtv_M2 pclk_3x_h1x_M2}]
set_false_path -from [get_clocks pclk_3x_h5x_M3] -to [get_clocks {pclk_ldtv_M3 pclk_3x_h1x_M3}]
set_false_path -from [get_clocks pclk_3x_h1x_M*] -to [get_clocks {pclk_ldtv_M*}]


### JTAG Signal Constraints ###

#constrain the TCK port
#create_clock -name tck -period "10MHz" [get_ports altera_reserved_tck]
#cut all paths to and from tck
set_clock_groups -exclusive -group [get_clocks altera_reserved_tck]
#constrain the TDI port
set_input_delay -clock altera_reserved_tck 20 [get_ports altera_reserved_tdi]
#constrain the TMS port
set_input_delay -clock altera_reserved_tck 20 [get_ports altera_reserved_tms]
#constrain the TDO port
#set_output_delay -clock altera_reserved_tck 20 [get_ports altera_reserved_tdo]
