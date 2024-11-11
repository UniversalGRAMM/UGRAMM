//===================================================================//
// Universal GRAaph Minor Mapping (UGRAMM) method for CGRA           //
// file : routing.h                                                  //
// description: contains variables and function declaration related  //
//              to routing.                                          //
//===================================================================//

#ifndef ROUTING_H
#define ROUTING_H

#include <boost/config.hpp>
#include <iostream>
#include <fstream>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/algorithm/string.hpp>
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

//-------------------------------------------------------------------//
//------------------- [Routing] Helper function ---------------------//
//-------------------------------------------------------------------//

/**
 * This function enables wildcard naming for the locked. 
 *
 * It breaks the provided lock Name string into substrings based on a key. Once a list of 
 * substrings have been created, it checks in the gNamesInv map to see if all substring is 
 * present within a gName.
 *
 * @param key a char used to split the string into substrings
 * @param gName name of node in the device model graph
 * @param lockName name of the locked name with wildcard included
 * @return bool Returns true if all of the substrings of the locked name are found in gName, false otherwise.
 */
bool matchesPattern(const std::string& key, const std::string& gName, const std::string& lockName);

/**
 * This function gets the GID for the the funcCell node in the device model graph that
 * meets the the lockedNodeName
 * 
 * @param lockedNodeName Passes the name for the locked PE node
 * @param suitableGIDs Modifies this vector list with suitable G-graph node IDs 
 * @return void
 */
void findGNodeID_FuncCell(const std::string &lockedNodeName, std::vector<int> &suitableGIDs);

/**
 * For the given outputPin (signal), finds the associated FunCell node from the device model.
 * 
 * This function identifies the FunCell node in the device model graph G associated with the 
 * specified output pin (signal), tracing the source of this edge (walking uphill).
 * 
 * Ex: FunCell(signal) -> outPin (selectedCellOutputPin) [Walking uphill --- finding the source of this edge]
 * 
 * @param signal The output pin (signal) for which to find the associated FunCell.
 * @param G Pointer to the directed graph representing the device model.
 * @return int Returns the FunCell node corresponding to the given output pin.
 */
int findFuncCellFromOutputPin(int signal, DirectedGraph *G);

/**
 * For the given FunCell (signal), finds the associated outputPin node from the device model.
 * 
 * This function identifies the outputPin node in the device model graph G that is associated 
 * with the given FunCell (signal), tracing the sink of the edge (walking downhill).
 * 
 * Ex: FunCell(signal) -> outPin (selectedCellOutputPin) [Walking downhill --- finding the sink of this edge]
 * 
 * @param signal The FunCell (signal) for which to find the associated outputPin.
 * @param G Pointer to the directed graph representing the device model.
 * @return int Returns the outputPin node corresponding to the given FunCell.
 */
int findOutputPinFromFuncCell(int signal, DirectedGraph *G);

/**
 * Finds the root of the vertex model for the given signal.
 * 
 * As GRAMM performs pin-to-pin mapping, the root of the vertex model, 
 * or starting point, will always be an output pin that serves as the 
 * driver for the routing fanout of the specified signal.
 * 
 * @param signal The signal for which to find the root output pin.
 * @param gConfig Pointer to the map containing configuration data for nodes in the graph.
 * @return int Returns the root output pin acting as the driver for the signal.
 */
int findRoot(int signal, std::map<int, NodeConfig> *gConfig);

/**
 * Gets the current cost associated with the given signal.
 * 
 * @param signal The signal whose cost is to be retrieved.
 * @return int The cost of the specified signal.
 */
int getSignalCost(int signal);

/**
 * Checks if there is an overlap for the given signal.
 * Confirms by checking if there are more than one 
 * users for any element in the routing tree.
 * 
 * @param signal The signal to check for overlap.
 * @return bool True if there is an overlap, false otherwise.
 */
bool hasOverlap(int signal);

/**
 * Calculates the total amount of used resources in the device model graph.
 * 
 * @param G Pointer to the directed graph representing the device model.
 * @return int The total used resources in the graph.
 */
int totalUsed(DirectedGraph *G);

/**
 * Calculates the total amount of overused resources in the device model graph.
 * 
 * @param G Pointer to the directed graph representing the device model.
 * @return int The total overused resources in the graph.
 */
int totalOveruse(DirectedGraph *G);

/**
 * Adjusts the history costs for the device model graph.
 * 
 * @param G Pointer to the directed graph representing the device model.
 */
void adjustHistoryCosts(DirectedGraph *G);

/**
 * Comparison function for sorting.
 * 
 * @param a Pointer to the first item.
 * @param b Pointer to the second item.
 * @return int The result of the comparison.
 */
int cmpfunc(const void *a, const void *b);

/**
 * Sorting the nodes of H according to the size (number of vertices) of their vertex model
 * 
 * @param list Pointer to the integer array describing the ordering of the vertices of H graph.
 * @param n The number of elements/vertices in the H graph
 */
void sortList(int list[], int n, std::map<int, NodeConfig> *hConfig);

//-------------------------------------------------------------------//
//-------------------- [Routing] Main function ----------------------//
//-------------------------------------------------------------------//

/**
 * Removes the specified signal from the list of nodes.
 * 
 * @param signal The signal (application-graph-node) whose routing is to be removed.
 * @param nodes Pointer to the list of (device-model-nodes) used in routing structure of the "signal".
 */
void ripup(int signal, std::list<int> *nodes);

/**
 * Removes the routing associated with the given signal from the routing structure.
 * Make a list of used (device-model-nodes) for the given the given siganl (application-graph-node)
 * 
 * @param signal The signal (application-graph-node) whose routing is to be removed.
 * @param G Pointer to the directed graph representing the device model.
 */
void ripUpRouting(int signal, DirectedGraph *G);

/**
 * Deposits the route into the routing structure for the given signal.
 * 
 * @param signal The signal (application-graph node) for which to deposit the route.
 * @param nodes Pointer to the list of device-model nodes used in routing the signal.
 */
void depositRoute(int signal, std::list<int> *nodes);


/**
 * Calculate Pathfinder-based cost for a given vertex.
 *
 * @param next Vertex for which to calculate the cost.
 * @return Calculated cost for the vertex.
 */
float calculate_cost (vertex_descriptor next);

/**
 * Pathfinder approach for routing signal -> sink
 * 
 * 
 * @param G Pointer to the device model graph.
 * @param signal The source node (signal) to be routed.
 * @param sink-set :: The destination nodes (sink) for the routing.
 * @param route Pointer to the list where the route will be stored.
 * @param gConfig Pointer to the map containing node configurations for the device model graph.
 * @param hConfig A map containing node configuration details of device-model graph.
 * @return int Returns an integer cost of the routing path.
 */
int route(DirectedGraph *G, int signal, std::set<int> sink, std::list<int> *route, std::map<int, NodeConfig> *gConfig, std::map<int, NodeConfig> *hConfig);

/**
 * Routes all fanout edges of the specified application-graph node.
 * 
 * @param G Pointer to the device model graph.
 * @param H Pointer to the application graph.
 * @param y The application-graph node whose fanout edges are to be routed.
 * @param gConfig Pointer to the map containing node configurations for the device model graph.
 * @param hConfig A map containing node configuration details of device-model graph.
 * @return int Returns an integer cost of the routing path.
 */
int routeSignal(DirectedGraph *G, DirectedGraph *H, int y, std::map<int, NodeConfig> *gConfig, std::map<int, NodeConfig> *hConfig);

#endif
