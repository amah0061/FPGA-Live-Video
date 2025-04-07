module usec_counter (
	input clk,
	output reg [31:0] usec
);

	reg [5:0] counter;

	always @(posedge clk) 
		begin
			if (counter == 49) begin
				usec <= usec + 1;
				counter <= 0;
			end else 
				counter <= counter + 1;
		end
			
endmodule 