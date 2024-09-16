//===================================================================//
// GRAap Minor Mapping (GRAMM) method for CGRA                       //
// file : GRAMM.h                                                    //
// description: contains global variables and associated functions   //
//===================================================================//

#ifndef GRAMM_H
#define GRAMM_H

#include <boost/config.hpp>
#include <iostream>
#include <fstream>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/copy.hpp>
#include <queue>
#include <map>
#include <list>
#include <bitset>
#include <algorithm>
#include <vector>
#include <limits>
#include <boost/algorithm/string.hpp>
#include <sys/time.h>
#include "spdlog/spdlog.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <boost/program_options.hpp>
#include "../lib/json.h"

//-------------------------------------------------------------------//
//------------------------ GRAMM Configuration ----------------------//
#define MAX_DIST 10000000
#define RIKEN 1                 //Defining the architecture type (1 --> Riken, 0 --> Adres)
#define DEBUG 0                 //For enbaling the print-statements 
#define computeTopoEnable 0     //Compute topological order and sort the graph based on it while finding the minor.
#define maxIterations 39
//-------------------------------------------------------------------//

//Struct for defining the node configuration in hConfig and gConfig data-structures:
struct NodeConfig {
    // For [G] --> Device Model Graph
    std::string Cell;          //Cell-type --> FuncCell, RouteCell, PinCell
    std::string Type;          //Node-Type --> io, alu, memport....
    int Latency = 0;           //Optional            

    // For [H] --> Application Graph
    std::string Opcode;        //OpcodeType --> FADD, FMUL, FSUB, INPUT, OUTPUT, etc.
    std::string loadPin;       //Load pin of the PinCell node --> inPinA, inPinB
    std::pair<int, int> Location = {0,0}; //Optional  
};

//Struct for defining the expected attributes defined in the h and g graph:
struct DotVertex {
    // For [H] --> Application Graph
    std::string H_Name;       //[Required] Contains name of the operation in Application graph (ex: Load_0)
    std::string H_Opcode;     //[Required] Contains the Opcode of the operation (ex: FMUL, FADD, INPUT, OUTPUT etc.)
    std::string H_Latency;    //[Optional] Contains the latency of the operation
    std::string H_PlacementX; //[Optional] Contains the placement location of X
    std::string H_PlacementY; //[Optional] Contains the placement location of Y

    // For [G] --> Device Model Graph
    std::string G_Name;       //[Required] Contains the unique name of the cell in the device model graph.
    std::string G_NodeCell;   //[Required] Contains the Opcode of the NodeType (FuncCell, RouteCell, PinCell)
    std::string G_NodeType;   //[Required] Contains the Node type of Device Model Graph (For example "ALU" for NodeType "FuncCell") 
    std::string G_VisualX;    //[Optional] Visual X co-ordinate
    std::string G_VisualY;    //[Optional] Visual Y co-ordinate
};

//Struct for defining the edge types in the H graph to determine the pin layout
struct EdgeProperty {
    // For [H] --> Application Graph
    std::string H_LoadPin;   //[Required]
    std::string H_DriverPin;   //[Required]
    // For [H] --> Application Graph and [G] --> Device Model Graph
    int weight;
};

//Properties of the application and device model graph:
typedef boost::adjacency_list<boost::listS, boost::vecS, boost::bidirectionalS, DotVertex, EdgeProperty> DirectedGraph;
typedef boost::adjacency_list<boost::listS, boost::vecS, boost::undirectedS, DotVertex, EdgeProperty> UnDirectedGraph;
typedef boost::graph_traits<DirectedGraph>::edge_iterator edge_iterator;
typedef boost::graph_traits<DirectedGraph>::in_edge_iterator in_edge_iterator;
typedef boost::graph_traits<DirectedGraph>::out_edge_iterator out_edge_iterator;
typedef boost::graph_traits<DirectedGraph>::vertex_descriptor vertex_descriptor;
typedef boost::graph_traits<DirectedGraph>::vertex_iterator vertex_iterator;
typedef boost::graph_traits<DirectedGraph>::out_edge_iterator OutEdgeIterator;
typedef DirectedGraph::edge_descriptor Edge;

// the routing tree for a signal
// explanation for the three fields:
// for each node in the tree, we have a map that returns a list of its child nodes (i.e. the nodes it drives)
// for each node in the tree, we have a map that returns its parent node (i.e., the unique node that drives it)
// we also keep track of the list of all nodes in the routing tree
struct RoutingTree {
  std::map<int, std::list<int>> children;
  std::map<int, int> parent;
  std::list<int> nodes;
};

// Routing related variables:
extern std::vector<RoutingTree> *Trees;      // routing trees for every node in H
extern std::vector<std::list<int>> *Users;   // for every node in device model G track its users
extern std::map<int, int> invUsers;          // InvUsers, key = hID, value = current_mapped gID   
extern std::vector<int> *HistoryCosts;       // for history of congestion in PathFinder
extern std::vector<int> *TraceBack;          
extern std::vector<int> *TopoOrder;          // NOT USED: for topological order
extern std::map<int, std::string> hNames;    //Map for storing the unique names of Application graph
extern std::map<int, std::string> gNames;    //Map for storing the unique names of device model graph
extern std::bitset<100000> explored;

//Pathefinder cost parameters:
extern float PFac;  //Congestion cost factor
extern float HFac;  //History cost factor

//Logger variable:
extern std::shared_ptr<spdlog::logger>  GRAMM;

extern std::vector<std::string> inPin;
extern std::vector<std::string> outPin;

//New way for node and opcode types:
extern std::map<std::string, std::vector<std::string>> GrammConfig;    //New general way

//JSON parsing for the config file:
using json = nlohmann::json;
extern json jsonParsed;

//Function declaration:

/**
 * Finds a minor embedding of graph H into graph G.
 * 
 * 
 * @param H Pointer to the directed graph H (original application graph read from a DOT file).
 * @param G Pointer to the directed graph G (original device model read from a DOT file).
 * @param hConfig Pointer to a map containing configuration information about nodes in graph H.
 * @param gConfig Pointer to a map containing configuration information about nodes in graph G.
 * @return int Returns an integer indicating the success or failure of finding the minor embedding.
 */
int findMinorEmbedding(DirectedGraph *H, DirectedGraph *G, std::map<int, NodeConfig> *hConfig, std::map<int, NodeConfig> *gConfig);

/**
 * Finds the minimal vertex model for embedding.
 * 
 * 
 * @param H Pointer to the directed graph H (original application graph read from a DOT file).
 * @param G Pointer to the directed graph G (original device model graph read from a DOT file).
 * @param y Integer indicating a vertex in graph H to begin from.
 * @param hConfig Pointer to a map containing configuration information about nodes in graph H.
 * @param gConfig Pointer to a map containing configuration information about nodes in graph G.
 * @return int Returns an integer indicating the success or failure of finding the minimal embedding.
 */
int findMinVertexModel(DirectedGraph *G, DirectedGraph *H, int y, std::map<int, NodeConfig> *hConfig, std::map<int, NodeConfig> *gConfig);

#endif //GRAMM header