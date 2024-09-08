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
#include <boost/algorithm/string.hpp>
#include <sys/time.h>
#include "spdlog/spdlog.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <boost/program_options.hpp>

//-------------------------------------------------------------------//
//------------------------ GRAMM Configuration ----------------------//
#define MAX_DIST 10000000
#define RIKEN 1                 //Defining the architecture type (1 --> Riken, 0 --> Adres)
#define DEBUG 0                 //For enbaling the print-statements 
#define computeTopoEnable 0     //Compute topological order and sort the graph based on it while finding the minor.
#define maxIterations 39
//-------------------------------------------------------------------//

//CGRA device model parameters:
extern int numR;       //Number of rows in the CGRA architecture
extern int numC;       //Number of columns in the CGRA architecture
extern int CGRAdim;    //CGRA architecture dimension (3 means --> 3x3)

//Pathefinder cost parameters:
extern float PFac;  //Congestion cost factor
extern float HFac;  //History cost factor

//Logger variable:
extern std::shared_ptr<spdlog::logger>  GRAMM;

extern std::vector<std::string> inPin;
extern std::vector<std::string> outPin;

//New way for node and opcode types:
extern std::map<std::string, std::vector<std::string>> GrammConfig;    //New general way

struct NodeConfig {
    //G:
    std::string Cell;          //[New way] Cell-type of the node --> FuncCell, RouteCell, PinCell
    std::string Type;          //[New way] Node-Type of the node --> io, alu, memport....
    int Latency = 0;           //Optional            
    int Width   = 0;           //Optional  

    //H:
    std::string Opcode;        //[New way] Opcode of the node --> FADD, FMUL, FSUB, INPUT, OUTPUT, etc.
    std::string loadPin;       //Load pin of the PinCell node --> inPinA, inPinB
    std::pair<int, int> Location = {0,0 }; //Optional  
};


//Struct for defining the node types in both the application and device model graph:
struct DotVertex {
    // For [H] --> Application Graph
    std::string name;       //Contains name of the operation in Application graph (ex: Load_0)
    std::string opcode;     //Contains the Opcode of the operation (ex: FMUL, FADD, INPUT, OUTPUT etc.)
    std::string type;       //Contains the type of the operation (ex: ALU, MEMPORT, CONST etc.)
    // For [G] --> Device Model Graph
    std::string G_Name;       //Contains the unique name of the cell in the device model graph.
    std::string G_NodeCell;   //Contains the Opcode of the NodeType (FuncCell, RouteCell, PinCell)
    std::string G_NodeType;   //Contains the Node type of Device Model Graph (For example "ALU" for NodeType "FuncCell") 
    float G_VisualX;
    float G_VisualY;
};

//Struct for defining the edge types in the H graph to determine the pin layout
struct EdgeProperty {
    std::string loadPin;
    std::string driverPin;
};

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

extern std::vector<RoutingTree> *Trees;
extern std::vector<std::list<int>> *Users;
extern std::vector<int> *HistoryCosts;
extern std::vector<int> *TraceBack;
extern std::vector<int> *TopoOrder;
extern std::map<int, std::string> hNames;    //Map for storing the unique names of Application graph
extern std::map<int, std::string> gNames;    //Map for storing the unique names of device model graph
extern std::bitset<100000> explored;

//Properties of the application and device model graph:
typedef boost::property<boost::edge_weight_t, int> EdgeWeightProperty;
typedef boost::adjacency_list<boost::listS, boost::vecS, boost::bidirectionalS, DotVertex, EdgeProperty, EdgeWeightProperty > DirectedGraph;
typedef boost::graph_traits<DirectedGraph>::edge_iterator edge_iterator;
typedef boost::graph_traits<DirectedGraph>::in_edge_iterator in_edge_iterator;
typedef boost::graph_traits<DirectedGraph>::out_edge_iterator out_edge_iterator;
typedef boost::graph_traits<DirectedGraph>::vertex_descriptor vertex_descriptor;
typedef boost::graph_traits<DirectedGraph>::out_edge_iterator OutEdgeIterator;
typedef DirectedGraph::edge_descriptor Edge;

#endif //GRAMM header