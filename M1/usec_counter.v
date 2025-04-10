// **
//
// Milestone 1 usec_counter.v module
// Made by Group B06
// 
// Author(s): 
//    James Thomson 33856257, 
//    Aadi Mahajan 33855994, 
//    Lance Miranda 31481795, 
//    Ryan Shanta 32284470, 
//    Xavier Hasiotis-Welsh 33880271,
//
// Last Edited: 10/04/2025
//
// **

module usec_counter (
    input clk,
    output reg [31:0] usec
);

    reg [5:0] counter; // set up the counter register (needs to count to 50, 6 bits)

    // At the positive edge of the clock signal activate this always block
    always @(posedge clk) 
        begin
            if (counter == 6'd49) begin // if the counter is equal to 49 (50 loops), add 1 to the microsecond counter (1 microsecond = 50 clock cycle)
                usec <= usec + 1'b1;
                counter <= 1'b0; // reset counter
            end else 
                counter <= counter + 1'b1; // add 1 to counter on clock cycle
        end

endmodule