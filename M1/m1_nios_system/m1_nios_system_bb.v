
module m1_nios_system (
	clk_clk,
	reset_reset_n,
	sdram_addr,
	sdram_ba,
	sdram_cas_n,
	sdram_cke,
	sdram_cs_n,
	sdram_dq,
	sdram_dqm,
	sdram_ras_n,
	sdram_we_n,
	ledr_export,
	sw_export,
	key_export,
	hex_0_export,
	hex_3_export,
	address_export,
	data_export,
	camera_export);	

	input		clk_clk;
	input		reset_reset_n;
	output	[12:0]	sdram_addr;
	output	[1:0]	sdram_ba;
	output		sdram_cas_n;
	output		sdram_cke;
	output		sdram_cs_n;
	inout	[15:0]	sdram_dq;
	output	[1:0]	sdram_dqm;
	output		sdram_ras_n;
	output		sdram_we_n;
	output	[9:0]	ledr_export;
	input	[9:0]	sw_export;
	input	[1:0]	key_export;
	output	[23:0]	hex_0_export;
	output	[23:0]	hex_3_export;
	output	[16:0]	address_export;
	output	[3:0]	data_export;
	input		camera_export;
endmodule
