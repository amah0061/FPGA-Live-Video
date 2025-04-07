module usec_counter (
	input clk,
	output reg [31:0] usec
);

	reg counter;

	always @(posedge clk) 
		begin
			if (counter == 50) begin
				usec = usec + 1;
				counter = 1;
			end else 
				counter = counter + 1;
		end
			
endmodule 