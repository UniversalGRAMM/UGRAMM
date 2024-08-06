#!/usr/bin/python3

#-------------------------------------------
#  UGRAMM    :: helper script
#  Author    :: Omkar Bhilare
#  Email-ID  :: omkar.bhilare@mail.utoronto.ca
#--------------------------------------------

# Required packages:
import argparse
import networkx as nx
import matplotlib.pyplot as plt

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
            #Connecting the SB input pins to the MUX 
            
            #(9-->10)
            G.add_edge( PEindex + 9, PEindex + 10)

            #(8-->11)
            G.add_edge( PEindex + 8, PEindex + 11)

            #-------------------------------------------------------
            #-------------------------------------------------------

            #Connecting the Const to the Mux 

            #(12-->10)
            G.add_edge( PEindex + 12, PEindex + 10)

            #(12-->11)
            G.add_edge( PEindex + 12, PEindex + 11)

            #-------------------------------------------------------
            #-------------------------------------------------------
            
            #Connecting the Mux to the ALU:

            #(10-->13)
            G.add_edge( PEindex + 10, PEindex + 13)

            #(11-->13)
            G.add_edge( PEindex + 11, PEindex + 13)        

            #-------------------------------------------------------
            #-------------------------------------------------------

            #Connecting all the output of PE to the all pins of SB:
            for k in range(sb_max):
                #(13-->k)
                G.add_edge( PEindex + 13, PEindex + k)       


            #-------------------------------------------------------
            # STEP 4: Assigning types:
            #-------------------------------------------------------

            # Assigning mux type:
            # We have total 12 muxes, 10 from SB and 2 from PE:
            for k in range(sb_max+2):       
                G.nodes[PEindex + k]["G_NodeType"]     = "RouteCell";
                G.nodes[PEindex + k]["G_opcode"] = "Mux";
                G.nodes[PEindex + k]["G_ID"]       = str(PEindex + k) 
            

            # Assigning the input pins type:
            G.nodes[PEindex + 8]["G_NodeType"]     = "PinCell";
            G.nodes[PEindex + 8]["G_opcode"] = "InputPinA";
            G.nodes[PEindex + 8]["G_ID"]       = str(PEindex + 8)  

            G.nodes[PEindex + 9]["G_NodeType"]     = "PinCell";
            G.nodes[PEindex + 9]["G_opcode"] = "InputPinB";
            G.nodes[PEindex + 9]["G_ID"]       = str(PEindex + 9)  

            # Assigning the constant type:
            G.nodes[PEindex + 12]["G_NodeType"]     = "FuncCell";
            G.nodes[PEindex + 12]["G_opcode"] = "Constant";
            G.nodes[PEindex + 12]["G_ID"]       = str(PEindex + 12)  

            # Assigning the ALU type:
            G.nodes[PEindex + 13]["G_NodeType"]     = "FuncCell";
            G.nodes[PEindex + 13]["G_opcode"] = "ALU";
            G.nodes[PEindex + 13]["G_ID"]       = str(PEindex + 13) 


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