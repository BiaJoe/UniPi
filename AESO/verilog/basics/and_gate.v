// and_gate.v
module and_gate(
    input a, b,    // Inputs to the AND gate
    output c       // Output from the AND gate
);
    assign c = a & b;  // AND operation
endmodule


