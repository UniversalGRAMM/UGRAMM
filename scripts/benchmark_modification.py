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
    filePath = str(args.Benchmark)
    fileName = os.path.basename(filePath)
    
    normalizedPath = os.path.normpath(filePath)
    dirPath = os.path.dirname(normalizedPath)
    folderName = os.path.basename(dirPath)

    G = nx.DiGraph(nx.nx_pydot.read_dot(filePath))


    valid_input_pins_name = frozenset({'inPinA', 'inPinB'})
    valid_output_pins_name = frozenset({'outPinA'})

    currently_used_input_pins = set()

    for node in G.nodes:
        currently_used_input_pins = set(valid_input_pins_name)
        # Iterate over all outgoing edges for the current node and add the dedicated input 
        # pins to the currently_used_input_pins set 
        for u, v, properties in G.in_edges(node, data=True):
            # print(f"Edge from {u} to {v} has properties {properties}")
            desired_pin = properties.get('operand')
            currently_used_output_pins = set(valid_output_pins_name)

            if 'opcode' in G.nodes[v] and G.nodes[v]['opcode'].upper() == 'OUTPUT':
                G[u][v]['load'] = 'inPinA'
            elif desired_pin in valid_input_pins_name:
                G[u][v]['load'] = desired_pin
                currently_used_input_pins.remove(desired_pin)
            
            if 'opcode' in G.nodes[v] and G.nodes[v]['opcode'].upper() == 'INPUT':
                G[u][v]['driver'] = 'outPinA'
            else:
                G[u][v]['driver'] = currently_used_output_pins.pop()
            
        for u, v, properties in G.in_edges(node, data=True):
            desired_pin = properties.get('operand')
            currently_used_output_pins = set(valid_output_pins_name)
            
            if 'opcode' in G.nodes[v] and G.nodes[v]['opcode'].upper() == 'OUTPUT':
                G[u][v]['load'] = 'inPinA'
            elif desired_pin not in valid_input_pins_name:
                new_pin_name = currently_used_input_pins.pop()
                G[u][v]['load'] = new_pin_name
            
            if 'opcode' in G.nodes[v] and G.nodes[v]['opcode'].upper() == 'INPUT':
                G[u][v]['driver'] = 'outPinA'
            else:
                G[u][v]['driver'] = currently_used_output_pins.pop()

        for u, v, properties in G.in_edges(node, data=True):
            if 'operand' in properties:
                del properties['operand']
        
        if 'shape' in G.nodes[node]:
            del G.nodes[node]['shape']

        if 'type' in G.nodes[node]:
            del G.nodes[node]['type']
        
        if 'opcode' in G.nodes[node]:
            G.nodes[node]['opcode'] = G.nodes[node]['opcode'].upper()
        
    # -------------------------------------------------------
    #  Writing device model graph for Riken architecture
    # -------------------------------------------------------

    #Pragma lines for the basic Riken benchmarks
    pragmaComment = [
        "/* ------- Application graph pragma -------\n",
        "[SupportedOps] = {ALU, FADD, FMUL};\n"
        "[SupportedOps] = {MEMPORT, INPUT, OUTPUT};\n"
        "[SupportedOps] = {Constant, CONST};\n"
        "*/\n"
        "\n"
    ]
    
    # output_file = "modified_" + str(args.Benchmark)
    outputDir = str(args.OutputDir) + "/" + str(folderName)

    if not os.path.exists(outputDir):
        os.makedirs(outputDir)

    os.chdir(outputDir)
    output_file = str(fileName)

    with open(output_file, 'w') as f:
        for pragma in pragmaComment:
            f.write(pragma)

            
        nx.nx_pydot.write_dot(G, f)
    
    
# Main function for modifying the application graph for UGRAMM:
def main(args):
    print("====================================================")
    print("============ Universal GRAMM (UGRAMM) ==============")
    print("======= helper script: Application Graph Modifier ======")
    print("====================================================\n")

    #Printing arguments:
    print("Generating device model for following CGRA configuration: ")
    print("> Benchmark:", args.Benchmark)
    print("> Output Directory:", args.OutputDir)
    modify_application(args)

if __name__ == "__main__":

    parser = argparse.ArgumentParser()  # Create ArgumentParser object

    # Add arguments with specific names and types
    parser.add_argument('-Benchmark')
    parser.add_argument('-OutputDir')

    args = parser.parse_args()          # Parse the arguments

    main(args)                          # Calling main function