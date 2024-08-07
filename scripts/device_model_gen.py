#!/usr/bin/python3

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

# Creates device model for the RIKEN architecture:
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

    #Each PE+SB has 14 nodes inside in it which contains blocks such as mux, ALU, SB, constant etc.
    PEstep  = 14
    
    #The first PE index number starts after the left and right most IO connection.
    PEstart =  args.NR + args.NR

    total_nodes_riken = (args.NR + args.NR + (PEstep*args.NR*args.NC)) 
    
    #------------------------------------------------------------------
    #------------------------------------------------------------------
    
    G = nx.DiGraph()    #Creating a directed graph using networkx

    #--------------------------------------------------------
    #Step 0: Adding total number of nodes into Dir-Graph:
    #--------------------------------------------------------
    G.add_nodes_from(range(0,total_nodes_riken))
    #print(list(G.nodes))

    #----------------------------------------------------------------
    #Step 1: Add the connection to the Load/Store units on both side
    #----------------------------------------------------------------
    for i in range(args.NR):
        G.nodes[i]["G_Name"]             = "LS_" + str(i)
        G.nodes[i+args.NR]["G_Name"]     = "LS_" + str(i+args.NR)

        G.nodes[i]["G_NodeType"]         = "FuncCell"    #Left-IOs
        G.nodes[i+args.NR]["G_NodeType"] = "FuncCell"    #Right-IOs

        G.nodes[i]["G_opcode"]         = "MemPort"    #Left-IOs
        G.nodes[i+args.NR]["G_opcode"] = "MemPort"    #Right-IOs

        G.nodes[i]["G_ID"]         = str(i)    #Left-IOs
        G.nodes[i+args.NR]["G_ID"] = str(i+args.NR)    #Right-IOs
        
        #----------------------------------
        #Connectivity of IO ports and SBs:
        #----------------------------------

        #Left-most column:
        for j in range(sb_max):
            if (j == left_sb):
                continue
            G.add_edge(i, PEstart+ i*args.NC*PEstep + j)    #Edge from current IO port to adjacent SB's pins

        G.add_edge(PEstart+ i*args.NC*PEstep + left_sb, i)  #Edge from adjacent closest SB port to IO port

        #Right-most column:
        for j in range(sb_max):
            if (j == right_sb):
                continue
            G.add_edge(i+args.NR, PEstart+ i*args.NC*PEstep + (args.NC-1)*PEstep + j)    #Edge from current IO port to adjacent SB's pins
 
        G.add_edge(PEstart+ i*args.NC*PEstep + (args.NC-1)*PEstep + right_sb, i+args.NR)   #Edge from adjacent closest SB port to IO port


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

            #--------------------------
            #         SOUTH
            #--------------------------
            if (i < (args.NR - 1)):                   #South connections are not present for the last-row
                dest_row = i+1
                dest_col = j
                # Connecting current SB (I, J) to SB position on south direction.
                # Connecting 0th pin of current SB to all other pins of the destination SB.
                for k in range(sb_max):
                    if (k == up_sb):
                        continue
                    G.add_edge( PEstart + i*args.NC*PEstep + j*PEstep + 0, 
                        PEstart + dest_row*args.NC*PEstep + dest_col*PEstep + k)
                    
            #--------------------------
            #       SOUTH-WEST
            #--------------------------
            if ((i < (args.NR - 1)) and (j > 0)):       #South-WEST connections are not present for first column and last row
                dest_row = i+1
                dest_col = j-1
                # Connecting current SB (I, J) to SB position on SOUTH-WEST direction.
                # Connecting 1st pin of current SB to all other pins of the destination SB.
                for k in range(sb_max):
                    if (k == 5):
                        continue
                    G.add_edge( PEstart + i*args.NC*PEstep + j*PEstep + 1, 
                        PEstart + dest_row*args.NC*PEstep + dest_col*PEstep + k)
            
            #--------------------------
            #         WEST
            #--------------------------
            if (j > 0):                                #WEST connections are not present for the first column
                dest_row = i
                dest_col = j-1
                for k in range(sb_max):
                    if (k == 6):
                        continue
                    G.add_edge( PEstart + i*args.NC*PEstep + j*PEstep + 2, 
                        PEstart + dest_row*args.NC*PEstep + dest_col*PEstep + k)            

            #--------------------------
            #       NORTH-WEST
            #-------------------------- 
            if ((i > 0) and (j > 0)):                  #NORTH-WEST connections are not present for first row and column
                dest_row = i-1
                dest_col = j-1
                for k in range(sb_max):
                    if (k == 7):
                        continue
                    G.add_edge( PEstart + i*args.NC*PEstep + j*PEstep + 3, 
                        PEstart + dest_row*args.NC*PEstep + dest_col*PEstep + k)
            
            #--------------------------
            #         NORTH
            #--------------------------
            if (i > 0):                               #NORTH connections are not present for the first row
                dest_row = i-1
                dest_col = j
                for k in range(sb_max):
                    if (k == 0):
                        continue
                    G.add_edge( PEstart + i*args.NC*PEstep + j*PEstep + 4, 
                        PEstart + dest_row*args.NC*PEstep + dest_col*PEstep + k)

            #--------------------------
            #       NORTH-EAST
            #--------------------------
            if ((i > 0) and (j < (args.NC-1))):        #NORTH-EAST connections are not present for first row and last column
                dest_row = i-1
                dest_col = j+1
                for k in range(sb_max):
                    if (k == 1):
                        continue
                    G.add_edge( PEstart + i*args.NC*PEstep + j*PEstep + 5, 
                        PEstart + dest_row*args.NC*PEstep + dest_col*PEstep + k)               

            #--------------------------
            #         EAST
            #--------------------------
            if ((j < (args.NC-1))):                   #EAST connections are not present for last column
                dest_row = i
                dest_col = j+1
                for k in range(sb_max):
                    if (k == 2):
                        continue
                    G.add_edge( PEstart + i*args.NC*PEstep + j*PEstep + 6, 
                        PEstart + dest_row*args.NC*PEstep + dest_col*PEstep + k)

            #--------------------------
            #       SOUTH-EAST
            #--------------------------
            if ((i < (args.NR - 1)) and (j < (args.NC-1))): #SOUTH-EAST connections are not present for last row & column
                dest_row = i+1
                dest_col = j+1
                for k in range(sb_max):
                    if (k == 3):
                        continue
                    G.add_edge( PEstart + i*args.NC*PEstep + j*PEstep + 7, 
                        PEstart + dest_row*args.NC*PEstep + dest_col*PEstep + k)

    #--------------------------------------------------------
    #Step 3: Intra-PE connectivity
    #           &
    #Step 4: Assigning the Node type attribute
    #--------------------------------------------------------
    for i in range(args.NR):
        for j in range(args.NC):

            PEindex = PEstart + i*args.NC*PEstep + j*PEstep
            

            pe_mux_in_a_offest = 9      #Caveat, need to fix this later in the GRAMM code.
            pe_mux_in_b_offset = 8
            pe_mux_a_offset    = 10      
            pe_mux_b_offset    = 11
            pe_const_offset    = 12
            pe_alu_offset      = 13

            #Connecting the SB input to the MUX 
            #(9-->10)
            G.add_edge( PEindex + pe_mux_in_a_offest, PEindex + pe_mux_a_offset)

            #(8-->11)
            G.add_edge( PEindex + pe_mux_in_b_offset, PEindex + pe_mux_b_offset)

            #-------------------------------------------------------
            #-------------------------------------------------------

            #Connecting the Const to the Mux 

            #(12-->10)
            G.add_edge( PEindex + pe_const_offset, PEindex + pe_mux_a_offset)

            #(12-->11)
            G.add_edge( PEindex + pe_const_offset, PEindex + pe_mux_b_offset)

            #-------------------------------------------------------
            #-------------------------------------------------------
            
            #Connecting the Mux to the ALU:

            #(10-->13)
            G.add_edge( PEindex + pe_mux_a_offset, PEindex + pe_alu_offset)

            #(11-->13)
            G.add_edge( PEindex + pe_mux_b_offset, PEindex + pe_alu_offset)        

            #-------------------------------------------------------
            #-------------------------------------------------------

            #Connecting all the output of PE to the all pins of SB:
            for k in range(sb_max):
                #(13-->k)
                G.add_edge( PEindex + pe_alu_offset, PEindex + k)       


            #-------------------------------------------------------
            # STEP 4: Assigning types:
            #-------------------------------------------------------

            # Assigning mux type:
            # We have total 12 muxes, 10 from SB and 2 from PE:
            # Current PE number:
            current_pe_number = int((PEindex - PEstart)/PEstep)
            x_coordinate = int(current_pe_number/args.NR)
            y_coordinate = current_pe_number%args.NR  
            base_name = "pe" + ".w" + str(width)  + ".c" + str(x_coordinate) + ".r" + str(y_coordinate) 

            for k in range(sb_max+2):       
                if (k == sb_max):
                    G.nodes[PEindex + k]["G_Name"]         = base_name + ".mux_a"   
                elif (k == (sb_max+1)):
                    G.nodes[PEindex + k]["G_Name"]         = base_name + ".mux_b"
                else:
                    G.nodes[PEindex + k]["G_Name"]         = base_name + ".crossbar_mux_" + str(k) 

                G.nodes[PEindex + k]["G_NodeType"]     = "RouteCell";
                G.nodes[PEindex + k]["G_opcode"]       = "Mux";
                G.nodes[PEindex + k]["G_ID"]           = str(PEindex + k) 

            # Assigning the constant type:
            G.nodes[PEindex + pe_const_offset]["G_Name"]         =  base_name + ".const"
            G.nodes[PEindex + pe_const_offset]["G_NodeType"]     = "FuncCell";
            G.nodes[PEindex + pe_const_offset]["G_opcode"]       = "Constant";
            G.nodes[PEindex + pe_const_offset]["G_ID"]           = str(PEindex + pe_const_offset)  

            # Assigning the ALU type:
            G.nodes[PEindex + pe_alu_offset]["G_Name"]         =  base_name + ".alu"
            G.nodes[PEindex + pe_alu_offset]["G_NodeType"]     = "FuncCell";
            G.nodes[PEindex + pe_alu_offset]["G_opcode"]       = "ALU";
            G.nodes[PEindex + pe_alu_offset]["G_ID"]           = str(PEindex + pe_alu_offset) 


    #--------------------------------------------------------
    #   Visualizing a PE
    #
    #--------------------------------------------------------

    PE_end = PEstart + PEstep
    H = G.subgraph(list(G.nodes)[PEstart:PE_end])

    nodes = H.nodes()
    colors = []
    for n in nodes:
        print(H.nodes[n]["G_Name"])
        if (H.nodes[n]["G_NodeType"]) == "RouteCell":
            node_color = 'lightblue'
        elif (H.nodes[n]["G_NodeType"]) == "FuncCell":
            node_color = 'lightcoral'
        elif (H.nodes[n]["G_NodeType"]) == "PinCell":
            node_color = 'lightgreen'
        colors.append(node_color)

    # Apply the neato layout using graphviz_layout
    pos = {
        16: (13, 2),
        17: (12, 2.5),
        18: (11, 3),
        19: (12, 3.5),
        20: (13, 4),
        21: (14, 3.5),
        22: (15, 3),
        23: (14, 2.5),
        24: (1, 2),
        25: (1, 4),
        26: (5, 4),
        27: (5, 2),
        28: (3, 3),
        29: (7, 3)
    }

    # Draw the graph with the specified positions
    labels = nx.get_node_attributes(H, 'G_Name')
    label_offset = 0.10
    label_pos = {node: (pos[node][0], pos[node][1] - label_offset) for node in H.nodes()}

    nx.draw(H, pos, node_color=colors, edge_color='gray', node_size=500)
    nx.draw_networkx_labels(H, label_pos, labels=labels, font_size=12, font_weight='bold', verticalalignment='top', horizontalalignment='center')
    plt.show()

    # -------------------------------------------------------
    #  Writing device model graph for Riken architecture
    # -------------------------------------------------------
    output_file = "riken_" + str(args.NR) + "_" + str(args.NC) + ".dot"
    nx.nx_pydot.write_dot(G, output_file)

def create_riken_with_pins(args):
    
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
    PEstep  = 18
    
    #The first PE index number starts after the left and right most IO connection.
    PEstart =  args.NC + (args.NR*3)

    total_nodes_riken = (args.NR + args.NR + (PEstep*args.NR*args.NC)) 
    
    #------------------------------------------------------------------
    #------------------------------------------------------------------
    
    G = nx.DiGraph()    #Creating a directed graph using networkx

    #--------------------------------------------------------
    #Step 0: Adding total number of nodes into Dir-Graph:
    #--------------------------------------------------------
    G.add_nodes_from(range(0,total_nodes_riken))
    #print(list(G.nodes))

    #----------------------------------------------------------------
    #Step 1: Add the connection to the Load/Store units on both side
    #----------------------------------------------------------------
    for i in range(args.NR):

        #---------------------------------------------------------------
        #Left-IO:
        #---------------------------------------------------------------
        left_io_index        = i + (1*i)
        left_io_PinA_index = left_io_index + 1
        
        #Left-IO:
        G.nodes[left_io_index]["G_Name"]             = "LS_" + str(i)  
        G.nodes[left_io_index]["G_NodeType"]         = "FuncCell"     
        G.nodes[left_io_index]["G_opcode"]           = "MemPort"     
        G.nodes[left_io_index]["G_ID"]               = str(left_io_index)          


        #Left-IO-bidirection-Pin :
        G.nodes[left_io_PinA_index]["G_Name"]             = "LS_" + str(i) + ".inPinA"
        G.nodes[left_io_PinA_index]["G_NodeType"]         = "PinCell"   
        G.nodes[left_io_PinA_index]["G_opcode"]           = "bidirectional"      
        G.nodes[left_io_PinA_index]["G_ID"]               = str(left_io_PinA_index)          

        #----------------------------------
        #Connectivity of Left IO port and Pin cell:
        #----------------------------------
        #bi-directional edge between left io and the pin cell
        G.add_edge(left_io_index, left_io_PinA_index)
        G.add_edge(left_io_PinA_index, left_io_index)

        #---------------------------------------------------------------
        #Right-IO:
        #---------------------------------------------------------------
        right_io_index        = i + (1*i) + (args.NR*2)
        right_io_PinA_index   = right_io_index + 1

        G.nodes[right_io_index]["G_Name"]     = "LS_" + str(i+args.NR)  #Right-IOs       
        G.nodes[right_io_index]["G_NodeType"] = "FuncCell"              #Right-IOs
        G.nodes[right_io_index]["G_opcode"]   = "MemPort"               #Right-IOs
        G.nodes[right_io_index]["G_ID"]       = str(right_io_index)     #Right-IOs

        #Right-IO-bidirection-Pin :
        G.nodes[right_io_PinA_index]["G_Name"]             = "LS_" + str(i) + ".inPinA"
        G.nodes[right_io_PinA_index]["G_NodeType"]         = "PinCell"   
        G.nodes[right_io_PinA_index]["G_opcode"]           = "bidirectional"      
        G.nodes[right_io_PinA_index]["G_ID"]               = str(right_io_PinA_index)     
 
        #----------------------------------
        #Connectivity of Right IO port and Pin cell:
        #----------------------------------
        G.add_edge(right_io_index, right_io_PinA_index)
        G.add_edge(right_io_PinA_index, right_io_index)

        #----------------------------------
        #Connectivity of IO ports and SBs:
        #----------------------------------

        #Left-most column:
        for j in range(sb_max):
            if (j == left_sb):
                continue
            G.add_edge(left_io_PinA_index, PEstart+ i*args.NC*PEstep + j)    #Edge from current IO port-pin to adjacent SB's pins

        G.add_edge(PEstart+ i*args.NC*PEstep + left_sb, left_io_PinA_index)  #Edge from adjacent closest SB port to IO port-pin

        #Right-most column:
        for j in range(sb_max):
            if (j == right_sb):
                continue
            G.add_edge(right_io_PinA_index, PEstart+ i*args.NC*PEstep + (args.NC-1)*PEstep + j)    #Edge from current IO port to adjacent SB's pins
 
        G.add_edge(PEstart+ i*args.NC*PEstep + (args.NC-1)*PEstep + right_sb, right_io_PinA_index)   #Edge from adjacent closest SB port to IO port


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

            #--------------------------
            #         SOUTH
            #--------------------------
            if (i < (args.NR - 1)):                   #South connections are not present for the last-row
                dest_row = i+1
                dest_col = j
                # Connecting current SB (I, J) to SB position on south direction.
                # Connecting 0th pin of current SB to all other pins of the destination SB.
                for k in range(sb_max):
                    if (k == up_sb):
                        continue
                    G.add_edge( PEstart + i*args.NC*PEstep + j*PEstep + 0, 
                        PEstart + dest_row*args.NC*PEstep + dest_col*PEstep + k)
                    
            #--------------------------
            #       SOUTH-WEST
            #--------------------------
            if ((i < (args.NR - 1)) and (j > 0)):       #South-WEST connections are not present for first column and last row
                dest_row = i+1
                dest_col = j-1
                # Connecting current SB (I, J) to SB position on SOUTH-WEST direction.
                # Connecting 1st pin of current SB to all other pins of the destination SB.
                for k in range(sb_max):
                    if (k == 5):
                        continue
                    G.add_edge( PEstart + i*args.NC*PEstep + j*PEstep + 1, 
                        PEstart + dest_row*args.NC*PEstep + dest_col*PEstep + k)
            
            #--------------------------
            #         WEST
            #--------------------------
            if (j > 0):                                #WEST connections are not present for the first column
                dest_row = i
                dest_col = j-1
                for k in range(sb_max):
                    if (k == 6):
                        continue
                    G.add_edge( PEstart + i*args.NC*PEstep + j*PEstep + 2, 
                        PEstart + dest_row*args.NC*PEstep + dest_col*PEstep + k)            

            #--------------------------
            #       NORTH-WEST
            #-------------------------- 
            if ((i > 0) and (j > 0)):                  #NORTH-WEST connections are not present for first row and column
                dest_row = i-1
                dest_col = j-1
                for k in range(sb_max):
                    if (k == 7):
                        continue
                    G.add_edge( PEstart + i*args.NC*PEstep + j*PEstep + 3, 
                        PEstart + dest_row*args.NC*PEstep + dest_col*PEstep + k)
            
            #--------------------------
            #         NORTH
            #--------------------------
            if (i > 0):                               #NORTH connections are not present for the first row
                dest_row = i-1
                dest_col = j
                for k in range(sb_max):
                    if (k == 0):
                        continue
                    G.add_edge( PEstart + i*args.NC*PEstep + j*PEstep + 4, 
                        PEstart + dest_row*args.NC*PEstep + dest_col*PEstep + k)

            #--------------------------
            #       NORTH-EAST
            #--------------------------
            if ((i > 0) and (j < (args.NC-1))):        #NORTH-EAST connections are not present for first row and last column
                dest_row = i-1
                dest_col = j+1
                for k in range(sb_max):
                    if (k == 1):
                        continue
                    G.add_edge( PEstart + i*args.NC*PEstep + j*PEstep + 5, 
                        PEstart + dest_row*args.NC*PEstep + dest_col*PEstep + k)               

            #--------------------------
            #         EAST
            #--------------------------
            if ((j < (args.NC-1))):                   #EAST connections are not present for last column
                dest_row = i
                dest_col = j+1
                for k in range(sb_max):
                    if (k == 2):
                        continue
                    G.add_edge( PEstart + i*args.NC*PEstep + j*PEstep + 6, 
                        PEstart + dest_row*args.NC*PEstep + dest_col*PEstep + k)

            #--------------------------
            #       SOUTH-EAST
            #--------------------------
            if ((i < (args.NR - 1)) and (j < (args.NC-1))): #SOUTH-EAST connections are not present for last row & column
                dest_row = i+1
                dest_col = j+1
                for k in range(sb_max):
                    if (k == 3):
                        continue
                    G.add_edge( PEstart + i*args.NC*PEstep + j*PEstep + 7, 
                        PEstart + dest_row*args.NC*PEstep + dest_col*PEstep + k)

    #--------------------------------------------------------
    #Step 3: Intra-PE connectivity
    #           &
    #Step 4: Assigning the Node type attribute
    #--------------------------------------------------------
    for i in range(args.NR):
        for j in range(args.NC):

            PEindex = PEstart + i*args.NC*PEstep + j*PEstep

            sb_pe_mux_a_offest = 8       #Fixed: Caveat, need to fix this later in the GRAMM code.
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
            # Current PE number:
            current_pe_number = int((PEindex - PEstart)/PEstep)
            x_coordinate = int(current_pe_number/args.NR)
            y_coordinate = current_pe_number%args.NR  

            base_name = "pe" + ".w" + str(width)  + ".c" + str(x_coordinate) + ".r" + str(y_coordinate) 

            for k in range(sb_max+2):       
                if (k == sb_max):
                    G.nodes[PEindex + k]["G_Name"]         = base_name + ".mux_a"  
                elif (k == (sb_max+1)):
                    G.nodes[PEindex + k]["G_Name"]         = base_name + ".mux_b"
                else:   
                    G.nodes[PEindex + k]["G_Name"]         = base_name + ".crossbar_mux_" + str(k)  

                G.nodes[PEindex + k]["G_NodeType"]     = "RouteCell";
                G.nodes[PEindex + k]["G_opcode"]       = "Mux";
                G.nodes[PEindex + k]["G_ID"]           = str(PEindex + k) 

            # Assigning the constant type: 
            G.nodes[PEindex + pe_const_offset]["G_Name"]         =  base_name + ".const"
            G.nodes[PEindex + pe_const_offset]["G_NodeType"]     = "FuncCell";
            G.nodes[PEindex + pe_const_offset]["G_opcode"]       = "Constant";
            G.nodes[PEindex + pe_const_offset]["G_ID"]           = str(PEindex + pe_const_offset)  

            # Assigning the ALU type:
            G.nodes[PEindex + pe_alu_offset]["G_Name"]         =  base_name + ".alu"
            G.nodes[PEindex + pe_alu_offset]["G_NodeType"]     = "FuncCell";
            G.nodes[PEindex + pe_alu_offset]["G_opcode"]       = "ALU";
            G.nodes[PEindex + pe_alu_offset]["G_ID"]           = str(PEindex + pe_alu_offset) 

            # Assigning the Pin type
            G.nodes[PEindex + pe_const_pinO_offset]["G_Name"]         = base_name + ".const.pinA" 
            G.nodes[PEindex + pe_const_pinO_offset]["G_NodeType"]     = "PinCell";
            G.nodes[PEindex + pe_const_pinO_offset]["G_opcode"]       = "in";
            G.nodes[PEindex + pe_const_pinO_offset]["G_ID"]           = str(PEindex + pe_const_pinO_offset)         

            G.nodes[PEindex + pe_alu_pinA_offset]["G_Name"]         = base_name + ".alu.pinA"  
            G.nodes[PEindex + pe_alu_pinA_offset]["G_NodeType"]     = "PinCell";
            G.nodes[PEindex + pe_alu_pinA_offset]["G_opcode"]       = "in";
            G.nodes[PEindex + pe_alu_pinA_offset]["G_ID"]           = str(PEindex + pe_alu_pinA_offset)           

            G.nodes[PEindex + pe_alu_pinB_offset]["G_Name"]         = base_name + ".alu.pinB" 
            G.nodes[PEindex + pe_alu_pinB_offset]["G_NodeType"]     = "PinCell";
            G.nodes[PEindex + pe_alu_pinB_offset]["G_opcode"]       = "in";
            G.nodes[PEindex + pe_alu_pinB_offset]["G_ID"]           = str(PEindex + pe_alu_pinB_offset)     

            G.nodes[PEindex + pe_alu_pinO_offset]["G_Name"]         = base_name + ".alu.pinO" 
            G.nodes[PEindex + pe_alu_pinO_offset]["G_NodeType"]     = "PinCell";
            G.nodes[PEindex + pe_alu_pinO_offset]["G_opcode"]       = "out";
            G.nodes[PEindex + pe_alu_pinO_offset]["G_ID"]           = str(PEindex + pe_alu_pinO_offset)     
    
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
        print(H.nodes[n]["G_Name"])
        if (H.nodes[n]["G_NodeType"]) == "RouteCell":
            node_color = 'lightblue'
        elif (H.nodes[n]["G_NodeType"]) == "FuncCell":
            node_color = 'lightcoral'
        elif (H.nodes[n]["G_NodeType"]) == "PinCell":
            node_color = 'lightgreen'
        colors.append(node_color)

    # Apply the neato layout using graphviz_layout
    pos = {
        32: (13, 2),           #Mux-0
        33: (12, 2.5),         #Mux-1
        34: (11, 3),           #Mux-2
        35: (12, 3.5),         #Mux-3
        36: (13, 4),           #Mux-4
        37: (14, 3.5),         #Mux-5
        38: (15, 3),           #Mux-6
        39: (14, 2.5),         #Mux-7
        40: (0, 4),            #Mux-8
        41: (0, 2),            #Mux-9
        42: (3.5,4),           #Mux-A
        43: (3.5, 2),          #Mux-B
        44: (1, 3),            #Constant
        45: (3, 3),            #Constant-Output
        46: (7,3),             #ALU 
        47: (5.5, 4),          #PinA
        48: (5.5,2),           #PinB
        49: (9,3)              #PinC
    }   

    # Draw the graph with the specified positions
    labels = nx.get_node_attributes(H, 'G_Name')
    label_offset = 0.10
    label_pos = {node: (pos[node][0], pos[node][1] - label_offset) for node in H.nodes()}

    nx.draw(H, pos, node_color=colors, edge_color='gray', node_size=500)
    nx.draw_networkx_labels(H, label_pos, labels=labels, font_size=10, font_weight='bold', verticalalignment='top', horizontalalignment='center')
    plt.show()

    # -------------------------------------------------------
    #  Writing device model graph for Riken architecture
    # -------------------------------------------------------
    output_file = "riken_" + str(args.NR) + "_" + str(args.NC) + ".dot"
    nx.nx_pydot.write_dot(G, output_file)
   
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
    print("Generating device model for following CGRA configuration: ")
    print("> Arch:", args.Arch)
    print("> NR:", args.NR)
    print("> NC:", args.NC)
    
    if (args.Arch == 'RIKEN'):
        print(f"Creating {args.Arch} architecture!!")
        create_riken(args)
    elif (args.Arch == 'ADRES'):
        print(f"Creating {args.Arch} architecture!!")
        create_adres(args)
    elif (args.Arch == 'RIKEN_with_pins'):
        print(f"Creating {args.Arch} architecture!!")
        args.Arch = 'RIKEN'
        create_riken_with_pins(args)
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