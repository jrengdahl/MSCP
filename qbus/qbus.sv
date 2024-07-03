module qbus (
    input logic clock,
    
    // FMC signals
    input logic [15:0] DA_IN,       // Address/Data bus (bidirectional)
    output logic [15:0] DA_OUT,     // Address/Data bus (bidirectional)
    output logic [15:0] DA_OE,      // Address/Data bus (bidirectional)
    input logic [6:0] A,            // Upper address bits [22:16]
    input logic NL,                 // Address latch enable
    input logic NOE,                // Read enable (active low)
    input logic NWE,                // Write enable (active low)
    input logic NE1,                // Chip select (active low)
    input logic NBL0,               // Byte enable 0 (active low)
    input logic NBL1,               // Byte enable 1 (active low)

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
    output logic FPGA_IRQ,          // interrupt from FPGA to H723
    output logic LED,
    output logic PLL_RSTN,
    output logic dummy
    );

    parameter [21:0] QADDR_IR = 22'o17772150;
    parameter [21:0] QADDR_SA = 22'o17772152;
    
    
    logic [15:0] SA_Status;         // status read by PDP-11, written by H723
    logic [15:0] SA_Address;        // address written by PDP-11, read by H723

    logic IR_Read;                  // IR register has been read by PDP-11 (poll queue)
    logic IR_Written;               // IR register has been written by PDP-11 (init)
    logic SA_Read;                  // SA register has been read by PDP-11
    logic SA_Written;               // SA register has been written by PDP-11

    logic IR_ReadR;                 // latched version of the above for read-and-clear
    logic IR_WrittenR;              //
    logic SA_ReadR;                 //
    logic SA_WrittenR;              //

    logic [23:0] Faddress;          // Latch for the FMC address
    logic Q_IR_selected;            // latches whether the IR register is addressed by the Qbus
    logic Q_SA_selected;            // latches whether the SA register is addressed by the Qbus
    logic Qaddress0;                // latches the low bit of the Qbus address
       
    assign FPGA_IRQ = IR_Read || IR_Written || SA_Read || SA_Written;


    assign LED = SA_Address[0];
    assign PLL_RSTN = 1;
   
    logic  F_IR_read_selected;      // comb, IR is being read by H723
    assign F_IR_read_selected = !NE1 && !NOE && Faddress[21:0] == QADDR_IR[21:0];

 
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


    // Latch the FMC address on NL
    always_ff @(posedge NL)
        begin
        Faddress <= {A, DA_IN, !NBL1 && NBL0};
        end

    // FMC write to SA register
    always_ff @(posedge NWE)
        begin
        if (!NE1 && Faddress[21:1] == QADDR_SA[21:1])
            begin
            if (!NBL0) SA_Status[7:0]  <= DA_IN[7:0];   // Byte 0 write
            if (!NBL1) SA_Status[15:8] <= DA_IN[15:8];  // Byte 1 write
            end
        end

    // FMC read
    
    always_ff @(posedge NL) // latch the status bits at the beginning of any cycle
        begin
        IR_ReadR <= IR_Read;
        IR_WrittenR <= IR_Written;
        SA_ReadR <= SA_Read;
        SA_WrittenR <= SA_Written;
        end
    
    always_comb
        begin
        DA_OUT = 0;
        DA_OE = 0;
        
        if (!NE1 && !NOE)
            begin
            if(Faddress[21:1] == QADDR_IR[21:1])
                begin
                DA_OUT = {12'b0, SA_WrittenR, SA_ReadR, IR_WrittenR, IR_ReadR};    // Drive the AD bus with register data
                DA_OE = 16'hFFFF;
                end
            else if(Faddress[21:1] == QADDR_SA[21:1])
                begin
                DA_OUT = SA_Address;    // Drive the AD bus with register data
                DA_OE = 16'hFFFF;
                end
            end
        end



    // Capture QBus address and related info at leading edge of BSYNC
    always_ff @(negedge BSYNCf or negedge BINITf)
        begin
        if(!BINITf)
            begin
            Q_IR_selected <= 0;
            Q_SA_selected <= 0;
            Qaddress0 <= 0;
            end
        else
            begin
            Q_IR_selected <= !BBS7f && ~BDALf_IN[12:1] == QADDR_IR[12:1];
            Q_SA_selected <= !BBS7f && ~BDALf_IN[12:1] == QADDR_SA[12:1];
            Qaddress0 <= ~BDALf_IN[0];
            end
        end

    // QBus write
    always_ff @(posedge BDOUTf or posedge F_IR_read_selected)
        begin
        if (F_IR_read_selected)
            begin
            IR_Written <= 0;
            SA_Written <= 0;
            end
        else if (Q_IR_selected)
            begin
            IR_Written <= 1;
            end
        else if (Q_SA_selected)
            begin
            if (BWTBTf || Qaddress0 == 0) SA_Address[7:0]  <= ~BDALf_IN[7:0];   // Byte 0 write
            if (BWTBTf || Qaddress0 == 1) SA_Address[15:8] <= ~BDALf_IN[15:8];  // Byte 1 write
            SA_Written <= 1;
            end
        end

    // QBus read, clocked part
    always_ff @(posedge BDINf or posedge F_IR_read_selected)
        begin
        if (F_IR_read_selected)
            begin
            IR_Read <= 0;
            SA_Read <= 0;
            end
        else if (Q_IR_selected)
            begin
            IR_Read <= 1;
            end
        else if (Q_SA_selected)
            begin
            SA_Read <= 1;
            end
        end

    // Qbus read operation, and write BRPLY, combinatorial part
    always_comb
        begin

        BRPLYg = 0;
        BDALf_OUT = 22'h000000;
        BDALf_OE  = 22'h000000;                         // disable the FPGA bus drivers by default
        Outbound = 0;                                   // disable the BDAL gate drivers by default
        
        // Qbus read operation
        if (Q_IR_selected && !BDINf)
            begin
            BDALf_OE = 22'h3FFFFF;                      // enable the FPGA bus drivers to output the data
            Outbound = 1;                               // enable the gate drivers
            end
        // Qbus read operation
        else if (Q_SA_selected && !BDINf)
            begin
            BDALf_OUT[21:18] = 4'b0000;
            BDALf_OUT[17] = 0;                          // memory parity error enable
            BDALf_OUT[16] = 0;                          // memory parity error
            BDALf_OUT[15:0] = SA_Status;                // Drive the BDALf bus with register data
            BDALf_OE = 22'h3FFFFF;                      // enable the FPGA bus drivers to output the data
            Outbound = 1;                               // enable the gate drivers
            end
            
        // assert BRPLY as needed
        if ((Q_IR_selected || Q_SA_selected) && (!BDINf || !BDOUTf))
            begin
            BRPLYg = 1;                                 // assert the reply signal
            end

        end          


    
    // do something silly with the clock to make sure it is not optimized away
    // so we can use it for the logic analyzer
    always_ff @(posedge clock)
        begin
        dummy <= !dummy;
        end


endmodule
