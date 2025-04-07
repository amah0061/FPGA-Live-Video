module usec_counter (
	input clk,
	output reg [31:0] usec
);

	always @(posedge clk) 
		begin			
			usec = usec + 1;
		end
			

endmodule 