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
	input CLOCK_50,       
	output wire [3:0] VGA_R,  
   output wire [3:0] VGA_G,  
   output wire [3:0] VGA_B,
	output wire VGA_HS,       
	output wire VGA_VS	 
);
// Define wires
wire vga_clk;
wire [3:0] data;

// Instantiate vga_clk
VGA_clk VGA_clk_inst(
.inclk0(CLOCK_50),
.c0(vga_clk)
);

// Instantiate pixel buffer
Pixel_Buffer pixel_buffer_inst(
.clock(vga_clk),
.data(),
.rdaddress(),
.wraddress(),
.wren(),
.q(data)
);

// Instatiate vga_controller
vga_controller vga_controller_inst(
.VGA_DATA(data),
.VGA_CLK(vga_clk),
.VGA_ADDR(),
.VGA_R(VGA_R),
.VGA_G(VGA_G),
.VGA_B(VGA_B),
.VGA_HS(VGA_HS),
.VGA_VS(VGA_VS)
);

endmodule
