module qbus (
    input logic clock,
    
    // FMC signals
    input logic [15:0] DA_IN,   // Address/Data bus (bidirectional)
    output logic [15:0] DA_OUT,  // Address/Data bus (bidirectional)
    output logic [15:0] DA_OE,   // Address/Data bus (bidirectional)
    input logic [6:0] A,        // Upper address bits [22:16]
    input logic NL,             // Address latch enable
    input logic NOE,            // Read enable (active low)
    input logic NWE,            // Write enable (active low)
    input logic NE1,            // Chip select (active low)
    input logic NBL0,           // Byte enable 0 (active low)
    input logic NBL1,           // Byte enable 1 (active low)

    // Qbus signals
    input logic [21:0] BDALf_IN,
    output logic [21:0] BDALf_OUT,
    output logic [21:0] BDALf_OE,
    input logic BSYNCf,
    input logic BDINf,
    input logic BDOUTf,
    input logic BRPLYf,
    input logic BWTBTf,
    input logic BBS7f,
    input logic BINITf,
    output logic Outbound,
    output logic BRPLYg,
    
    // For now these are unused. Drive the MOSFET gates low, don't want them floating.
    output logic BSYNCg,
    output logic BDINg,
    output logic BDOUTg,
    output logic BWTBTg,
    output logic BBS7g,
    output logic BIRQ4g,
    output logic BIRQ5g,
    output logic BIRQ6g,
    output logic BIAKOg,
    output logic BDMRg,
    output logic BSACKg,
    output logic BDMGOg,
    output logic BREFg,

    // other
    output logic LED,
    output logic PLL_RSTN,
    output logic dummy
    );

    parameter [21:0] QADDR = 22'o17772150;
    
    
    logic [15:0] register[3:0];    // 16-bit register to store data
    logic [23:0] Faddress;         // Latch for the FMC address
    logic [21:0] Qaddress;         // Latch for the Qbus address
    logic WriteCycle;
    logic BBS7;
    logic Qselected;

    
    assign Qselected = BBS7 && Qaddress[12:3] == QADDR[12:3];
    assign LED = register[0][0];
    assign PLL_RSTN = 1;
    
    assign BSYNCg = 0;
    assign BDINg  = 0;
    assign BDOUTg = 0;
    assign BWTBTg = 0;
    assign BBS7g  = 0;
    assign BIRQ4g = 0;
    assign BIRQ5g = 0;
    assign BIRQ6g = 0;
    assign BIAKOg = 0;
    assign BDMRg  = 0;
    assign BSACKg = 0;
    assign BDMGOg = 0;
    assign BREFg  = 0;


    // Capture QBus address and related info at leading edge of BSYNC
    always_ff @(negedge BSYNCf)
        begin
        Qaddress <= ~BDALf_IN;
        BBS7 <= ~BBS7f;
        WriteCycle <= ~BWTBTf;
        end
            

    always_comb
        begin

        DA_OE = 16'h0000;
        BRPLYg = 0;
        BDALf_OE = 22'h000000;                          // disable the FPGA bus drivers by default
        Outbound = 0;                                   // disable the BDAL gate drivers by default
        
        // FMC logic

        // Latch the FMC address on NL low
        if (!NL)
            begin
            Faddress = {A, DA_IN, ~NBL1};
            end

        // FMC write operation
        if (!NE1 && !NWE)
            begin
            if (!NBL0) register[Faddress[2:1]][7:0] = DA_IN[7:0];   // Byte 0 write
            if (!NBL1) register[Faddress[2:1]][15:8] = DA_IN[15:8]; // Byte 1 write
            end
            
        // FMC read operation
        if (!NE1 && !NOE)
            begin
            DA_OUT = register[Faddress[2:1]]; // Drive the AD bus with register data
            DA_OE = 16'hFFFF;
            end

       
        // Qbus logic
                
        // Qbus read operation
        if (Qselected && !BDINf)
            begin
            BDALf_OUT[21:18] = 4'b0000;
            BDALf_OUT[17] = 0;                          // memory parity error enable
            BDALf_OUT[16] = 0;                          // memory parity error
            BDALf_OUT[15:0] = register[Qaddress[2:1]];  // Drive the BDALf bus with register data
            BDALf_OE = 22'h3FFFFF;                      // enable the FPGA bus drivers to output the data
            Outbound = 1;                               // enable the gate drivers
            BRPLYg = 1;                                 // assert the reply signal
            end
            
        // Qbus write operation
        if (Qselected && !BDOUTf)
            begin
            if (BWTBTf || Qaddress[0] == 0) register[Qaddress[2:1]][7:0]  = ~BDALf_IN[7:0];      // Byte 0 write
            if (BWTBTf || Qaddress[0] == 1) register[Qaddress[2:1]][15:8] = ~BDALf_IN[15:8];  // Byte 1 write
            BRPLYg = 1;                                 // assert the reply signal
            end

        end          

    always_ff @(posedge clock)
        begin
        dummy = !dummy;
        end
            
endmodule
