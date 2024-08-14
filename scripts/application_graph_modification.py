#!/usr/bin/python3

#-------------------------------------------
#  UGRAMM    :: helper script
#  Author    :: Hamas Waqar
#  Email-ID  :: hamas.waqar@mail.utoronto.ca
#--------------------------------------------

# Required packages:
import argparse
import os
import networkx as nx
import matplotlib.pyplot as plt

# Creates device model for the RIKEN architecture:
def modify_application(args):
    
    #------------------------ Application Graph Config ----------------------------
    file_path = str(args.Benchmark)
    filename = os.path.basename(file_path)
    directoryname = os.path.dirname(file_path)
    # input_file = str(args.Benchmark)
    G = nx.DiGraph(nx.nx_pydot.read_dot(file_path))


    valid_input_pins_name = frozenset({'inPinA', 'inPinB'})
    valid_output_pins_name = frozenset({'outPinA'})

    currently_used_input_pins = set()

    for node in G.nodes:
        
        currently_used_input_pins = set(valid_input_pins_name)
        # Iterate over all outgoing edges for the current node and add the dedicated input 
        # pins to the currently_used_input_pins set 
        for u, v, properties in G.in_edges(node, data=True):
            # print(f"Edge from {u} to {v} has properties {properties}")
            load = properties.get('load')
            if load in valid_input_pins_name:
                currently_used_input_pins.remove(load)
        

        for u, v, properties in G.in_edges(node, data=True):
            # print(f"Edge from {u} to {v} has properties {properties}")
            load = properties.get('load')
            if load not in valid_input_pins_name:
                new_pin_name = currently_used_input_pins.pop()
                G[u][v]['load'] = new_pin_name


    # -------------------------------------------------------
    #  Writing device model graph for Riken architecture
    # -------------------------------------------------------
    # output_file = "modified_" + str(args.Benchmark)
    os.chdir(directoryname)
    output_file = "modified_" + str(filename)
    nx.nx_pydot.write_dot(G, output_file)
    
# Main function for modifying the application graph for UGRAMM:
def main(args):
    print("====================================================")
    print("============ Universal GRAMM (UGRAMM) ==============")
    print("======= helper script: Application Graph Modifier ======")
    print("====================================================\n")

    #Printing arguments:
    print("Generating device model for following CGRA configuration: ")
    print("> Benchmark:", args.Benchmark)
    modify_application(args)

if __name__ == "__main__":

    parser = argparse.ArgumentParser()  # Create ArgumentParser object

    # Add arguments with specific names and types
    parser.add_argument('-Benchmark')

    args = parser.parse_args()          # Parse the arguments

    main(args)                          # Calling main function