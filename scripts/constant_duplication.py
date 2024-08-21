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

    # directoryname = os.path.dirname(file_path)
    # input_file = str(args.Benchmark)
    # base_path = os.path.normpath(file_path)  # Normalize path for consistency
    # extracted_path = os.path.join(os.path.basename(os.path.dirname(base_path)), os.path.basename(base_path))

    G = nx.DiGraph(nx.nx_pydot.read_dot(filePath))

    # Get all the constant that has a fanout and sore them in the list called constFanoutNodes
    nodeAttributes = G.nodes(data=True)
    constFanoutNodes = list()
    for node, attributes in nodeAttributes:
        opcode = attributes.get('opcode')
        # Currently, we are only checking if const a fanout. Can
        # be modified to have more for other functions
        if opcode == "const" and G.out_degree(node) > 1:
            print (f"Constant Found that has a fanout at node {node}")
            constFanoutNodes.append(node)

    # For each const node whose fanout signals needs to resolve, 
    # we will duplicate the constant to remove the fanout
    for node in constFanoutNodes:
        # Getting the outedges for the node in question
        outEdges = list(G.out_edges(node, data=True))
        # Creating a new driver node for all fanout outedges except for the first one
        for u, v, properties in outEdges[1:]:
            # Getting a new Unique ID
            newNodeID = len(G.nodes) + 1
            # Moddifying the newNode name to indicate the operation and node id (ex const_36)
            prefix, delimiter, number = u.partition('_')
            newNode = f"{prefix}{delimiter}{newNodeID}"
            # Creating a duplicate of the driver node while also copying all the respective attribute
            nodeProperties = G.nodes[node]
            G.add_node(newNode, **nodeProperties)
            # Adding the edge between the new driver node and the sink and transfering over the edge attribute
            load = properties.get('load')
            driver = properties.get('driver')
            G.add_edge(newNode, v)
            G[newNode][v]['load'] = load
            G[newNode][v]['driver'] = driver

    # Deleting all the fan out edges between the constant
    for node in constFanoutNodes:
        u, v, properties = outEdges[0]
        out_edges = list(G.out_edges(node))
        G.remove_edges_from(out_edges)
        G.add_edge(u, v, **properties)

    # for node in G.nodes():
    #     # Get the out-edges of the current node
    #     out_edges = list(G.out_edges(node))
        
    #     # If the node has more than one out-edge, add it to constFanoutNodes
    #     if len(out_edges) > 1:
    #         constFanoutNodes.append(node)

    # # Step 2: Remove out-edges of nodes in constFanoutNodes
    # for node in constFanoutNodes:
    #     # Get the out-edges of the current node again (after identification)
    #     out_edges = list(G.out_edges(node))
        
    #     # Remove all out-edges of the node
    #     G.remove_edges_from(out_edges)



    # -------------------------------------------------------
    #  Writing device model graph for Riken architecture
    # -------------------------------------------------------
    # output_file = "modified_" + str(args.Benchmark)
    outputDir = str(args.OutputDir) + "/" + str(folderName)

    if not os.path.exists(outputDir):
        os.makedirs(outputDir)

    os.chdir(outputDir)
    output_file = "modified_" + str(fileName)
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
    print("> Output Directory:", args.OutputDir)
    modify_application(args)

if __name__ == "__main__":

    parser = argparse.ArgumentParser()  # Create ArgumentParser object

    # Add arguments with specific names and types
    parser.add_argument('-Benchmark')
    parser.add_argument('-OutputDir')

    args = parser.parse_args()          # Parse the arguments

    main(args)                          # Calling main function