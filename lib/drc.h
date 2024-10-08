//===================================================================//
// Universal GRAaph Minor Mapping (UGRAMM) method for CGRA           //
// file : drc.h                                                      //
// description: contains variables and function declaration related  //
//              to DRC.                                              //
//===================================================================//

#ifndef DRC_H
#define DRC_H

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
#include <vector>
#include <boost/graph/connected_components.hpp>
#include <boost/graph/copy.hpp>
#include <sys/time.h>



//Properties of the application and device model graph used in DRC:
typedef boost::adjacency_list<boost::listS, boost::vecS, boost::undirectedS, DotVertex, EdgeProperty> UnDirectedGraph;

//------------ The following sections is for DRC Rules check for Device Model Graph -----------//
/**
 * Input PinCell nodes in the Device Model Graph must have one fanout edges that is connected to a funcCell. While output PinCell nodes in the Device Model Graph must also have 
 * one fanin edges that is also connected to a funcCells
 * 
 * @param G A pointer to the device-model graph.
 * @param gConfig A map containing node configuration details of device-model graph.
 * @param errorDetected A pointer to a bool variable to indicate an DRC error is found.
 */
void deviceModelDRC_VerifyPinNodes(DirectedGraph *G, std::map<int, NodeConfig> *gConfig, bool *errorDetected);

/**
 * Device model graphs should not have any floating nodes
 * 
 * @param G A pointer to the device-model graph.
 * @param gConfig A map containing node configuration details of device-model graph.
 * @param errorDetected A pointer to a bool variable to indicate an DRC error is found.
 */
void deviceModelDRC_CheckFloatingNodes(DirectedGraph *G, std::map<int, NodeConfig> *gConfig, bool *errorDetected);

/**
 *  Check if the device model graph is weakly conencted
 * 
 * @param G A pointer to the device-model graph.
 * @param gConfig A map containing node configuration details of device-model graph.
 * @param errorDetected A pointer to a bool variable to indicate an DRC error is found.
 */
void deviceModelDRC_CheckDeviceModelWeaklyConnected(DirectedGraph *G, std::map<int, NodeConfig> *gConfig, bool *errorDetected);


/**
 *  Check if FuncCells in the Device Model Graph is connected to other FuncCell. If any two FuncCell does not have a path, this means that the graph is disjointed.
 * 
 * @param G A pointer to the device-model graph.
 * @param gConfig A map containing node configuration details of device-model graph.
 * @param errorDetected A pointer to a bool variable to indicate an DRC error is found.
 */
void deviceModelDRC_CheckFuncCellConnectivity(DirectedGraph *G, std::map<int, NodeConfig> *gConfig, bool *errorDetected);


/**
 *  Check if the device model dot files has the correct attributes and also verify if the attributes haves been correctly loaded into the gConfig data structure
 * 
 * @param G A pointer to the device-model graph.
 * @param gConfig A map containing node configuration details of device-model graph.
 * @param errorDetected A pointer to a bool variable to indicate an DRC error is found.
 */
void deviceModelDRC_CheckDeviceModelAttributes(DirectedGraph *G, std::map<int, NodeConfig> *gConfig, bool *errorDetected);


//------------ The following sections is for DRC Rules check for Application Model Graph -----------//
/**
 * Application graph should not have any floating nodes
 * 
 * @param H A pointer to the application graph.
 * @param hConfig A map containing node configuration details of application graph.
 * @param errorDetected A pointer to a bool variable to indicate an DRC error is found.
 */
void applicationGraphDRC_CheckFloatingNodes(DirectedGraph *H, std::map<int, NodeConfig> *hConfig, bool *errorDetected);


/**
 * Pin names used in the application DFG must follow the names defined in inPin  and outPin vectors in GRAMM.cpp
 * 
 * @param H A pointer to the application graph.
 * @param hConfig A map containing node configuration details of application graph.
 * @param errorDetected A pointer to a bool variable to indicate an DRC error is found.
 */
void applicationGraphDRC_CheckPinNames(DirectedGraph *H, std::map<int, NodeConfig> *hConfig, bool *errorDetected);


/**
 * Check if the application DFG is weakly conencted
 * 
 * @param H A pointer to the application graph.
 * @param hConfig A map containing node configuration details of application graph.
 * @param errorDetected A pointer to a bool variable to indicate an DRC error is found.
 */
void applicationGraphDRC_CheckApplicationDFGWeaklyConnected(DirectedGraph *H, std::map<int, NodeConfig> *hConfig, bool *errorDetected);

/**
 *  Check if the application DFG dot files has the correct attributes and also verify if the attributes haves been correctly loaded into the hConfig data structure
 * 
 * @param H A pointer to the application graph.
 * @param hConfig A map containing node configuration details of application graph.
 * @param errorDetected A pointer to a bool variable to indicate an DRC error is found.
 */
void applicationGraphDRC_CheckDeviceModelAttributes(DirectedGraph *H, std::map<int, NodeConfig> *hConfig, bool *errorDetected);


/**
 *  Check if the application DFG dot files has the correct attributes and also verify if the attributes haves been correctly loaded into the hConfig data structure
 * 
 * @param H A pointer to the application graph.
 * @param hConfig A map containing node configuration details of application graph.
 * @param errorDetected A pointer to a bool variable to indicate an DRC error is found.
 */
void applicationGraphDRC_CheckDeviceModelAttributes(DirectedGraph *H, std::map<int, NodeConfig> *hConfig, bool *errorDetected);

/**
 *  Check if there is a dupplication in locked device model nodes between different vertices of the application graph
 * 
 * @param H A pointer to the application graph.
 * @param hConfig A map containing node configuration details of application graph.
 * @param errorDetected A pointer to a bool variable to indicate an DRC error is found.
 */
void applicationGraphDRC_CheckDupplicationInLockNodes(DirectedGraph *H, std::map<int, NodeConfig> *hConfig, bool *errorDetected);

/**
 *  Check that the locked device model node is the correct node type for the application graph's vertecies
 * 
 * @param H A pointer to the application graph.
 * @param hConfig A map containing node configuration details of application graph.
 * @param gConfig A map containing node configuration details of device-model graph.
 * @param errorDetected A pointer to a bool variable to indicate an DRC error is found.
 */
void applicationGraphDRC_CheckLockNodeType(DirectedGraph *H, std::map<int, NodeConfig> *hConfig, std::map<int, NodeConfig> *gConfig, bool *errorDetected);


//------------ The following sections is the functions that runs all DRC rules -----------//
/**
 * Main DRC function that run through all DRC rules check for both H and G graph seen above
 * 
 * @param H A pointer to the application-model graph.
 * @param G A pointer to the device-model graph.
 * @param hConfig A map containing node configuration details of application-model graph.
 * @param gConfig A map containing node configuration details of device-model graph.
 */
double runDRC(DirectedGraph *H, DirectedGraph *G, std::map<int, NodeConfig> *hConfig, std::map<int, NodeConfig> *gConfig);

#endif