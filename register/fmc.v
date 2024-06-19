module fmc (
    input wire clock,
    input wire [15:0] DA_IN,   // Address/Data bus (bidirectional)
    output reg [15:0] DA_OUT,  // Address/Data bus (bidirectional)
    output reg [15:0] DA_OE,   // Address/Data bus (bidirectional)
    input wire [6:0] A,        // Upper address bits [22:16]
    input wire NL,             // Address latch enable
    input wire NOE,            // Read enable (active low)
    input wire NWE,            // Write enable (active low)
    input wire NE1,            // Chip select (active low)
    input wire NBL0,           // Byte enable 0 (active low)
    input wire NBL1,           // Byte enable 1 (active low)
    output wire LED,
    output wire PLL_RSTN,
    output reg dummy
    );

    reg [15:0] register[3:0];    // 16-bit register to store data
    reg [23:0] address;          // Latch for the address
    
    // Bidirectional bus control
    assign LED = register[0][0];
    assign PLL_RSTN = 1;
    
    
    always @(*)
        begin
        // Latch the address on NL low
        if (!NL)
            begin
            address = {A, DA_IN, ~NBL1};
            end
        
        // Write operation
        if (!NE1 && !NWE)
            begin
            if (!NBL0) register[address[2:1]][7:0] = DA_IN[7:0];   // Byte 0 write
            if (!NBL1) register[address[2:1]][15:8] = DA_IN[15:8]; // Byte 1 write
            end
        
        // Read operation
        if (!NE1 && !NOE)
            begin
            DA_OE = 16'hFFFF;
            DA_OUT = register[address[2:1]]; // Drive the AD bus with register data
            end
        else
            begin
            DA_OE = 16'h0000;
            end
        end
        
        always @(posedge clock)
            dummy = !dummy;

endmodule
