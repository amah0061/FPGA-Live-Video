// *********************************
//
// Milestone 1 task 1 integration module
// Made by Group B06
// 
// Author(s): James Thomson 33856257
// Last Edited: 24/03/2025
//
// *********************************

module m1_t1 (
	// task 1
	input CLOCK_50,       
	output wire [3:0] VGA_R,  
   output wire [3:0] VGA_G,  
   output wire [3:0] VGA_B,
	output wire VGA_HS,       
	output wire VGA_VS,
	// nios system
	input [1:0] KEY,
	output [9:0] LEDR,
	input [9:0] SW,
	inout [9:0] GPIO,
	// sdram
   output [12:0] DRAM_ADDR,
	output [1:0] DRAM_BA,
	output DRAM_CAS_N,
	output DRAM_CKE,
	output DRAM_CLK,
	output DRAM_CS_N,
	inout [15:0] DRAM_DQ,
	output DRAM_LDQM,
	output DRAM_UDQM,
	output DRAM_RAS_N,
	output DRAM_WE_N 
);
// Define wires
wire vga_clk;
wire [3:0] data_raw, data_buff;
wire [16:0] read_address, write_address;
wire [31:0] usec_count;

// SPI predef pins to high impedance (missing CAM_READY)
assign GPIO[1:0] = 2'bzz;
assign GPIO[4:3] = 2'bzz;
assign GPIO[6] = 1'bz;

// Instantiate sdram_clk
sdram_pll pll0(
.inclk0(CLOCK_50),
.c0(DRAM_CLK)
);


// Instantiate vga_clk
VGA_clk VGA_clk_inst(
.inclk0(CLOCK_50),
.c0(vga_clk)
);

// Instantiate pixel buffer
Pixel_Buffer pixel_buffer_inst(
.clock(vga_clk),
.data(data_raw),
.rdaddress(read_address),
.wraddress(write_address),
.wren(1'b1),
.q(data_buff)
);

// Instantiate vga_controller
vga_controller vga_controller_inst(
.VGA_DATA(data_buff),
.VGA_CLK(vga_clk),
.VGA_ADDR(read_address),
.VGA_R(VGA_R),
.VGA_G(VGA_G),
.VGA_B(VGA_B),
.VGA_HS(VGA_HS),
.VGA_VS(VGA_VS)
);

//instantiate usec_counter
usec_counter usec_counter_inst(
.clk(CLOCK_50),
.usec(usec_count)
);

// Instantiate nios system
m1_nios_system u0 (
.address_export(write_address),
.camera_export(GPIO[2]),
.clk_clk(CLOCK_50),
.data_export(data_raw),
.hex_0_export(),
.hex_3_export(),
.key_export(KEY[1:0]),
.ledr_export(LEDR[9:0]),
.sw_export(SW[9:0]),
.reset_reset_n(KEY[0]),
.sdram_addr(DRAM_ADDR),
.sdram_ba(DRAM_BA),      		
.sdram_cas_n(DRAM_CAS_N),
.sdram_cke(DRAM_CKE),
.sdram_cs_n(DRAM_CS_N),
.sdram_dq(DRAM_DQ),
.sdram_dqm({DRAM_UDQM, DRAM_LDQM}),
.sdram_ras_n(DRAM_RAS_N),
.sdram_we_n(DRAM_WE_N),
//spi stuff
.spi_MISO(GPIO[7]),       //     spi.MISO -> SPI_SDO
.spi_MOSI(GPIO[8]),       //        .MOSI -> SPI_SDI
.spi_SCLK(GPIO[9]),       //        .SCLK -> SPI_SCLK
.spi_SS_n(GPIO[5]),       //        .SS_n -> SPI_CS_N
//micro second counter
.usec_export(usec_count)   		
);

endmodule