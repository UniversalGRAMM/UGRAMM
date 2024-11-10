#-------------------------------------------
#  UGRAMM    :: helper script
#  Author    :: Omkar Bhilare
#  Email-ID  :: omkar.bhilare@mail.utoronto.ca
#--------------------------------------------

# Required packages:
import argparse
import numpy as np
import networkx as nx
import matplotlib.pyplot as plt
from networkx.drawing.nx_agraph import graphviz_layout
import math

#Default Width
width = 32

def get_node_name(node_number, nr, nc):
    # Initialize the node counter
    node_counter = 0
    
    # Calculate total IOs and PEs
    num_ios = nr * 2
    num_pes = nr * nc

    # Assign ranges for IOs
    io_ranges = []
    for io in range(1, num_ios + 1):
        io_start = node_counter
        io_end = io_start + 2  # each IO has 3 nodes (0-2)
        io_ranges.append((f"IO{io}", io_start, io_end))
        node_counter = io_end + 1

    # Assign ranges for PEs
    pe_ranges = []
    for pe in range(1, num_pes + 1):
        pe_start = node_counter
        pe_end = pe_start + 17  # each PE has 18 nodes (0-17)
        pe_ranges.append((f"PE{pe}", pe_start, pe_end))
        node_counter = pe_end + 1

    # Search through IO ranges
    for io_name, io_start, io_end in io_ranges:
        if io_start <= node_number <= io_end:
            return io_name

    # Search through PE ranges
    for pe_name, pe_start, pe_end in pe_ranges:
        if pe_start <= node_number <= pe_end:
            return pe_name

    # If no matching range is found
    return "Node not found"

def print_io_pe_grid(nr, nc):
    # Calculate total IOs and PEs
    num_ios = nr * 2
    num_pes = nr * nc

    # Initialize node counter
    node_counter = 0

    # Assign ranges for IOs
    io_ranges = []
    for io in range(1, num_ios + 1):
        io_start = node_counter
        io_end = io_start + 2  # each IO has 3 nodes (0-2)
        io_ranges.append((f"IO{io}", io_start, io_end))
        node_counter = io_end + 1

    # Assign ranges for PEs
    pe_ranges = []
    for pe in range(1, num_pes + 1):
        pe_start = node_counter
        pe_end = pe_start + 17  # each PE has 18 nodes (0-17)
        pe_ranges.append((f"PE{pe}", pe_start, pe_end))
        node_counter = pe_end + 1

    # Print the ranges in the specified grid format
    pe_index = 0  # Track current PE in the pe_ranges list
    left_io_index = 0  # Track IOs for the left side (first half of IOs)
    right_io_index = nr  # Track IOs for the right side (second half of IOs)

    for row in range(nr):
        # Print left IO for the row
        io_left = io_ranges[left_io_index]
        left_io_index += 1
        print(f"{io_left[0]}: [{io_left[1]}-{io_left[2]}]", end=" ")

        # Print all PEs in this row
        for col in range(nc):
            pe = pe_ranges[pe_index]
            pe_index += 1
            print(f"{pe[0]}: [{pe[1]}-{pe[2]}]", end=" ")

        # Print right IO for the row
        io_right = io_ranges[right_io_index]
        right_io_index += 1
        print(f"{io_right[0]}: [{io_right[1]}-{io_right[2]}]")


def create_riken(args):
    
    #------------------------ Riken Config ----------------------------
  

    #------------------------------------------------
    #Maximum outputs of switchblock:
    #------------------------------------------------
    '''
                         up_sb (4)
                             |
                         |-------|
        left_sb (2) <----|   SB  |---> right_sb (6)
                         |-------|
                             |
                         down_sb (0)
    '''
    sb_max = 10         #This riken architecture has maximum 10 outputs
    left_sb  = 2
    right_sb = 6
    up_sb    = 4
    down_sb  = 5    
    
    #Each PE+SB has 17 nodes inside in it which contains blocks such as mux, ALU, SB, constant etc.
    #The first PE index number starts after the left and right most IO connection.
    IOStep  = 3
    PEstep  = 18
    Number_of_PEs = args.NR * args.NC
    Number_of_IOs = args.NR * 2
    PEstart =  (Number_of_IOs*3)  # x2 --> Right and Left IOs, x2 --> Input port of IOs, x2 --> Out port of IOs 
    total_nodes_riken = (Number_of_IOs + (PEstep*args.NR*args.NC)) 
    
    #print_io_pe_grid(args.NR, args.NC)

    #------------------------------------------------------------------
    #------------------------------------------------------------------
    
    G = nx.DiGraph()    #Creating a directed graph using networkx

    #--------------------------------------------------------
    #Step 0: Adding total number of nodes into Dir-Graph:
    #--------------------------------------------------------
    G.add_nodes_from(range(0,total_nodes_riken))
    #print(list(G.nodes))

    vertical_gap = 0.3

    #----------------------------------------------------------------
    #Step 1: Add the connection to the Load/Store units on both side
    #----------------------------------------------------------------
    for i in range(args.NR):

        #---------------------------------------------------------------
        #Left-IO:
        #---------------------------------------------------------------

        left_io_index        = i*3
        left_io_inPin_index  = left_io_index + 1
        left_io_outPin_index = left_io_index + 2

        #Left-IO:
        io_gap = 1
        left_io_output_pin_scalar = (vertical_gap + io_gap)*i
        left_io_input_pin_scalar  = left_io_output_pin_scalar + 0.8
        left_io_pin_scalar = left_io_output_pin_scalar + 0.4

        left_io_base_name = "LS.w32.c0.r" + str(i)  + ".memport"
        G.nodes[left_io_index]["G_Name"]             = left_io_base_name
        G.nodes[left_io_index]["G_CellType"]         = "FuncCell"     
        G.nodes[left_io_index]["G_NodeType"]         = "MemPort"     
        G.nodes[left_io_index]["G_VisualX"]          = str(0)
        G.nodes[left_io_index]["G_VisualY"]          = str(left_io_pin_scalar)       
        G.nodes[left_io_index]["G_Width"]            = str(width)       
        
        #Left-IO-input-Pin :
        G.nodes[left_io_inPin_index]["G_Name"]             = left_io_base_name + ".inPinA"
        G.nodes[left_io_inPin_index]["G_CellType"]         = "PinCell"   
        G.nodes[left_io_inPin_index]["G_NodeType"]         = "in"      
        G.nodes[left_io_inPin_index]["G_VisualX"]          = str(0)
        G.nodes[left_io_inPin_index]["G_VisualY"]          = str(left_io_input_pin_scalar)   
        G.nodes[left_io_inPin_index]["G_Width"]                  = str(width)

        #Left-IO-output-Pin :
        G.nodes[left_io_outPin_index]["G_Name"]             = left_io_base_name + ".outPinA"
        G.nodes[left_io_outPin_index]["G_CellType"]         = "PinCell"   
        G.nodes[left_io_outPin_index]["G_NodeType"]         = "out"      
        G.nodes[left_io_outPin_index]["G_VisualX"]          = str(0)
        G.nodes[left_io_outPin_index]["G_VisualY"]          = str(left_io_output_pin_scalar)   
        G.nodes[left_io_outPin_index]["G_Width"]            = str(width) 

        #----------------------------------
        #Connectivity of Left IO port and Pin cell:
        #----------------------------------
        #bi-directional edge between left io and the pin cell
        G.add_edge(left_io_index, left_io_outPin_index)
        G.add_edge(left_io_inPin_index, left_io_index)

        #---------------------------------------------------------------
        #Right-IO:
        #---------------------------------------------------------------
        right_io_output_pin_scalar = (vertical_gap + io_gap)*i
        right_io_input_pin_scalar  = right_io_output_pin_scalar + 0.8
        right_io_pin_scalar = right_io_output_pin_scalar + 0.4

        right_io_index          = (i*3) + (args.NR*3)
        right_io_inPin_index    = right_io_index + 1
        right_io_outPin_index   = right_io_index + 2
        
        right_io_base_name = "LS.w32.c" + str(args.NC+1) + ".r" + str(i) + ".memport"
        G.nodes[right_io_index]["G_Name"]     = right_io_base_name      #Right-IOs       
        G.nodes[right_io_index]["G_CellType"] = "FuncCell"              #Right-IOs
        G.nodes[right_io_index]["G_NodeType"] = "MemPort"               #Right-IOs
        G.nodes[right_io_index]["G_VisualX"]  = str(args.NC+1)
        G.nodes[right_io_index]["G_VisualY"]  = str(right_io_pin_scalar)     
        G.nodes[right_io_index]["G_Width"]    = str(width) 

        #Right-IO-input-Pin :
        G.nodes[right_io_inPin_index]["G_Name"]             = right_io_base_name + ".inPinA"
        G.nodes[right_io_inPin_index]["G_CellType"]         = "PinCell"   
        G.nodes[right_io_inPin_index]["G_NodeType"]         = "in"      
        G.nodes[right_io_inPin_index]["G_VisualX"]          = str(args.NC+1)
        G.nodes[right_io_inPin_index]["G_VisualY"]          = str(right_io_input_pin_scalar)   
        G.nodes[right_io_inPin_index]["G_Width"]            = str(width) 

        #Right-IO-output-Pin :
        G.nodes[right_io_outPin_index]["G_Name"]             = right_io_base_name + ".outPinA"
        G.nodes[right_io_outPin_index]["G_CellType"]         = "PinCell"   
        G.nodes[right_io_outPin_index]["G_NodeType"]         = "out"      
        G.nodes[right_io_outPin_index]["G_VisualX"]          = str(args.NC+1)
        G.nodes[right_io_outPin_index]["G_VisualY"]          = str(right_io_output_pin_scalar)   
        G.nodes[right_io_outPin_index]["G_Width"]            = str(width) 

        #print(f"left_io_index {left_io_index} for {left_io_base_name} | right_io_index {right_io_index} for {right_io_base_name}")
        #print(f'Left pin: {G.nodes[left_io_outPin_index]["G_VisualX"]}, {G.nodes[left_io_outPin_index]["G_VisualY"]} || Right pin: {G.nodes[right_io_outPin_index]["G_VisualX"]}, {G.nodes[right_io_outPin_index]["G_VisualY"]}')
        
        #----------------------------------
        #Connectivity of Right IO port and Pin cell:
        #----------------------------------
        G.add_edge(right_io_index, right_io_outPin_index)
        G.add_edge(right_io_inPin_index, right_io_index)

        #----------------------------------
        #Connectivity of IO ports and SBs:
        #----------------------------------

        left_adjacent_pe_number  = PEstart+ i*args.NC*PEstep
        right_adjacent_pe_number = PEstart+ i*args.NC*PEstep + (args.NC-1)*PEstep
        #print(f"left_io_index {left_io_index} for PE {left_adjacent_pe_number} | right_io_index {right_io_index} for PE {right_adjacent_pe_number}")

        #Left-most column:
        for j in range(sb_max):
            if (j == left_sb):
                continue
            #Edge from current IO out-pin to adjacent SB's pins
            G.add_edge(left_io_outPin_index, left_adjacent_pe_number + j)  

            #Edge from current IO input-pin to adjacent SB's pins  
            #G.add_edge(left_adjacent_pe_number + j, left_io_inPin_index)  # NEEDs to be removed

        #Edge from adjacent closest SB port to IO input-pin
        G.add_edge(left_adjacent_pe_number + left_sb, left_io_inPin_index)  

        #Edge from adjacent closest SB port to IO output-pin
        #G.add_edge(left_io_outPin_index, left_adjacent_pe_number + left_sb) # NEEDs to be removed

        #Right-most column:
        for j in range(sb_max):
            if (j == right_sb):
                continue
            #Edge from current IO out-pin to adjacent SB's pins
            G.add_edge(right_io_outPin_index, right_adjacent_pe_number + j)    

            #Edge from current IO input-pin to adjacent SB's pins  
            #G.add_edge(right_adjacent_pe_number + j, right_io_inPin_index) # NEEDs to be removed

        #Edge from adjacent closest SB port to IO input-pin
        G.add_edge(right_adjacent_pe_number + right_sb, right_io_inPin_index)   

        #Edge from adjacent closest SB port to IO output-pin
        #G.add_edge(right_io_outPin_index, right_adjacent_pe_number + right_sb)   # NEEDs to be removed

    #--------------------------------------------------------
    #Step 2: Add connection between Switch Blocks
    #--------------------------------------------------------
    '''
               N
            NW | NE
          W ---+--- E
            SW | SE
               S
    '''
    for i in range(args.NR):
        for j in range(args.NC):

            dest_row = 0
            dest_col = 0
            source_pe = PEstart + i*args.NC*PEstep + j*PEstep
            
            #--------------------------
            #         SOUTH
            #--------------------------
            if (i < (args.NR - 1)):                   #South connections are not present for the last-row
                dest_row = i+1
                dest_col = j
                target_pe = PEstart + dest_row*args.NC*PEstep + dest_col*PEstep
                #print(f'{source_pe} | {get_node_name(source_pe, args.NR, args.NC)} -> {target_pe} | {get_node_name(target_pe, args.NR, args.NC)}')
                # Connecting current SB (I, J) to SB position on south direction.
                # Connecting 0th pin of current SB to all other pins of the destination SB.
                for k in range(sb_max):
                    if (k == up_sb):
                        continue
                    G.add_edge( source_pe + 0, 
                        target_pe + k)
                    
            #--------------------------
            #       SOUTH-WEST
            #--------------------------
            if ((i < (args.NR - 1)) and (j > 0)):       #South-WEST connections are not present for first column and last row
                dest_row = i+1
                dest_col = j-1
                target_pe = PEstart + dest_row*args.NC*PEstep + dest_col*PEstep
                #print(f'{source_pe} | {get_node_name(source_pe, args.NR, args.NC)} -> {target_pe} | {get_node_name(target_pe, args.NR, args.NC)}')
                # Connecting current SB (I, J) to SB position on SOUTH-WEST direction.
                # Connecting 1st pin of current SB to all other pins of the destination SB.
                for k in range(sb_max):
                    if (k == 5):
                        continue
                    G.add_edge( source_pe + 1, 
                        target_pe + k)
            
            #--------------------------
            #         WEST
            #--------------------------
            if (j > 0):                                #WEST connections are not present for the first column
                dest_row = i
                dest_col = j-1
                target_pe = PEstart + dest_row*args.NC*PEstep + dest_col*PEstep
                #print(f'{source_pe} | {get_node_name(source_pe, args.NR, args.NC)} -> {target_pe} | {get_node_name(target_pe, args.NR, args.NC)}')
                for k in range(sb_max):
                    if (k == 6):
                        continue
                    G.add_edge( source_pe + 2, 
                        target_pe + k)            

            #--------------------------
            #       NORTH-WEST
            #-------------------------- 
            if ((i > 0) and (j > 0)):                  #NORTH-WEST connections are not present for first row and column
                dest_row = i-1
                dest_col = j-1
                target_pe = PEstart + dest_row*args.NC*PEstep + dest_col*PEstep
                #print(f'{source_pe} | {get_node_name(source_pe, args.NR, args.NC)} -> {target_pe} | {get_node_name(target_pe, args.NR, args.NC)}')
                for k in range(sb_max):
                    if (k == 7):
                        continue
                    G.add_edge( source_pe + 3, 
                        target_pe + k)
            
            #--------------------------
            #         NORTH
            #--------------------------
            if (i > 0):                               #NORTH connections are not present for the first row
                dest_row = i-1
                dest_col = j
                target_pe = PEstart + dest_row*args.NC*PEstep + dest_col*PEstep
                #print(f'{source_pe} | {get_node_name(source_pe, args.NR, args.NC)} -> {target_pe} | {get_node_name(target_pe, args.NR, args.NC)}')
                for k in range(sb_max):
                    if (k == 0):
                        continue
                    G.add_edge( source_pe + 4, 
                        target_pe + k)

            #--------------------------
            #       NORTH-EAST
            #--------------------------
            if ((i > 0) and (j < (args.NC-1))):        #NORTH-EAST connections are not present for first row and last column
                dest_row = i-1
                dest_col = j+1
                target_pe = PEstart + dest_row*args.NC*PEstep + dest_col*PEstep
                #print(f'{source_pe} | {get_node_name(source_pe, args.NR, args.NC)} -> {target_pe} | {get_node_name(target_pe, args.NR, args.NC)}')
                for k in range(sb_max):
                    if (k == 1):
                        continue
                    G.add_edge( source_pe + 5, 
                        target_pe + k)               

            #--------------------------
            #         EAST
            #--------------------------
            if ((j < (args.NC-1))):                   #EAST connections are not present for last column
                dest_row = i
                dest_col = j+1
                target_pe = PEstart + dest_row*args.NC*PEstep + dest_col*PEstep
                #print(f'{source_pe} | {get_node_name(source_pe, args.NR, args.NC)} -> {target_pe} | {get_node_name(target_pe, args.NR, args.NC)}')
                for k in range(sb_max):
                    if (k == 2):
                        continue
                    G.add_edge( source_pe + 6, 
                        target_pe + k)

            #--------------------------
            #       SOUTH-EAST
            #--------------------------
            if ((i < (args.NR - 1)) and (j < (args.NC-1))): #SOUTH-EAST connections are not present for last row & column
                dest_row = i+1
                dest_col = j+1
                target_pe = PEstart + dest_row*args.NC*PEstep + dest_col*PEstep
                #print(f'{source_pe} | {get_node_name(source_pe, args.NR, args.NC)} -> {target_pe} | {get_node_name(target_pe, args.NR, args.NC)}')
                for k in range(sb_max):
                    if (k == 3):
                        continue
                    G.add_edge( source_pe + 7, 
                        target_pe + k)

    #--------------------------------------------------------
    #Step 3: Intra-PE connectivity
    #           &
    #Step 4: Assigning the Node type attribute
    #--------------------------------------------------------
    for i in range(args.NR):
        for j in range(args.NC):

            PEindex = PEstart + i*args.NC*PEstep + j*PEstep

            # Current PE number:
            current_pe_number = int((PEindex - PEstart)/PEstep)
            x_coordinate = j+1
            y_coordinate = i
            #print(f"{x_coordinate},{y_coordinate} | {current_pe_number} | {PEindex} | {get_node_name(source_pe, args.NR, args.NC)}")

            sb_pe_mux_a_offest = 8       
            sb_pe_mux_b_offset = 9
            pe_mux_a_offset    = 10      
            pe_mux_b_offset    = 11
            pe_const_offset    = 12
            pe_const_pinO_offset = 13
            pe_alu_offset         = 14
            pe_alu_pinA_offset    = 15
            pe_alu_pinB_offset    = 16
            pe_alu_pinO_offset    = 17

            #-------------------------------------------------------
            #-------------------------------------------------------
            # SB input to the pe muxes
            G.add_edge( PEindex + sb_pe_mux_a_offest, PEindex + pe_mux_a_offset)    # sb_pe_mux_a_offest -> pe_mux_a_offset
            G.add_edge( PEindex + sb_pe_mux_b_offset, PEindex + pe_mux_b_offset)    # sb_pe_mux_b_offset -> pe_mux_b_offset
            #-------------------------------------------------------

            #-------------------------------------------------------
            #-------------------------------------------------------
            # PE const to the PE const-pin
            G.add_edge( PEindex + pe_const_offset, PEindex + pe_const_pinO_offset)    # pe_const_offset -> pe_const_pinO_offset
            #-------------------------------------------------------

            #-------------------------------------------------------
            #-------------------------------------------------------
            # PE const-pin to the PE_Muxes
            G.add_edge( PEindex + pe_const_pinO_offset, PEindex + pe_mux_a_offset)    # pe_const_pinO_offset -> pe_mux_a_offset
            G.add_edge( PEindex + pe_const_pinO_offset, PEindex + pe_mux_b_offset)    # pe_const_pinO_offset -> pe_mux_b_offset
            #-------------------------------------------------------

            #-------------------------------------------------------
            #-------------------------------------------------------
            # PE_Muxes to the Pins of ALU
            G.add_edge( PEindex + pe_mux_a_offset, PEindex + pe_alu_pinA_offset)    # pe_mux_a_offset -> pe_alu_pinA_offset
            G.add_edge( PEindex + pe_mux_b_offset, PEindex + pe_alu_pinB_offset)    # pe_mux_b_offset -> pe_alu_pinB_offset
            #-------------------------------------------------------


            #-------------------------------------------------------
            #-------------------------------------------------------
            # Pins of ALU to the PE alu
            G.add_edge( PEindex + pe_alu_pinA_offset, PEindex + pe_alu_offset)    # pe_alu_pinA_offset -> pe_alu_offset
            G.add_edge( PEindex + pe_alu_pinB_offset, PEindex + pe_alu_offset)    # pe_alu_pinB_offset -> pe_alu_offset
            #-------------------------------------------------------

            #-------------------------------------------------------
            #-------------------------------------------------------
            # PE_alu to the output pin
            G.add_edge( PEindex + pe_alu_offset, PEindex + pe_alu_pinO_offset)    # pe_alu_offset -> pe_alu_pinO_offset
            #-------------------------------------------------------


            #Connecting all the output of output pin of PE to the all pins of SB:
            for k in range(sb_max):
                G.add_edge( PEindex + pe_alu_pinO_offset, PEindex + k)       


            #-------------------------------------------------------
            # STEP 4: Assigning types:
            #-------------------------------------------------------

            # Assigning mux type:
            # We have total 12 muxes, 10 from SB and 2 from PE:

            #Visual X and Y calculation:
            pe_gap = 1
            alu_out_pin_scalar   = (vertical_gap + pe_gap)*y_coordinate
            const_scalar         = alu_out_pin_scalar + 0.8    
            const_out_pin_scalar = alu_out_pin_scalar + 0.6
            alu_in_pin_scalar    = alu_out_pin_scalar + 0.4
            alu_scalar           = alu_out_pin_scalar + 0.2
            
            base_name = "pe" + ".w" + str(width)  + ".c" + str(x_coordinate) + ".r" + str(y_coordinate) 
            #print(f'for {current_pe_number} | {base_name} :: {x_coordinate}, {y_coordinate} :: {j+1}, {i}')
            MUX_NAME  = ["S", "SW", "W", "NW", "N", "NE", "E", "SE", "PEinA", "PEinB"]

            for k in range(sb_max+2):       
                if (k == sb_max):
                    G.nodes[PEindex + k]["G_Name"]         = base_name + ".mux_a"  
                elif (k == (sb_max+1)):
                    G.nodes[PEindex + k]["G_Name"]         = base_name + ".mux_b"
                else:   
                    G.nodes[PEindex + k]["G_Name"]         = base_name + ".crossbar_mux_" + MUX_NAME[k]  

                G.nodes[PEindex + k]["G_CellType"]     = "RouteCell"
                G.nodes[PEindex + k]["G_NodeType"]     = "Mux"
                G.nodes[PEindex + k]["G_Width"]        = str(width)

            # Assigning the constant type: 
            G.nodes[PEindex + pe_const_offset]["G_Name"]         =  base_name + ".const"
            G.nodes[PEindex + pe_const_offset]["G_CellType"]     = "FuncCell"
            G.nodes[PEindex + pe_const_offset]["G_NodeType"]     = "Constant"
            G.nodes[PEindex + pe_const_offset]["G_VisualX"]      = str(x_coordinate)
            G.nodes[PEindex + pe_const_offset]["G_VisualY"]      = str(const_scalar)   
            G.nodes[PEindex + pe_const_offset]["G_Width"]        = str(width)   

            # Assigning the Pin type
            G.nodes[PEindex + pe_const_pinO_offset]["G_Name"]         = base_name + ".const.outPinA" 
            G.nodes[PEindex + pe_const_pinO_offset]["G_CellType"]     = "PinCell"
            G.nodes[PEindex + pe_const_pinO_offset]["G_NodeType"]     = "out"
            G.nodes[PEindex + pe_const_pinO_offset]["G_VisualX"]      = str(x_coordinate)
            G.nodes[PEindex + pe_const_pinO_offset]["G_VisualY"]      = str(const_out_pin_scalar)  
            G.nodes[PEindex + pe_const_pinO_offset]["G_Width"]        = str(width)

            # Assigning the ALU type:
            G.nodes[PEindex + pe_alu_offset]["G_Name"]         =  base_name + ".alu"
            G.nodes[PEindex + pe_alu_offset]["G_CellType"]     = "FuncCell"
            G.nodes[PEindex + pe_alu_offset]["G_NodeType"]     = "ALU"
            G.nodes[PEindex + pe_alu_offset]["G_VisualX"]      = str(x_coordinate)
            G.nodes[PEindex + pe_alu_offset]["G_VisualY"]      = str(alu_scalar)
            G.nodes[PEindex + pe_alu_offset]["G_Width"]        = str(width)

            # Assigning the Pin type
            G.nodes[PEindex + pe_alu_pinA_offset]["G_Name"]         = base_name + ".alu.inPinA"  
            G.nodes[PEindex + pe_alu_pinA_offset]["G_CellType"]     = "PinCell"
            G.nodes[PEindex + pe_alu_pinA_offset]["G_NodeType"]     = "in"
            G.nodes[PEindex + pe_alu_pinA_offset]["G_VisualX"]      = str((x_coordinate) - 0.25)
            G.nodes[PEindex + pe_alu_pinA_offset]["G_VisualY"]      = str(alu_in_pin_scalar)
            G.nodes[PEindex + pe_alu_pinA_offset]["G_Width"]        = str(width)

            G.nodes[PEindex + pe_alu_pinB_offset]["G_Name"]         = base_name + ".alu.inPinB" 
            G.nodes[PEindex + pe_alu_pinB_offset]["G_CellType"]     = "PinCell"
            G.nodes[PEindex + pe_alu_pinB_offset]["G_NodeType"]     = "in"
            G.nodes[PEindex + pe_alu_pinB_offset]["G_VisualX"]      = str((x_coordinate) + 0.25)
            G.nodes[PEindex + pe_alu_pinB_offset]["G_VisualY"]      = str(alu_in_pin_scalar)
            G.nodes[PEindex + pe_alu_pinB_offset]["G_Width"]        = str(width)

            G.nodes[PEindex + pe_alu_pinO_offset]["G_Name"]         = base_name + ".alu.outPinA" 
            G.nodes[PEindex + pe_alu_pinO_offset]["G_CellType"]     = "PinCell"
            G.nodes[PEindex + pe_alu_pinO_offset]["G_NodeType"]     = "out"
            G.nodes[PEindex + pe_alu_pinO_offset]["G_VisualX"]      = str(x_coordinate)
            G.nodes[PEindex + pe_alu_pinO_offset]["G_VisualY"]      = str(alu_out_pin_scalar)
            G.nodes[PEindex + pe_alu_pinO_offset]["G_Width"]        = str(width)

    #--------------------------------------------------------
    #   Visualizing a PE
    #
    #--------------------------------------------------------
    #PEstart
    PE_end = PEstart + PEstep
    H = G.subgraph(list(G.nodes)[PEstart:PE_end])

    nodes = H.nodes()
    colors = []
    for n in nodes:
        #print(n)
        #print(H.nodes[n]["G_Name"])
        if (H.nodes[n]["G_CellType"]) == "RouteCell":
            node_color = 'lightblue'
        elif (H.nodes[n]["G_CellType"]) == "FuncCell":
            node_color = 'lightcoral'
        elif (H.nodes[n]["G_CellType"]) == "PinCell":
            node_color = 'lightgreen'
        colors.append(node_color)

    # Apply the neato layout using graphviz_layout
    pos = {
        PEstart: (13, 2),           #Mux-0
        PEstart+1: (12, 2.5),         #Mux-1
        PEstart+2: (11, 3),           #Mux-2
        PEstart+3: (12, 3.5),         #Mux-3
        PEstart+4: (13, 4),           #Mux-4
        PEstart+5: (14, 3.5),         #Mux-5
        PEstart+6: (15, 3),           #Mux-6
        PEstart+7: (14, 2.5),         #Mux-7
        PEstart+8: (0, 4),            #Mux-8
        PEstart+9: (0, 2),            #Mux-9
        PEstart+10: (3.5,4),           #Mux-A
        PEstart+11: (3.5, 2),          #Mux-B
        PEstart+12: (1, 3),            #Constant
        PEstart+13: (3, 3),            #Constant-Output
        PEstart+14: (7,3),             #ALU 
        PEstart+15: (5.5, 4),          #PinA
        PEstart+16: (5.5,2),           #PinB
        PEstart+17: (9,3)              #PinC
    }   

    # Draw the graph with the specified positions
    labels = nx.get_node_attributes(H, 'G_Name')
    label_offset = 0.10
    label_pos = {node: (pos[node][0], pos[node][1] - label_offset) for node in H.nodes()}

    nx.draw(H, pos, node_color=colors, edge_color='gray', node_size=500)
    nx.draw_networkx_labels(H, label_pos, labels=labels, font_size=10, font_weight='bold', verticalalignment='top', horizontalalignment='center')
    #plt.show()

    # -------------------------------------------------------
    #  Writing device model graph for Riken architecture
    # -------------------------------------------------------
    output_file = "riken_" + str(args.NR) + "_" + str(args.NC) + ".dot"
    nx.nx_pydot.write_dot(G, output_file)

    #--------------------------------------------------------
    #  Writing the PRAGMA to the output file
    #--------------------------------------------------------
    pragma_string = """ /*
{
    "ALU" : ["FADD", "FMUL", "FSUB", "FDIV"],
    "MEMPORT" : ["input", "output"],
    "Constant" : ["const"]
}
*/
    """
    print("\n\npragma for the device model: \n")
    print(pragma_string)

    with open(output_file,'r') as contents:
        save = contents.read()
    with open(output_file,'w') as contents:
        contents.write(pragma_string)
    with open(output_file,'a') as contents:
        contents.write(save)
   
# Creates device model for the ADRES architecture:
def create_adres(args):
    pass

# Main function for generating device model for UGRAMM:
def main(args):
    print("====================================================")
    print("============ Universal GRAMM (UGRAMM) ==============")
    print("======= helper script: Device Model Generator ======")
    print("====================================================\n")

    #Printing arguments:
    print("Generating device model for the following CGRA configuration: ")
    print("> Arch:", args.Arch)
    print("> NR:", args.NR)
    print("> NC:", args.NC)
    
    if (args.Arch == 'RIKEN'):
        print(f"Creating {args.Arch} architecture!!")
        create_riken(args)
    elif (args.Arch == 'ADRES'):
        print(f"Creating {args.Arch} architecture!!")
        create_adres(args)

    else:
        print("Unsupported architecture input")
        exit()

if __name__ == "__main__":

    parser = argparse.ArgumentParser()  # Create ArgumentParser object

    # Add arguments with specific names and types
    parser.add_argument('-NR', type=int)
    parser.add_argument('-NC', type=int)
    parser.add_argument('-Arch')
    args = parser.parse_args()          # Parse the arguments

    main(args)                          # Calling main function
