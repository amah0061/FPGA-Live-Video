// *********************************
//
// Milestone 2
// Made by Group B06
// 
// Author(s): 
//	James Thomson 33856257, 
//	Aadi Mahajan 33855994, 
//	Lance Miranda 31481795, 
//	Ryan Shanta 32284470, 
//	Xavier Hasiotis-Welsh 33880271,
//
// Last Edited: 04/05/2025
//
// *********************************

module m3_complete (
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
	output DRAM_WE_N,
	// hex
	output [7:0] HEX0,
	output [7:0] HEX1,
	output [7:0] HEX2,
	output [7:0] HEX3,
	// gyro
	output GSENSOR_SCLK,
	inout GSENSOR_SDI,
	input GSENSOR_SDO,
	output GSENSOR_CS_N,
	input [2:1] GSENSOR_INT
);

// Define wires
wire vga_clk;
wire [3:0] data_raw, data_buff;
wire [16:0] read_address, write_address;
wire [31:0] usec_count;
wire [23:0] hex0;
wire [23:0] hex3;

// SPI wires
wire spi_clk;
wire spi_miso;
wire spi_mosi;
wire [1:0] spi_cs;

// Processor communication wires
wire p_spi_out;
wire p_processing0_out;
wire p_processing1_out;
wire p_display_out;
wire p_display_out_spi;

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
m3_nios_system u0 (
.address_export(write_address),
.camera_export(GPIO[2]),
.clk_clk(CLOCK_50),
.data_export(data_raw),
// gyro
.gsensor_int1_export(GSENSOR_INT[1]),
.gsensor_int2_export(GSENSOR_INT[2]),
// peripherals
.hex_0_export(hex0),
.hex_3_export(hex3),
.key_export(KEY[1:0]),
.ledr_export(LEDR[9:0]),
.sw_export(SW[9:0]),
.reset_reset_n(KEY[0]),
// peripherals between processor
.p_processing0_in_export(p_spi_out),
.p_processing0_out_export(p_processing0_out),
.p_spi_in_export(p_processing0_out),
.p_spi_out_export(p_spi_out),
.p_display_in_export(p_processing1_out),
.p_display_out_export(p_display_out),
.p_processing1_in_export(p_display_out),
.p_processing1_out_export(p_processing1_out),
.p_spi_in_display_export(p_display_out_spi),
.p_display_out_spi_export(p_display_out_spi),
// sdram
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
.spi_MISO(spi_miso),       //     spi.MISO -> SPI_SDO
.spi_MOSI(spi_mosi),       //        .MOSI -> SPI_SDI
.spi_SCLK(spi_clk),       //        .SCLK -> SPI_SCLK
.spi_SS_n(spi_cs),       //        .SS_n -> SPI_CS_N
//micro second counter
.usec_export(usec_count) 		
);

// SPI assignments
// ESP-cam
assign GPIO[8] = spi_mosi;
assign GPIO[9] = spi_clk;
assign GPIO[5] = spi_cs[0];

// gyro
assign GSENSOR_SDI = spi_mosi;
assign GSENSOR_SCLK = spi_clk;
assign GSENSOR_CS_N = spi_cs[1];

assign spi_miso = (spi_cs[0] == 1'b0) ? GPIO[7] :  
                  (spi_cs[1] == 1'b0) ? GSENSOR_SDO :
						1'bz;  

// HEX values 
assign HEX0 = hex0[7:0];
assign HEX1 = hex0[15:8];
assign HEX2 = hex0[23:16];
assign HEX3 = hex3[7:0];

endmodule
