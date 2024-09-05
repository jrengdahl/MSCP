module DFFR (
    input logic clk,     // Clock input
    input logic reset,   // Asynchronous reset input, active low
    input logic D,       // Data input
    output logic Q       // Data output
);

    always_ff @(posedge clk or posedge reset)
        begin
        if (reset) Q <= 1'b0;   // Asynchronously reset Q to 0
        else       Q <= D;      // On clock edge, assign D to Q
        end

endmodule

module SRFF (
    input logic set,     // Asynchronous set input, high-true
    input logic reset,   // Asynchronous reset input, high-true
    output logic Q       // Data output
);

    always @*
        begin
        if (reset)    Q = 1'b0;   // Asynchronously reset Q to 0
        else if (set) Q = 1'b1;   // Asynchronously set Q to 1
        end

endmodule



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
    input logic BDMGIf,
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
    output wire BSACKg,
    output logic BDMGOg,
    output logic BREFg,

    // other
    output logic FPGA_IRQ,          // interrupt from FPGA to H723
    output logic LED,
    output logic PLL_RSTN,
    output logic dummy
    );

    // address of registers as seen from the H723
    parameter [21:0] FADDR_IR   = 0;
    parameter [21:0] FADDR_SA   = 2;
    parameter [21:0] FADDR_CT   = 4;
    parameter [21:0] FADDR_LO   = 6;
    parameter [21:0] FADDR_HI   = 8;
    parameter [21:0] FADDR_DATA_OUT = 10;
    parameter [21:0] FADDR_DATA_IN = 12;

    // addresses of registers as seen from the PDP-11
    parameter [21:0] QADDR_IR = 22'o17772150;
    parameter [21:0] QADDR_SA = 22'o17772152;

    parameter [21:0] QADDR_ROM = 22'o17771000;      // this is the alternate boot ROM address, since 17773000 it taken by the KDJ11's on-board ROM
    
    // RQDX3 compatible registers
    logic [15:0] SA_Status;         // status read by PDP-11, written by H723
    logic [15:0] SA_Address;        // address written by PDP-11, read by H723


    // registers used by the H723 to access Qbus via DMA
    logic [14:0] Q_Ctl;             // bus control, written by H723
    logic [21:0] Q_Addr;            // bus address
    logic [15:0] Q_Data_out;        // bus data output
    
    
    // status bits set by PDP-11 activity, read by the H723
    logic IP_Read;                  // IP register has been read by PDP-11 (poll queue)
    logic IP_Written;               // IP register has been written by PDP-11 (init)
    logic SA_Read;                  // SA register has been read by PDP-11
    logic SA_Written;               // SA register has been written by PDP-11
    logic BRPLY_Asserted;           // BRPLYL has been asserted
    logic BRPLY_Deasserted;         // BRPLY has been deasserted
    
    // latched version of the above for read-and-clear
    logic IP_ReadR;                 //
    logic IP_WrittenR;              //
    logic SA_ReadR;                 //
    logic SA_WrittenR;              //
    logic BRPLY_AssertedR;          //
    logic BRPLY_DeassertedR;        //

    // Both the Qbus and FMC bus have multiplexed address/data
    // these are the addresses latched during the address phase of a bbus cycle for both busses
    logic [23:0] Faddress;          // Latch for the FMC address
    logic Q_IP_selected;            // latches whether the IR register is addressed by the Qbus
    logic Q_SA_selected;            // latches whether the SA register is addressed by the Qbus
    logic Q_ROM_selected;           // latches whether the SA register is addressed by the Qbus
    logic [12:0] Qaddress;          // latches the low bits of the Qbus address

    logic [15:0] ROMdata;           // data from the boot ROM
    

    // interrupt the H723 if the PDP-11 has read or written any register
    assign FPGA_IRQ = IP_Read || IP_Written || SA_Read || SA_Written;

    // IP is being read by H723
    wire F_IP_read_enable = !NE1 && !NOE && Faddress[21:0] == FADDR_IR[21:0];
  

    assign BIAKOg = 0;

    assign PLL_RSTN = 1;            // the PLL needs this held high
    
    
    
    // Instantiate the BootRom module
    BootRom u_BootRom (
        .addr(Qaddress[8:1]),           // Connect addr signal
        .rdata_a(ROMdata[15:0]),        // Connect rdata_a signal
        .clk(clock)                     // Connect clk signal
    );

    
///////////////////////////////////////////
///
///  FMC operations
///
///////////////////////////////////////////
    
    
    // Latch the FMC address on NL
    always_ff @(posedge NL)
        begin
        Faddress <= {A, DA_IN, !NBL1 && NBL0};
        end

    // FMC write
    always_ff @(posedge NWE or posedge IP_Written)
        begin
        if(IP_Written)
            begin
                SA_Status <= 0;
                Q_Ctl <= 0;
                Q_Addr <= 0;
                Q_Data_out <= 0;
            end
        else if (!NE1 && Faddress[21:1] == FADDR_SA[21:1])
            begin
            if (!NBL0) SA_Status[7:0]  <= DA_IN[7:0];   // Byte 0 write
            if (!NBL1) SA_Status[15:8] <= DA_IN[15:8];  // Byte 1 write
            end
        else if (!NE1 && Faddress[21:1] == FADDR_CT[21:1])
            begin
            if (!NBL0) Q_Ctl[7:0]  <= DA_IN[7:0];   // Byte 0 write
            if (!NBL1) Q_Ctl[14:8] <= DA_IN[14:8];  // Byte 1 write
            end
        else if (!NE1 && Faddress[21:1] == FADDR_LO[21:1])
            begin
            if (!NBL0) Q_Addr[7:0]  <= DA_IN[7:0];   // Byte 0 write
            if (!NBL1) Q_Addr[15:8] <= DA_IN[15:8];  // Byte 1 write
            end
        else if (!NE1 && Faddress[21:1] == FADDR_HI[21:1])
            begin
            if (!NBL0) Q_Addr[21:16]  <= DA_IN[5:0];   // Byte 0 write
            end
        else if (!NE1 && Faddress[21:1] == FADDR_DATA_OUT[21:1])
            begin
            if (!NBL0) Q_Data_out[7:0]  <= DA_IN[7:0];   // Byte 0 write
            if (!NBL1) Q_Data_out[15:8] <= DA_IN[15:8];  // Byte 1 write
            end
        end

    // FMC read
    
    always_ff @(posedge NL) // latch the status bits at the beginning of any cycle
        begin
        IP_ReadR <= IP_Read;
        IP_WrittenR <= IP_Written;
        SA_ReadR <= SA_Read;
        SA_WrittenR <= SA_Written;
        BRPLY_AssertedR <= BRPLY_Asserted;
        BRPLY_DeassertedR <= BRPLY_Deasserted;
        end
    
    always_comb
        begin
        DA_OUT = 0;
        DA_OE = 0;
        
        if (!NE1 && !NOE)
            begin
            DA_OE = 16'hFFFF;

            if(Faddress[21:1] == FADDR_IR[21:1])
                begin
                DA_OUT = {8'b0,     // Drive the AD bus with register data
                        BSACKg,
                        BRPLY_DeassertedR,
                        BRPLY_AssertedR,
                        ~BRPLYf,
                        SA_WrittenR,
                        SA_ReadR, 
                        IP_WrittenR, 
                        IP_ReadR};
                end
            else if(Faddress[21:1] == FADDR_SA[21:1])
                begin
                DA_OUT = SA_Address;    // Drive the AD bus with register data
                end
            else if(Faddress[21:1] == FADDR_CT[21:1])
                begin
                DA_OUT = {1'b0, Q_Ctl};    // Drive the AD bus with register data
                end
            else if(Faddress[21:1] == FADDR_LO[21:1])
                begin
                DA_OUT = Q_Addr[15:0];    // Drive the AD bus with register data
                end
            else if(Faddress[21:1] == FADDR_HI[21:1])
                begin
                DA_OUT = {10'b0, Q_Addr[21:16]};    // Drive the AD bus with register data
                end
            else if(Faddress[21:1] == FADDR_DATA_OUT[21:1])
                begin
                DA_OUT = Q_Data_out;    // Drive the AD bus with register data
                end
            else if(Faddress[21:1] == FADDR_DATA_IN[21:1])
                begin
                DA_OUT = ~BDALf_IN;    // Drive the AD bus directly from the Qbus data
                end
            end
        end

///////////////////////////////////////////
///
///  QBUS operations as bus slave
///
///////////////////////////////////////////

    wire Clear_SA = Q_Ctl[12];

    // Capture QBus address and related info at leading edge of BSYNC
    always_ff @(negedge BSYNCf or negedge BINITf)
        begin
        if(!BINITf)
            begin
            Q_IP_selected   <= 0;
            Q_SA_selected   <= 0;
            Q_ROM_selected  <= 0;
            Qaddress        <= 0;
            end
        else
            begin
            Q_IP_selected  <= !BBS7f && ~BDALf_IN[12:1] == QADDR_IR[12:1];
            Q_SA_selected  <= !BBS7f && ~BDALf_IN[12:1] == QADDR_SA[12:1];
            Q_ROM_selected <= !BBS7f && ~BDALf_IN[12:9] == QADDR_ROM[12:9];
            Qaddress       <=           ~BDALf_IN[12:0];
            end
        end

    // QBus write
    always_ff @(posedge BDOUTf or posedge F_IP_read_enable)
        begin
        if (F_IP_read_enable)
            begin
            IP_Written <= 0;
            SA_Written <= 0;
            if(Clear_SA)
                begin
                SA_Address <= 0;
                end
            end
        else if (Q_IP_selected)
            begin
            IP_Written <= 1;
            end
        else if (Q_SA_selected)
            begin
            if (BWTBTf || Qaddress[0] == 0) SA_Address[7:0]  <= ~BDALf_IN[7:0];   // Byte 0 write
            if (BWTBTf || Qaddress[0] == 1) SA_Address[15:8] <= ~BDALf_IN[15:8];  // Byte 1 write
            SA_Written <= 1;
            end
        end

    // QBus read, clocked part
    always_ff @(posedge BDINf or posedge F_IP_read_enable)
        begin
        if (F_IP_read_enable)
            begin
            IP_Read <= 0;
            SA_Read <= 0;
            end
        else if (Q_IP_selected)
            begin
            IP_Read <= 1;
            end
        else if (Q_SA_selected)
            begin
            SA_Read <= 1;
            end
        end


    wire Q_Addr_enable = Q_Ctl[10];
    wire Q_Data_enable = Q_Ctl[11];
    wire OutGateBegin = !BDINf && !BRPLYf;
    logic OutGate;                         // used to enable slave outputs to Qbus
    
    // OutGate begins at BDIN low and ends and BRPLY high
    always_ff @(posedge BRPLYf or posedge OutGateBegin)
        begin
        if(OutGateBegin)  OutGate <= 1;
        else              OutGate <= 0;
        end

    
    // This section handles data transmitted on qbus by this module
    // whether the module as a slave is read by the PDP-11,
    // or the module as a Qbus master intiates a DMA transaction to the PDP-11 system memory
    always_comb
        begin

        BDALf_OUT = 22'h000000;
        BDALf_OE  = 22'h000000;                         // disable the FPGA bus drivers by default
        Outbound = 0;                                   // disable the BDAL gate drivers by default
        
        // transactions performed as bus master
        // The timing of master transactions is controlled by the H723 by writing to the Q-Ctl register
        if (BSACKg)
            begin
            // output the address during the address portion of any cycle
            if (Q_Addr_enable)
                begin
                BDALf_OUT = Q_Addr;
                BDALf_OE = 22'h3FFFFF;                      // enable the FPGA bus drivers to output the data
                Outbound = 1;                               // enable the gate drivers
                end
            // output the data during the data portion of a write cycle
            else if(Q_Data_enable)
                begin
                BDALf_OUT[21:18] = 4'b0000;
                BDALf_OUT[17] = 0;                          // memory parity error enable
                BDALf_OUT[16] = 0;                          // memory parity error
                BDALf_OUT[15:0] = Q_Data_out;               // Drive the BDALf bus with register data
                BDALf_OE = 22'h3FFFFF;                      // enable the FPGA bus drivers to output the data
                Outbound = 1;                               // enable the gate drivers
                end
            end

        // transactions performed as bus slave
        // Qbus read of IR register
        else if (Q_IP_selected && OutGate)
            begin
            BDALf_OE = 22'h3FFFFF;                          // enable the FPGA bus drivers to output the data
            Outbound = 1;                                   // enable the gate drivers
            end
            
        // Qbus read of SA register
        else if (Q_SA_selected && OutGate)
            begin
            BDALf_OUT[21:18] = 4'b0000;
            BDALf_OUT[17] = 0;                              // memory parity error enable
            BDALf_OUT[16] = 0;                              // memory parity error
            BDALf_OUT[15:0] = SA_Status;                    // Drive the BDALf bus with register data
            BDALf_OE = 22'h3FFFFF;                          // enable the FPGA bus drivers to output the data
            Outbound = 1;                                   // enable the gate drivers
            end
            
        // Qbus read of boot ROM
        else if (Q_ROM_selected && OutGate)
            begin
            BDALf_OUT[21:18] = 4'b0000;
            BDALf_OUT[17] = 0;                              // memory parity error enable
            BDALf_OUT[16] = 0;                              // memory parity error
            BDALf_OUT[15:0] = ROMdata[15:0];                // Drive the BDALf bus with boot ROM data
            BDALf_OE = 22'h3FFFFF;                          // enable the FPGA bus drivers to output the data
            Outbound = 1;                                   // enable the gate drivers
            end
            
        end          

        
    // assert BRPLY as needed
    assign BRPLYg = !BSACKg && (Q_IP_selected || Q_SA_selected || Q_ROM_selected) && (!BDINf || !BDOUTf);

    
    
///////////////////////////////////////////
///
///  Qbus operations as bus master
///
///////////////////////////////////////////


    // the Q_Ctl register enables the H723 to control the bus master outputs
    assign BSYNCg  = Q_Ctl[0];
    assign BDINg   = Q_Ctl[1];
    assign BDOUTg  = Q_Ctl[2];
    assign BWTBTg  = Q_Ctl[3];
    assign BDMRg   = Q_Ctl[4];
    assign BREFg   = Q_Ctl[5];
    assign BBS7g   = Q_Ctl[6];
    assign BIRQ4g  = Q_Ctl[7];
    assign BIRQ5g  = Q_Ctl[8];
    assign BIRQ6g  = Q_Ctl[9];

    
    // Detect assertion of BRPLYL
    always_ff @(negedge BRPLYf or posedge F_IP_read_enable)
        begin
        if (F_IP_read_enable)   BRPLY_Asserted <= 0;
        else                    BRPLY_Asserted <= 1;
        end

    // Detect deassertion of BRPLYL
    always_ff @(posedge BRPLYf or posedge F_IP_read_enable)
        begin
        if (F_IP_read_enable)   BRPLY_Deasserted <= 0;
        else                    BRPLY_Deasserted <= 1;
        end

        
    
    // When a DMA grant in is received,
    // and DMA has not been requested,
    // and DMA is not already in progress,
    // then relay the grant on down the chain.
    
    // When a DMA grant in is received,
    // and DMA has been requested,
    // and DMA is not already in progress,
    // then assert DMA_grant. The leading edge of this
    // signal is used a few lines later to set the BSACKg flip-flip.
    

    assign BDMGOg = !BDMGIf && !BDMRg && !BSACKg;
    wire DMA_grant = !BDMGIf && BDMRg && BSYNCf && BRPLYf;
    wire DMA_done = !NE1 && !NWE && Faddress[21:1] == FADDR_CT[21:1] && !NBL1 && DA_IN[15];

    // when DMA_grant is received, assert BSACKL, and keep on asserting it until the H723 says DMA is done
    SRFF BSACK_inst (.set(DMA_grant), .reset(DMA_done), .Q(BSACKg));


     
        
///////////////////////////////////////////
///
///  Debug stuff
///
///////////////////////////////////////////



    assign LED = SA_Address[0];     // light LED for testing
       
    // do something silly with the clock to make sure it is not optimized away
    // so we can use it for the logic analyzer
    always_ff @(posedge clock)
        begin
        dummy <= !dummy;
        end


endmodule
