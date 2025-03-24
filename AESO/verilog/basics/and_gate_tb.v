// and_gate_tb.v
`timescale 1ns / 1ps

module and_gate_tb;
    // Testbench signals
    reg a, b;
    wire c;

    // Instantiate the AND gate module
    and_gate uut (
        .a(a), 
        .b(b), 
        .c(c)
    );

    // Generate VCD output for waveform viewing
    initial begin
        $dumpfile("and_gate.vcd");  // Create a VCD file named and_gate.vcd
        $dumpvars(0, and_gate_tb);  // Dump all variables from the and_gate_tb module

        // Stimulus for the inputs
        a = 0; b = 0; #10;  // Delay of 10 time units
        a = 0; b = 1; #10;
        a = 1; b = 0; #10;
        a = 1; b = 1; #10;

        // Finish simulation
        $finish;
    end
endmodule